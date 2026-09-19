#include "voicekey_usb.h"
#include "voicekey_descriptors.h"
#include "tusb.h"
#include "esp_private/usb_phy.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static vk_usb_hooks_t hooks;
static usb_phy_handle_t phy;
static uint8_t mic_mute;
static bool streaming, playing;
static uint8_t speaker_mute;
static int16_t speaker_volume;
static uint8_t last_report[8];

static void usb_task(void *arg) {
    (void)arg;
    for (;;) {
        /* Bounded wait: HID release is serviced even without control traffic. */
        tud_task_ext(1, false);
        if (playing) tud_audio_fb_set(hooks.feedback(esp_timer_get_time()));
        if (tud_hid_ready()) {
            uint64_t now = esp_timer_get_time() / 1000;
            vk_hid_action_t a = hooks.hid_next(now);
            uint8_t keys[6] = {0};
            uint8_t modifiers = 0;
            if (a == VK_HID_PRESS) { keys[0] = VK_TRIGGER_KEY; modifiers = VK_TRIGGER_MODIFIERS; }
            if (a != VK_HID_NONE && tud_hid_keyboard_report(0, modifiers, keys)) {
                memset(last_report, 0, sizeof(last_report));
                last_report[0] = modifiers;
                memcpy(last_report + 2, keys, sizeof(keys));
                hooks.hid_commit(a, now);
            }
        }
    }
}
esp_err_t vk_usb_start(const vk_usb_hooks_t *h) {
    if (!h || !h->audio_packet || !h->connected || !h->streaming || !h->host_mute || !h->hid_next || !h->hid_commit || !h->playback_active || !h->playback_packet ||
        !h->playback_control || !h->feedback || !h->diagnostics)
        return ESP_ERR_INVALID_ARG;
    hooks = *h;
    usb_phy_config_t cfg = {.controller = USB_PHY_CTRL_OTG, .target = USB_PHY_TARGET_INT,
        .otg_mode = USB_OTG_MODE_DEVICE, .otg_speed = USB_PHY_SPEED_FULL};
    esp_err_t err = usb_new_phy(&cfg, &phy);
    if (err != ESP_OK) return err;
    if (!tusb_init()) return ESP_FAIL;
    return xTaskCreatePinnedToCore(usb_task, "usb", 4096, NULL, 10, NULL, 0) == pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}
