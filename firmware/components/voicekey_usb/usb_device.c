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
static bool streaming;
static uint8_t last_report[8];

static void usb_task(void *arg) {
    (void)arg;
    for (;;) {
        /* Bounded wait: HID release is serviced even without control traffic. */
        tud_task_ext(1, false);
        if (tud_hid_ready()) {
            uint64_t now = esp_timer_get_time() / 1000;
            vk_hid_action_t a = hooks.hid_next(now);
            uint8_t keys[6] = {0};
            if (a == VK_HID_PRESS) keys[0] = VK_KEY_F18;
            if (a != VK_HID_NONE && tud_hid_keyboard_report(0, 0, keys)) {
                memset(last_report, 0, sizeof(last_report));
                memcpy(last_report + 2, keys, sizeof(keys));
                hooks.hid_commit(a, now);
            }
        }
    }
}
esp_err_t vk_usb_start(const vk_usb_hooks_t *h) {
    if (!h || !h->audio_packet || !h->connected || !h->streaming || !h->host_mute || !h->hid_next || !h->hid_commit)
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
    streaming = false;
    mic_mute = 0;
    hooks.host_mute(false);
    hooks.connected(true);
}
void tud_umount_cb(void) { streaming = false; hooks.connected(false); }
void tud_suspend_cb(bool remote_wakeup_en) {
    (void)remote_wakeup_en;
    /* Preserve host alternate setting, discard pending data and keys. */
    hooks.connected(false);
    tud_audio_clear_ep_in_ff();
}
void tud_resume_cb(void) {
    hooks.connected(true);
    hooks.streaming(streaming);
}
bool tud_audio_set_itf_close_EP_cb(uint8_t rhport, tusb_control_request_t const *req) {
    (void)rhport;
    if ((req->wIndex & 0xff) == VK_ITF_AUDIO_STREAM) {
        hooks.streaming(false);
        tud_audio_clear_ep_in_ff();
    }
    return true;
}
bool tud_audio_set_itf_cb(uint8_t rhport, tusb_control_request_t const *req) {
    (void)rhport;
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
static bool reply(uint8_t port, tusb_control_request_t const *req, void *data, uint16_t size) {
    return tud_audio_buffer_and_schedule_control_xfer(port, req, data, size);
}
bool tud_audio_get_req_entity_cb(uint8_t port, tusb_control_request_t const *req) {
    if ((req->wIndex & 0xff) != VK_ITF_AUDIO_CONTROL) return false;
    uint8_t entity = req->wIndex >> 8, selector = req->wValue >> 8, channel = req->wValue & 0xff;
    if (channel != 0) return false;
    if (entity == 4) {
        if (selector == AUDIO_CS_CTRL_SAM_FREQ) {
            if (req->bRequest == AUDIO_CS_REQ_CUR) { uint32_t rate = VK_RATE; return reply(port, req, &rate, 4); }
            if (req->bRequest == AUDIO_CS_REQ_RANGE) {
                audio_control_range_4_n_t(1) r = {.wNumSubRanges = 1,
                    .subrange = {{.bMin = VK_RATE, .bMax = VK_RATE, .bRes = 0}}};
                return reply(port, req, &r, sizeof(r));
            }
        }
        if (selector == AUDIO_CS_CTRL_CLK_VALID && req->bRequest == AUDIO_CS_REQ_CUR) {
            uint8_t valid = 1; return reply(port, req, &valid, 1);
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
    if ((req->wIndex & 0xff) != VK_ITF_AUDIO_CONTROL || (req->wIndex >> 8) != 2 ||
        (req->wValue >> 8) != AUDIO_FU_CTRL_MUTE || (req->wValue & 0xff) != 0 ||
        req->bRequest != AUDIO_CS_REQ_CUR || req->wLength != 1 || data[0] > 1) return false;
    mic_mute = data[0];
    hooks.host_mute(mic_mute != 0);
    tud_audio_clear_ep_in_ff();
    return true;
}
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t type, uint8_t *buf, uint16_t len) {
    if (instance || report_id || type != HID_REPORT_TYPE_INPUT) return 0;
    size_t n = len < 8 ? len : 8;
    memcpy(buf, last_report, n);
    return n;
}
void tud_hid_set_report_cb(uint8_t instance, uint8_t id, hid_report_type_t type, uint8_t const *buf, uint16_t len) {
    (void)instance; (void)id; (void)type; (void)buf; (void)len; /* Keyboard LED output is not microphone mute. */
}
