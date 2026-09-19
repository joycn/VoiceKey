/* Real application USB callbacks and state machine; only IDF scheduling/USB
   transport are substituted. This does not emulate a USB host/controller. */
#include "voicekey_usb.h"
#include "voicekey_audio.h"
#include "voicekey_descriptors.h"
#include "esp_private/usb_phy.h"
#include "tusb.h"
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

static vk_state_t state;
static vk_playback_t playback;
static uint8_t received[196];
static uint16_t received_len;
static uint32_t feedback_value, consumed_frames;
static unsigned feedback_submissions;
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
static void connected(bool v) { vk_set_connected(&state, v); vk_playback_active(&playback, false); }
static void streaming(bool v) { vk_set_streaming(&state, v); }
static void host_mute(bool v) { vk_set_host_mute(&state, v); }
static void playback_active(bool value) { vk_playback_active(&playback, value); }
static void playback_packet(const int16_t *p, size_t n) { vk_playback_push(&playback, p, n); }
static void playback_control(bool mute, int16_t volume) { vk_playback_control(&playback, mute, volume); }
static uint32_t feedback(uint64_t now) { return vk_playback_feedback(&playback, consumed_frames, now); }
static size_t diagnostics(uint8_t *p, size_t n) { memset(p, 0x5a, n); return n; }
bool tud_audio_n_fb_set(uint8_t id, uint32_t value) { assert(!id); feedback_value = value; feedback_submissions++; return true; }
bool tud_audio_n_clear_ep_out_ff(uint8_t id) { assert(!id); received_len = 0; return true; }
uint16_t tud_audio_n_read(uint8_t id, void *p, uint16_t len) {
    assert(!id); if (len > received_len) len = received_len; memcpy(p, received, len); received_len -= len; return len;
}
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
    assert(!instance && !id);
    assert(modifier == (keys[0] ? 0x0e : 0));
    if (!accept) return false;
    memset(report, 0, sizeof(report)); report[0] = modifier; memcpy(report + 2, keys, 6); reports++; return true;
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
    run_at(1000); assert(reports == 1 && !state.neutral_needed && report[2] == 0 && report[0] == 0);
    assert(vk_request_trigger(&state, 1100, state.wake_epoch));
    accept = false; run_at(1100);
    assert(reports == 1 && state.pending && !state.key_down);
    accept = true; run_at(1110);
    assert(reports == 2 && state.key_down && report[2] == 0x16 && report[0] == 0x0e);
    uint8_t readback[8];
    assert(tud_hid_get_report_cb(0, 0, HID_REPORT_TYPE_INPUT, readback, 8) == 8);
    assert(!memcmp(readback, report, 8));
    run_at(1139); assert(reports == 2);
    accept = false; run_at(1140); assert(state.key_down);
    accept = true; run_at(1141); assert(reports == 3 && !state.key_down && report[2] == 0 && report[0] == 0);
    assert(tud_hid_get_report_cb(0, 0, HID_REPORT_TYPE_INPUT, readback, 8) == 8);
    for (unsigned i=0; i<8; ++i) assert(readback[i] == 0);
    assert(vk_request_trigger(&state, 4000, state.wake_epoch));
    ready = false; run_at(4000); ready = true; run_at(4201);
    assert(reports == 3 && !state.pending); /* Expired events are never replayed. */
}
static void check_playback(void) {
    tusb_control_request_t r = {.wIndex = VK_ITF_AUDIO_PLAYBACK, .wValue = 1};
    unsigned before_feedback = feedback_submissions;
    assert(tud_audio_set_itf_cb(0, &r) && playback.active);
    assert(feedback_submissions == before_feedback); /* format correction not ready inside set-itf */
    ready = false; run_at(1); ready = true;
    assert(feedback_submissions == before_feedback + 1 && feedback_value > 47u*65536);
    int16_t samples[98]; for (unsigned i=0;i<98;++i) samples[i] = 1234;
    memcpy(received, samples, sizeof(samples)); received_len = sizeof(samples);
    assert(tud_audio_rx_done_post_read_cb(0, 196, 0, VK_EP_PLAYBACK, 1));
    assert(playback.count == 49);
    vk_set_mute(&state, true);
    int32_t output[96]; vk_playback_render(&playback, output, 48);
    assert(output[0] == 1234*65536 && playback.count == 1); /* physical mic mute does not mute speaker */
    r = request(8, AUDIO_CS_CTRL_SAM_FREQ, 0, AUDIO_CS_REQ_CUR, 4);
    assert(tud_audio_get_req_entity_cb(0, &r) && le32(control) == 48000);
    r = request(6, AUDIO_FU_CTRL_VOLUME, 0, AUDIO_CS_REQ_RANGE, 8);
    assert(tud_audio_get_req_entity_cb(0, &r) && control_len == 8);
    r.bRequest = AUDIO_CS_REQ_CUR; r.wLength = 2; uint8_t volume[] = {0, 0xfa}; /* -6dB */
    assert(tud_audio_set_req_entity_cb(0, &r, volume) && playback.volume_db256 == -6*256);
    volume[1] = 1; assert(!tud_audio_set_req_entity_cb(0, &r, volume));
    r = request(6, AUDIO_FU_CTRL_MUTE, 0, AUDIO_CS_REQ_CUR, 1); uint8_t mute = 1;
    assert(tud_audio_set_req_entity_cb(0, &r, &mute));
    vk_playback_render(&playback, output, 48); for (unsigned i=0;i<96;++i) assert(output[i] == 0);
    memcpy(received, samples, sizeof(samples)); received_len = sizeof(samples);
    assert(tud_audio_rx_done_post_read_cb(0, 195, 0, VK_EP_PLAYBACK, 1) && !received_len && !playback.count);
    tud_suspend_cb(false); assert(!playback.active); tud_resume_cb(); assert(playback.active && !playback.count);
    r = (tusb_control_request_t){.wIndex = VK_ITF_AUDIO_PLAYBACK, .wValue = 0};
    assert(tud_audio_set_itf_close_EP_cb(0, &r)); assert(tud_audio_set_itf_cb(0, &r) && !playback.active);
    r.wValue = 2; assert(!tud_audio_set_itf_cb(0, &r));
    assert(CFG_TUD_HID_EP_BUFSIZE >= VK_DIAGNOSTIC_SIZE);
    audio_feedback_params_t params; memset(&params, 0xff, sizeof(params));
    tud_audio_feedback_params_cb(0, 1, &params); assert(params.method == AUDIO_FEEDBACK_METHOD_DISABLED && params.sample_freq == 48000);
    uint8_t feature[VK_DIAGNOSTIC_SIZE+16]; memset(feature, 0, sizeof(feature));
    assert(tud_hid_get_report_cb(0, 0, HID_REPORT_TYPE_FEATURE, feature, sizeof(feature)) == VK_DIAGNOSTIC_SIZE && feature[VK_DIAGNOSTIC_SIZE-1] == 0x5a && feature[VK_DIAGNOSTIC_SIZE] == 0);
    assert(tud_hid_get_report_cb(0, 0, HID_REPORT_TYPE_FEATURE, feature, 7) == 7);
    vk_set_mute(&state, false); tud_umount_cb(); tud_mount_cb();
}
static void check_task_feedback(void) {
    ready = false;
    int16_t input[VK_PLAY_TARGET * 2]; for (unsigned i=0;i<VK_PLAY_TARGET*2;++i) input[i]=100;
    int32_t output[100];
    for (int drift=-1;drift<=1;drift+=2) {
        vk_playback_init(&playback); consumed_frames = 0;
        tusb_control_request_t r = {.wIndex=VK_ITF_AUDIO_PLAYBACK,.wValue=1};
        unsigned before = feedback_submissions;
        assert(tud_audio_set_itf_cb(0,&r)); assert(feedback_submissions == before);
        uint64_t start = drift == -1 ? 10000 : 30000;
        run_at(start); uint32_t empty_feedback = feedback_value;
        vk_playback_push(&playback,input,VK_PLAY_TARGET);
        run_at(start+1); assert(feedback_value != empty_feedback);
        double device_fraction=0, host_fraction=0; uint32_t device_frames=0;
        for (unsigned ms=2;ms<=10000;++ms) {
            before = feedback_submissions; run_at(start+ms);
            assert(feedback_submissions == before+1); /* deleting periodic submission must fail */
            assert(feedback_value >= 47.75*65536 && feedback_value <= 48.25*65536);
            host_fraction += feedback_value / 65536.0;
            unsigned host=(unsigned)host_fraction; host_fraction-=host; assert(host<=49);
            received_len=host*4; memcpy(received,input,received_len);
            assert(tud_audio_rx_done_post_read_cb(0,received_len,0,VK_EP_PLAYBACK,1));
            device_fraction += 48*(1+drift*0.001); unsigned frames=(unsigned)device_fraction; device_fraction-=frames;
            vk_playback_render(&playback,output,frames); device_frames+=frames;
            consumed_frames=(device_frames/48)*48;
            assert(playback.count < VK_PLAY_FRAMES);
        }
        assert(playback.dropped==0 && playback.missing==0);
        assert(playback.measured_rate > 47.9 && playback.measured_rate < 48.1);
        r.wValue=0; assert(tud_audio_set_itf_cb(0,&r));
        before=feedback_submissions; run_at(start+10001); assert(feedback_submissions==before);
    }
    ready = true;
}

int main(void) {
    vk_playback_init(&playback);
    vk_init(&state, 2000); vk_set_audio_ok(&state, true);
    const vk_usb_hooks_t hooks = {.audio_packet=audio_packet, .connected=connected, .streaming=streaming, .host_mute=host_mute,
        .playback_active=playback_active, .playback_packet=playback_packet, .playback_control=playback_control,
        .feedback=feedback, .diagnostics=diagnostics, .hid_next=hid_next, .hid_commit=hid_commit};
    assert(vk_usb_start(NULL) == ESP_ERR_INVALID_ARG);
    assert(vk_usb_start(&hooks) == ESP_OK && task);
    tud_mount_cb();
    check_controls(); check_audio_lifecycle(); check_playback(); check_hid_delivery(); check_task_feedback();
    puts("PASS: actual USB callbacks + core: controls, mute, stream/suspend/resume, HID transport failures and expiration");
}