void tud_mount_cb(void) {
    memset(last_report, 0, sizeof(last_report));
    streaming = playing = false;
    speaker_mute = 0; speaker_volume = 0;
    hooks.playback_active(false); hooks.playback_control(false, 0);
    mic_mute = 0;
    hooks.host_mute(false);
    hooks.connected(true);
}
void tud_umount_cb(void) { streaming = playing = false; hooks.playback_active(false); hooks.connected(false); tud_audio_clear_ep_out_ff(); tud_audio_clear_ep_in_ff(); }
void tud_suspend_cb(bool remote_wakeup_en) {
    (void)remote_wakeup_en;
    /* Preserve host alternate setting, discard pending data and keys. */
    hooks.connected(false);
    hooks.playback_active(false); tud_audio_clear_ep_out_ff();
    tud_audio_clear_ep_in_ff();
}
void tud_resume_cb(void) {
    hooks.connected(true);
    hooks.streaming(streaming);
    hooks.playback_active(playing); tud_audio_clear_ep_out_ff();
}
bool tud_audio_set_itf_close_EP_cb(uint8_t rhport, tusb_control_request_t const *req) {
    (void)rhport;
    if ((req->wIndex & 0xff) == VK_ITF_AUDIO_STREAM) {
        hooks.streaming(false);
        tud_audio_clear_ep_in_ff();
    }
    if ((req->wIndex & 0xff) == VK_ITF_AUDIO_PLAYBACK) {
        hooks.playback_active(false); tud_audio_clear_ep_out_ff();
    }
    return true;
}
bool tud_audio_set_itf_cb(uint8_t rhport, tusb_control_request_t const *req) {
    (void)rhport;
    if ((req->wIndex & 0xff) == VK_ITF_AUDIO_PLAYBACK) {
        if (req->wValue > 1) return false;
        playing = req->wValue == 1;
        tud_audio_clear_ep_out_ff(); hooks.playback_active(playing);
        /* TinyUSB initializes feedback format after this callback returns.
           Only usb_task may submit, once set-interface processing completes. */
        return true;
    }
    if ((req->wIndex & 0xff) != VK_ITF_AUDIO_STREAM) return true;
    if (req->wValue > 1) return false;
    streaming = req->wValue == 1;
    tud_audio_clear_ep_in_ff();
    hooks.streaming(streaming);
    return true;
}
bool tud_audio_tx_done_pre_load_cb(uint8_t rhport, uint8_t func_id, uint8_t ep, uint8_t alt) {
    (void)rhport; (void)func_id; (void)ep;
    if (!alt) return true;
    int16_t samples[17] = {0};
    size_t n = hooks.audio_packet(samples);
    if (n > 17) n = 17;
    /* One packet only. No hidden multi-frame audio backlog across hardware mute. */
    tud_audio_clear_ep_in_ff();
    return tud_audio_write(samples, n * sizeof(*samples)) == n * sizeof(*samples);
}
bool tud_audio_rx_done_post_read_cb(uint8_t port, uint16_t bytes, uint8_t function, uint8_t ep, uint8_t alt) {
    (void)port;
    if (function || ep != VK_EP_PLAYBACK || !alt || !playing || bytes > 196 || bytes % 4) {
        tud_audio_clear_ep_out_ff(); return true;
    }
    int16_t samples[98];
    uint16_t got = tud_audio_read(samples, bytes);
    if (got != bytes) { tud_audio_clear_ep_out_ff(); return true; }
    hooks.playback_packet(samples, bytes / 4);
    return true;
}
void tud_audio_feedback_params_cb(uint8_t function, uint8_t alt, audio_feedback_params_t *params) {
    (void)function; (void)alt;
    memset(params, 0, sizeof(*params));
    params->method = AUDIO_FEEDBACK_METHOD_DISABLED; params->sample_freq = 48000;
}
static bool reply(uint8_t port, tusb_control_request_t const *req, void *data, uint16_t size) {
    return tud_audio_buffer_and_schedule_control_xfer(port, req, data, size);
}
bool tud_audio_get_req_entity_cb(uint8_t port, tusb_control_request_t const *req) {
    if ((req->wIndex & 0xff) != VK_ITF_AUDIO_CONTROL) return false;
    uint8_t entity = req->wIndex >> 8, selector = req->wValue >> 8, channel = req->wValue & 0xff;
    if (channel != 0) return false;
    if (entity == 4 || entity == 8) {
        uint32_t rate = entity == 4 ? VK_RATE : 48000;
        if (selector == AUDIO_CS_CTRL_SAM_FREQ) {
            if (req->bRequest == AUDIO_CS_REQ_CUR) { return reply(port, req, &rate, 4); }
            if (req->bRequest == AUDIO_CS_REQ_RANGE) {
                audio_control_range_4_n_t(1) r = {.wNumSubRanges = 1,
                    .subrange = {{.bMin = rate, .bMax = rate, .bRes = 0}}};
                return reply(port, req, &r, sizeof(r));
            }
        }
        if (selector == AUDIO_CS_CTRL_CLK_VALID && req->bRequest == AUDIO_CS_REQ_CUR) {
            uint8_t valid = 1; return reply(port, req, &valid, 1);
        }
    }
    if (entity == 6) {
        if (selector == AUDIO_FU_CTRL_MUTE && req->bRequest == AUDIO_CS_REQ_CUR)
            return reply(port, req, &speaker_mute, 1);
        if (selector == AUDIO_FU_CTRL_VOLUME) {
            if (req->bRequest == AUDIO_CS_REQ_CUR) return reply(port, req, &speaker_volume, 2);
            if (req->bRequest == AUDIO_CS_REQ_RANGE) {
                audio_control_range_2_n_t(1) range = {.wNumSubRanges = 1,
                    .subrange = {{.bMin = -60 * 256, .bMax = 0, .bRes = 256}}};
                return reply(port, req, &range, sizeof(range));
            }
        }
    }
    if (entity == 2 && selector == AUDIO_FU_CTRL_MUTE && req->bRequest == AUDIO_CS_REQ_CUR)
        return reply(port, req, &mic_mute, 1);
    if (entity == 1 && selector == AUDIO_TE_CTRL_CONNECTOR && req->bRequest == AUDIO_CS_REQ_CUR) {
        audio_desc_channel_cluster_t c = {.bNrChannels = 1, .bmChannelConfig = 0, .iChannelNames = 0};
        return reply(port, req, &c, sizeof(c));
    }
    return false;
}
bool tud_audio_set_req_entity_cb(uint8_t port, tusb_control_request_t const *req, uint8_t *data) {
    (void)port;
    if (!data || (req->wIndex & 0xff) != VK_ITF_AUDIO_CONTROL || (req->wValue & 0xff) != 0 ||
        req->bRequest != AUDIO_CS_REQ_CUR) return false;
    uint8_t entity = req->wIndex >> 8, selector = req->wValue >> 8;
    if (entity == 6) {
        if (selector == AUDIO_FU_CTRL_MUTE && req->wLength == 1 && data[0] <= 1) speaker_mute = data[0];
        else if (selector == AUDIO_FU_CTRL_VOLUME && req->wLength == 2) {
            int16_t v = (int16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8));
            if (v > 0 || v < -60 * 256 || v % 256) return false;
            speaker_volume = v;
        } else return false;
        hooks.playback_control(speaker_mute, speaker_volume); return true;
    }
    if (entity != 2 || selector != AUDIO_FU_CTRL_MUTE || req->wLength != 1 || data[0] > 1) return false;
    mic_mute = data[0];
    hooks.host_mute(mic_mute != 0);
    tud_audio_clear_ep_in_ff();
    return true;
}
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t type, uint8_t *buf, uint16_t len) {
    if (instance || report_id) return 0;
    if (type == HID_REPORT_TYPE_FEATURE) return hooks.diagnostics(buf, len < VK_DIAGNOSTIC_SIZE ? len : VK_DIAGNOSTIC_SIZE);
    if (type != HID_REPORT_TYPE_INPUT) return 0;
    size_t n = len < 8 ? len : 8;
    memcpy(buf, last_report, n);
    return n;
}
void tud_hid_set_report_cb(uint8_t instance, uint8_t id, hid_report_type_t type, uint8_t const *buf, uint16_t len) {
    (void)instance; (void)id; (void)type; (void)buf; (void)len; /* Keyboard LED output is not microphone mute. */
}
