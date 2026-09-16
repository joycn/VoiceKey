#include "runtime.h"
#include "board.h"
#include "voicekey_usb.h"
#include "voicekey_audio.h"
#include "voicekey_descriptors.h"
#include "esp_psram.h"
#include "esp_system.h"
#include <string.h>
#include "esp_app_desc.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include <inttypes.h>

static vk_state_t state;
static vk_playback_t playback;
static uint32_t dma_consumed, capture_generation, transport_faults;
static int64_t last_dma_us;
static bool transport_ready, playback_requested, initialization_complete, wake_ready;
static uint32_t board_init_error = VK_INIT_PENDING, audio_init_error = VK_INIT_PENDING, wake_init_error = VK_INIT_PENDING;
#define VK_TX_STALE_US 5000

static int32_t *dma_buffers[3];
static size_t dma_sizes[3];
static void reset_playback(bool active) {
    vk_playback_active(&playback, active);
    for (unsigned i = 0; i < 3; ++i) if (dma_buffers[i]) memset(dma_buffers[i], 0, dma_sizes[i]);
}
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
#define LOCK() portENTER_CRITICAL(&mux)
#define UNLOCK() portEXIT_CRITICAL(&mux)
static uint64_t now_ms(void) { return esp_timer_get_time() / 1000; }
void vk_runtime_init(void) { vk_init(&state, CONFIG_VOICEKEY_COOLDOWN_MS); vk_playback_init(&playback); }
static void transport_fault(void) {
    if (transport_ready) {
        transport_faults++; capture_generation++; vk_set_audio_ok(&state, false);
    }
    transport_ready = false;
    reset_playback(false);
}
void vk_runtime_inputs(void) {
    vk_board_status_t board; vk_board_status(&board);
    int64_t now = esp_timer_get_time();
    LOCK();
    vk_set_mute(&state, !board.valid || !board.i2s_active || board.muted);
    if (transport_ready && (!last_dma_us || now - last_dma_us > VK_TX_STALE_US)) transport_fault();
    UNLOCK();
}
uint32_t vk_runtime_audio_epoch(void) {
    LOCK(); uint32_t epoch = state.audio_epoch; UNLOCK(); return epoch;
}
void vk_runtime_audio_ok(bool ok) {
    LOCK(); if (!ok) capture_generation++; vk_set_audio_ok(&state, ok); UNLOCK();
}
void vk_runtime_capture_fault_isr(void) {
    portENTER_CRITICAL_ISR(&mux);
    capture_generation++; vk_set_audio_ok(&state, false);
    portEXIT_CRITICAL_ISR(&mux);
}
uint32_t vk_runtime_capture_generation(void) {
    LOCK(); uint32_t generation = capture_generation; UNLOCK(); return generation;
}
bool vk_runtime_capture_recover(uint32_t generation) {
    LOCK(); bool fresh = generation == capture_generation;
    if (fresh) vk_set_audio_ok(&state, true);
    UNLOCK(); return fresh;
}
void vk_runtime_init_status(uint32_t board, uint32_t audio, uint32_t wake, bool complete) {
    LOCK(); board_init_error = board; audio_init_error = audio; wake_init_error = wake;
    initialization_complete = complete; wake_ready = complete && wake == ESP_OK; UNLOCK();
}
bool vk_runtime_wake_ready(void) { LOCK(); bool ready = wake_ready; UNLOCK(); return ready; }
void vk_runtime_publish(const int16_t *samples, size_t count, uint32_t epoch) {
    LOCK(); vk_set_mute(&state, vk_board_muted()); vk_publish(&state, samples, count, epoch); UNLOCK();
}
size_t vk_runtime_wake_read(int16_t *samples, size_t count, uint32_t *epoch) {
    LOCK(); vk_set_mute(&state, vk_board_muted());
    size_t n = vk_wake_read(&state, samples, count, epoch); UNLOCK(); return n;
}
void vk_runtime_trigger(uint32_t epoch) {
    LOCK(); vk_set_mute(&state, vk_board_muted());
    vk_request_trigger(&state, now_ms(), epoch); UNLOCK();
}
static size_t audio_packet(int16_t *samples) {
    LOCK(); vk_set_mute(&state, vk_board_muted());
    size_t count = vk_usb_packet_samples(&state);
    vk_usb_read(&state, samples, count); UNLOCK(); return count;
}
static void connected(bool v) { LOCK(); vk_set_connected(&state, v); playback_requested = false; reset_playback(false); UNLOCK(); }
static void streaming(bool v) { LOCK(); vk_set_streaming(&state, v); UNLOCK(); }
static void host_mute(bool v) { LOCK(); vk_set_host_mute(&state, v); UNLOCK(); }
static vk_hid_action_t hid_next(uint64_t now) {
    LOCK(); vk_set_mute(&state, vk_board_muted());
    vk_hid_action_t a = vk_hid_next(&state, now); UNLOCK(); return a;
}
static void hid_commit(vk_hid_action_t a, uint64_t now) {
    LOCK(); vk_hid_commit(&state, a, now); UNLOCK();
}

