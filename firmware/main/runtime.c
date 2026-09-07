#include "runtime.h"
#include "board.h"
#include "voicekey_usb.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include <inttypes.h>

static vk_state_t state;
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
#define LOCK() portENTER_CRITICAL(&mux)
#define UNLOCK() portEXIT_CRITICAL(&mux)
static uint64_t now_ms(void) { return esp_timer_get_time() / 1000; }
void vk_runtime_init(void) { vk_init(&state, CONFIG_VOICEKEY_COOLDOWN_MS); }
void vk_runtime_inputs(void) {
    LOCK(); vk_set_mute(&state, vk_board_muted()); UNLOCK();
}
uint32_t vk_runtime_audio_epoch(void) {
    LOCK(); uint32_t epoch = state.audio_epoch; UNLOCK(); return epoch;
}
void vk_runtime_audio_ok(bool ok) { LOCK(); vk_set_audio_ok(&state, ok); UNLOCK(); }
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
static void connected(bool v) { LOCK(); vk_set_connected(&state, v); UNLOCK(); }
static void streaming(bool v) { LOCK(); vk_set_streaming(&state, v); UNLOCK(); }
static void host_mute(bool v) { LOCK(); vk_set_host_mute(&state, v); UNLOCK(); }
static vk_hid_action_t hid_next(uint64_t now) {
    LOCK(); vk_set_mute(&state, vk_board_muted());
    vk_hid_action_t a = vk_hid_next(&state, now); UNLOCK(); return a;
}
static void hid_commit(vk_hid_action_t a, uint64_t now) {
    LOCK(); vk_hid_commit(&state, a, now); UNLOCK();
}
esp_err_t vk_runtime_usb_start(void) {
    const vk_usb_hooks_t hooks = {.audio_packet = audio_packet, .connected = connected,
        .streaming = streaming, .host_mute = host_mute, .hid_next = hid_next, .hid_commit = hid_commit};
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
        (!state.audio_ok || !wake_ok) ? 0x201000 :
        (now < state.led_until_ms) ? 0x001020 : 0;
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
