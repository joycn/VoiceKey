/* Real application USB callbacks and state machine; only IDF scheduling/USB
   transport are substituted. This does not emulate a USB host/controller. */
#include "voicekey_usb.h"
#include "voicekey_descriptors.h"
#include "esp_private/usb_phy.h"
#include "tusb.h"
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

static vk_state_t state;
static void (*task)(void *);
static jmp_buf task_exit;
static unsigned steps, reports, clears;
static bool ready = true, accept = true;
static uint64_t time_ms;
static uint8_t report[8], control[64], packet[34];
static uint16_t control_len, packet_len;
static size_t audio_packet(int16_t *p) {
    size_t n = vk_usb_packet_samples(&state); vk_usb_read(&state, p, n); return n;
}
static void connected(bool v) { vk_set_connected(&state, v); }
static void streaming(bool v) { vk_set_streaming(&state, v); }
static void host_mute(bool v) { vk_set_host_mute(&state, v); }
static vk_hid_action_t hid_next(uint64_t now) { return vk_hid_next(&state, now); }
static void hid_commit(vk_hid_action_t a, uint64_t now) { vk_hid_commit(&state, a, now); }
int64_t esp_timer_get_time(void) { return (int64_t)time_ms * 1000; }
esp_err_t usb_new_phy(const usb_phy_config_t *cfg, usb_phy_handle_t *phy) {
    assert(cfg->otg_mode == USB_OTG_MODE_DEVICE); *phy = (void *)1; return ESP_OK;
}
bool tusb_rhport_init(uint8_t port, const tusb_rhport_init_t *init) {
    (void)init; assert(port == 0); return true;
}
int xTaskCreatePinnedToCore(void (*fn)(void *), const char *name, uint32_t stack,
        void *arg, unsigned priority, void *handle, int core) {
    (void)name; (void)stack; (void)arg; (void)priority; (void)handle; (void)core;
    task = fn; return 1;
}
void tud_task_ext(uint32_t timeout, bool isr) {
    assert(timeout == 1 && !isr);
    if (!steps--) longjmp(task_exit, 1);
}
bool tud_hid_n_ready(uint8_t instance) { assert(!instance); return ready; }
bool tud_hid_n_keyboard_report(uint8_t instance, uint8_t id, uint8_t modifier, const uint8_t keys[6]) {
    assert(!instance && !id && !modifier);
    if (!accept) return false;
    memset(report, 0, sizeof(report)); memcpy(report + 2, keys, 6); reports++; return true;
}
uint16_t tud_audio_n_write(uint8_t id, const void *data, uint16_t len) {
    assert(!id && len <= sizeof(packet)); memcpy(packet, data, len); packet_len = len; return len;
}
bool tud_audio_n_clear_ep_in_ff(uint8_t id) { assert(!id); clears++; return true; }
bool tud_audio_buffer_and_schedule_control_xfer(uint8_t port, const tusb_control_request_t *req, void *data, uint16_t len) {
    (void)req; assert(!port && len <= sizeof(control));
    memcpy(control, data, len); control_len = len; return true;
}
static void run_at(uint64_t now) {
    time_ms = now; steps = 1;
    if (!setjmp(task_exit)) task(NULL);
}
static tusb_control_request_t request(uint8_t entity, uint8_t selector, uint8_t channel, uint8_t op, uint16_t length) {
    return (tusb_control_request_t){.bRequest = op, .wIndex = (uint16_t)(entity << 8) | VK_ITF_AUDIO_CONTROL,
        .wValue = (uint16_t)(selector << 8) | channel, .wLength = length};
}
static uint32_t le32(const uint8_t *p) { return p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24); }
static void check_controls(void) {
    tusb_control_request_t r = request(4, AUDIO_CS_CTRL_SAM_FREQ, 0, AUDIO_CS_REQ_CUR, 4);
    assert(tud_audio_get_req_entity_cb(0, &r) && control_len == 4 && le32(control) == 16000);
    r.bRequest = AUDIO_CS_REQ_RANGE;
    assert(tud_audio_get_req_entity_cb(0, &r) && control_len == 14);
    assert(control[0] == 1 && control[1] == 0 && le32(control+2) == 16000 && le32(control+6) == 16000 && le32(control+10) == 0);
    r.wIndex = (4 << 8) | VK_ITF_HID; assert(!tud_audio_get_req_entity_cb(0, &r));
    r = request(4, AUDIO_CS_CTRL_SAM_FREQ, 1, AUDIO_CS_REQ_CUR, 4);
    assert(!tud_audio_get_req_entity_cb(0, &r));
    r = request(2, AUDIO_FU_CTRL_VOLUME, 0, AUDIO_CS_REQ_CUR, 2);
    assert(!tud_audio_get_req_entity_cb(0, &r));
    r = request(2, AUDIO_FU_CTRL_MUTE, 0, AUDIO_CS_REQ_CUR, 1);
    uint8_t value = 1;
    assert(tud_audio_set_req_entity_cb(0, &r, &value) && state.host_mute);
    assert(tud_audio_get_req_entity_cb(0, &r) && control_len == 1 && control[0] == 1);
    for (unsigned i = 0; i < 256; ++i) {
        value = i;
        assert(tud_audio_set_req_entity_cb(0, &r, &value) == (i <= 1));
    }
    value = 0; r.wLength = 2;
    assert(!tud_audio_set_req_entity_cb(0, &r, &value) && state.host_mute);
    r.wLength = 1; vk_set_mute(&state, true);
    assert(tud_audio_set_req_entity_cb(0, &r, &value) && state.muted);
    vk_set_mute(&state, false);
}
static void check_audio_lifecycle(void) {
    tusb_control_request_t r = {.wIndex = VK_ITF_AUDIO_STREAM, .wValue = 1};
    assert(tud_audio_set_itf_cb(0, &r) && state.streaming);
    int16_t input[160]; for (unsigned i=0; i<160; ++i) input[i] = i+1;
    vk_publish(&state, input, 160, state.audio_epoch);
    assert(tud_audio_tx_done_pre_load_cb(0, 0, VK_EP_AUDIO, 1) && packet_len == 32);
    assert(!memcmp(packet, input, 32));
    vk_set_mute(&state, true);
    assert(tud_audio_tx_done_pre_load_cb(0, 0, VK_EP_AUDIO, 1));
    for (unsigned i=0; i<packet_len; ++i) assert(packet[i] == 0);
    vk_set_mute(&state, false);
    assert(vk_request_trigger(&state, 100, state.wake_epoch));
    unsigned before = clears;
    tud_suspend_cb(false);
    assert(!state.connected && !state.pending && !state.usb.count && clears > before);
    tud_resume_cb(); assert(state.connected && state.streaming && state.neutral_needed);
    r.wValue = 0;
    assert(tud_audio_set_itf_close_EP_cb(0, &r));
    assert(tud_audio_set_itf_cb(0, &r) && !state.streaming);
    tud_suspend_cb(false); tud_resume_cb(); assert(!state.streaming);
    r.wValue = 2; assert(!tud_audio_set_itf_cb(0, &r));
    tud_umount_cb(); assert(!state.connected);
    tud_mount_cb(); assert(state.connected && !state.streaming && !state.host_mute);
}
static void check_hid_delivery(void) {
    run_at(1000); assert(reports == 1 && !state.neutral_needed && report[2] == 0);
    assert(vk_request_trigger(&state, 1100, state.wake_epoch));
    accept = false; run_at(1100);
    assert(reports == 1 && state.pending && !state.key_down);
    accept = true; run_at(1110);
    assert(reports == 2 && state.key_down && report[2] == VK_KEY_F18);
    uint8_t readback[8];
    assert(tud_hid_get_report_cb(0, 0, HID_REPORT_TYPE_INPUT, readback, 8) == 8);
    assert(!memcmp(readback, report, 8));
    run_at(1139); assert(reports == 2);
    accept = false; run_at(1140); assert(state.key_down);
    accept = true; run_at(1141); assert(reports == 3 && !state.key_down && report[2] == 0);
    assert(vk_request_trigger(&state, 4000, state.wake_epoch));
    ready = false; run_at(4000); ready = true; run_at(4201);
    assert(reports == 3 && !state.pending); /* Expired events are never replayed. */
}
int main(void) {
    vk_init(&state, 2000); vk_set_audio_ok(&state, true);
    const vk_usb_hooks_t hooks = {audio_packet, connected, streaming, host_mute, hid_next, hid_commit};
    assert(vk_usb_start(NULL) == ESP_ERR_INVALID_ARG);
    assert(vk_usb_start(&hooks) == ESP_OK && task);
    tud_mount_cb();
    check_controls(); check_audio_lifecycle(); check_hid_delivery();
    puts("PASS: actual USB callbacks + core: controls, mute, stream/suspend/resume, HID transport failures and expiration");
}