static void playback_active(bool v) { LOCK(); playback_requested = v; reset_playback(v && transport_ready); UNLOCK(); }
static void playback_packet(const int16_t *samples, size_t frames) { LOCK(); vk_playback_push(&playback, samples, frames); UNLOCK(); }
static void playback_control(bool mute, int16_t volume) { LOCK(); vk_playback_control(&playback, mute, volume); UNLOCK(); }
void vk_runtime_playback_dma(int32_t *stereo, size_t frames) {
    portENTER_CRITICAL_ISR(&mux);
    for (unsigned i = 0; i < 3; ++i) {
        if (dma_buffers[i] == stereo) break;
        if (!dma_buffers[i]) { dma_buffers[i] = stereo; dma_sizes[i] = frames * 8; break; }
    }
    int64_t now = esp_timer_get_time();
    bool fresh_clock = last_dma_us && now - last_dma_us <= VK_TX_STALE_US;
    if (!fresh_clock) transport_fault();
    else if (!transport_ready) { transport_ready = true; reset_playback(playback_requested); }
    last_dma_us = now;
    dma_consumed += frames;
    vk_playback_render(&playback, stereo, frames);
    portEXIT_CRITICAL_ISR(&mux);
}
static uint32_t feedback(uint64_t now) {
    LOCK(); uint32_t value = vk_playback_feedback(&playback, dma_consumed, now); UNLOCK(); return value;
}
static void put32(uint8_t *b, size_t offset, uint32_t v) {
    for (unsigned i = 0; i < 4; ++i) b[offset+i] = (uint8_t)(v >> (8*i));
}
static size_t diagnostics(uint8_t *buffer, size_t capacity) {
    uint8_t b[VK_DIAGNOSTIC_SIZE] = {2};
    vk_board_status_t board; vk_board_status(&board);
    memcpy(b+1, board.version, 3);
    b[4] = (board.valid ? 1 : 0) | (board.muted ? 2 : 0) | (board.i2s_active ? 4 : 0);
    b[5] = 1; b[6] = 1; /* VoiceKey firmware protocol release 1.1 */
    put32(b, 8, board.errors); put32(b, 12, board.busy);
    uint64_t age = now_ms() - board.updated_ms; put32(b, 16, age > UINT32_MAX ? UINT32_MAX : age);
    LOCK();
    b[7] = (state.audio_ok ? 1 : 0) | (state.connected ? 2 : 0) | (state.streaming ? 4 : 0) | (playback.active ? 8 : 0) | (wake_ready ? 16 : 0) |
        (initialization_complete ? 32 : 0) | (transport_ready ? 64 : 0);
    put32(b, 20, state.usb.count); put32(b, 24, state.wake.count); put32(b, 28, playback.count);
    put32(b, 32, state.usb.dropped); put32(b, 36, state.usb.missing); put32(b, 40, state.wake.dropped);
    put32(b, 44, playback.dropped); put32(b, 48, playback.missing); put32(b, 52, dma_consumed);
    memcpy(b + 56, esp_app_get_description()->app_elf_sha256, 8);
    put32(b, 64, board_init_error); put32(b, 68, audio_init_error); put32(b, 72, wake_init_error);
    put32(b, 88, capture_generation); put32(b, 92, transport_faults);
    UNLOCK();
    put32(b, 76, esp_psram_get_size()); put32(b, 80, esp_get_free_heap_size());
    put32(b, 84, esp_get_minimum_free_heap_size());
    size_t n = capacity < sizeof(b) ? capacity : sizeof(b); memcpy(buffer, b, n); return n;
}

esp_err_t vk_runtime_usb_start(void) {
    const vk_usb_hooks_t hooks = {.audio_packet = audio_packet, .connected = connected,
        .streaming = streaming, .host_mute = host_mute, .playback_active = playback_active,
        .playback_packet = playback_packet, .playback_control = playback_control, .feedback = feedback,
        .diagnostics = diagnostics, .hid_next = hid_next, .hid_commit = hid_commit};
    return vk_usb_start(&hooks);
}
void vk_runtime_ui(bool wake_ok) {
    static bool raw_last, stable;
    static uint64_t changed_ms;
    static unsigned last_rgb = UINT32_MAX;
    uint64_t now = now_ms();
    bool raw = vk_board_button();
    if (raw != raw_last) { raw_last = raw; changed_ms = now; }
    LOCK();
    vk_set_mute(&state, vk_board_muted());
    if (raw != stable && now - changed_ms >= 30) {
        stable = raw;
        if (stable) vk_request_trigger(&state, now, state.wake_epoch);
    }
    unsigned rgb = state.muted ? 0x200000 :
        (!state.audio_ok || !wake_ok) ? 0x200020 :
        (now < state.led_until_ms) ? 0x000020 : 0;
    UNLOCK();
    if (rgb != last_rgb) { vk_board_led(rgb >> 16, rgb >> 8, rgb); last_rgb = rgb; }
}
void vk_runtime_log(void) {
    LOCK();
    size_t u = state.usb.count, w = state.wake.count;
    uint64_t ud = state.usb.dropped, um = state.usb.missing, wd = state.wake.dropped;
    uint64_t tr = state.triggers, su = state.suppressed;
    bool mute = state.muted, stream = state.streaming, audio = state.audio_ok;
    UNLOCK();
    ESP_LOGI("voicekey", "audio=%d mute=%d streaming=%d usb=%u wake=%u dropped=%"PRIu64"/%"PRIu64
        " underrun=%"PRIu64" triggers=%"PRIu64" suppressed=%"PRIu64,
        audio, mute, stream, (unsigned)u, (unsigned)w, ud, wd, um, tr, su);
}
