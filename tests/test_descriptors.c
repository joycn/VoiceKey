#include "tusb.h"
#include "voicekey_descriptors.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned u16(const uint8_t *p) { return p[0] | (p[1] << 8); }
static void check_string(uint8_t id, const char *wanted) {
    const uint16_t *s = tud_descriptor_string_cb(id, 0x0409);
    assert(s && (s[0] & 255) == strlen(wanted) * 2 + 2);
    for (size_t i=0;i<strlen(wanted);i++) assert(s[i+1] == (uint8_t)wanted[i]);
}
int main(void) {
    const tusb_desc_device_t *device = (const void *)tud_descriptor_device_cb();
    assert(device->bLength == 18 && device->bNumConfigurations == 1);
    assert(device->bDeviceClass == TUSB_CLASS_MISC && device->bcdUSB == 0x0200);
    check_string(device->iProduct, "VoiceKey Microphone");
    check_string(4, "VoiceKey");
    assert(tud_descriptor_string_cb(255, 0) == NULL);
    assert(tud_descriptor_configuration_cb(1) == NULL);
    const uint8_t *data = tud_descriptor_configuration_cb(0);
    unsigned total = u16(data+2), pos = 0, endpoints = 0, seen_itf = 0, audio_alts = 0;
    unsigned iid = 255, alt = 255, ac_start = 0, ac_end = 0, feature = 0, format = 0;
    while (pos < total) {
        const uint8_t *d = data + pos;
        assert(d[0] >= 2 && pos + d[0] <= total);
        if (d[1] == TUSB_DESC_CONFIGURATION) {
            assert(d[4] == 3 && !(d[7] & TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP));
        } else if (d[1] == TUSB_DESC_INTERFACE_ASSOCIATION) {
            assert(d[2] == 0 && d[3] == 2 && d[4] == TUSB_CLASS_AUDIO);
        } else if (d[1] == TUSB_DESC_INTERFACE) {
            iid=d[2]; alt=d[3]; seen_itf |= 1u << iid;
            if (iid == VK_ITF_AUDIO_STREAM) {
                audio_alts |= 1u << alt;
                assert((alt == 0 && d[4] == 0) || (alt == 1 && d[4] == 1));
            }
            if (iid == VK_ITF_HID) assert(d[5] == TUSB_CLASS_HID && d[7] == HID_ITF_PROTOCOL_KEYBOARD);
        } else if (d[1] == TUSB_DESC_ENDPOINT) {
            if (d[2] == VK_EP_AUDIO) {
                assert(iid == VK_ITF_AUDIO_STREAM && alt == 1);
                assert((d[3] & 3) == TUSB_XFER_ISOCHRONOUS && (d[3] & 0x0c) == TUSB_ISO_EP_ATT_ASYNCHRONOUS);
                assert(u16(d+4) == 34 && d[6] == 1); endpoints |= 1;
            } else {
                assert(d[2] == VK_EP_HID && iid == VK_ITF_HID);
                assert((d[3] & 3) == TUSB_XFER_INTERRUPT && u16(d+4) == 8); endpoints |= 2;
            }
        } else if (d[1] == TUSB_DESC_CS_INTERFACE) {
            if (iid == VK_ITF_AUDIO_CONTROL) {
                if (d[2] == AUDIO_CS_AC_INTERFACE_HEADER) { ac_start=pos; ac_end=pos+u16(d+6); }
                if (d[2] == AUDIO_CS_AC_INTERFACE_FEATURE_UNIT) {
                    assert(d[3] == 2 && d[4] == 1);
                    assert(d[5] == 3 && d[6] == 0 && d[9] == 0); feature++;
                }
            }
            if (iid == VK_ITF_AUDIO_STREAM && d[2] == AUDIO_CS_AS_INTERFACE_FORMAT_TYPE) {
                assert(d[3] == AUDIO_FORMAT_TYPE_I && d[4] == 2 && d[5] == 16); format++;
            }
        }
        pos += d[0];
        if (ac_start && pos == ac_end) assert(data[pos+1] == TUSB_DESC_INTERFACE);
    }
    assert(pos == total && seen_itf == 7 && audio_alts == 3 && endpoints == 3 && feature == 1 && format == 1);
    assert(ac_end > ac_start);
    assert(tud_hid_descriptor_report_cb(0) != NULL && tud_hid_descriptor_report_cb(1) == NULL);
    printf("PASS: UAC2 + HID descriptors (%u bytes), topology, strings, endpoints, controls and PCM format\n",total);
}
