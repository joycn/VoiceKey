#include "tusb.h"
#include "voicekey_descriptors.h"
#include "sdkconfig.h"
#include <string.h>

const uint8_t vk_hid_descriptor[] = {TUD_HID_REPORT_DESC_KEYBOARD(),
    0x06, 0x00, 0xff, /* vendor usage page, feature report on EP0 */
    0x09, 0x01, 0xa1, 0x01, 0x15, 0x00, 0x26, 0xff, 0x00,
    0x75, 0x08, 0x95, VK_DIAGNOSTIC_SIZE, 0x09, 0x01, 0xb1, 0x02, 0xc0};
static const tusb_desc_device_t device = {
    .bLength = sizeof(tusb_desc_device_t), .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200, .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON, .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = CONFIG_VOICEKEY_USB_VID, .idProduct = CONFIG_VOICEKEY_USB_PID,
    .bcdDevice = 0x0001, .iManufacturer = 1, .iProduct = 2,
    .iSerialNumber = 0, /* No invented shared serial; reconnect same port during POC. */
    .bNumConfigurations = 1,
};
#define VK_CONFIG_LEN (TUD_CONFIG_DESC_LEN + VK_AUDIO_DESC_LEN + TUD_HID_DESC_LEN)
const uint8_t vk_configuration_descriptor[] = {
    TUD_CONFIG_DESCRIPTOR(1, VK_ITF_COUNT, 0, VK_CONFIG_LEN, 0, 500),
    /* UAC2 topology: clock 4 -> input 1 -> feature 2 -> output 3. */
    TUD_AUDIO_DESC_IAD(VK_ITF_AUDIO_CONTROL, 3, 3),
    TUD_AUDIO_DESC_STD_AC(VK_ITF_AUDIO_CONTROL, 0, 3),
    TUD_AUDIO_DESC_CS_AC(0x0200, AUDIO_FUNC_HEADSET,
        TUD_AUDIO_DESC_CLK_SRC_LEN + TUD_AUDIO_DESC_INPUT_TERM_LEN + TUD_AUDIO_DESC_OUTPUT_TERM_LEN +
        TUD_AUDIO_DESC_FEATURE_UNIT_ONE_CHANNEL_LEN + TUD_AUDIO_DESC_CLK_SRC_LEN +
        TUD_AUDIO_DESC_INPUT_TERM_LEN + TUD_AUDIO_DESC_OUTPUT_TERM_LEN + TUD_AUDIO_DESC_FEATURE_UNIT_TWO_CHANNEL_LEN, 0),
    TUD_AUDIO_DESC_CLK_SRC(4, AUDIO_CLOCK_SOURCE_ATT_INT_FIX_CLK,
        (AUDIO_CTRL_R << AUDIO_CLOCK_SOURCE_CTRL_CLK_FRQ_POS) |
        (AUDIO_CTRL_R << AUDIO_CLOCK_SOURCE_CTRL_CLK_VAL_POS), 1, 0),
    TUD_AUDIO_DESC_INPUT_TERM(1, AUDIO_TERM_TYPE_IN_GENERIC_MIC, 0, 4, 1,
        AUDIO_CHANNEL_CONFIG_NON_PREDEFINED, 0, AUDIO_CTRL_R << AUDIO_IN_TERM_CTRL_CONNECTOR_POS, 0),
    TUD_AUDIO_DESC_OUTPUT_TERM(3, AUDIO_TERM_TYPE_USB_STREAMING, 0, 2, 4, 0, 0),
    /* Only master mute is exposed. No nonfunctional host volume controls. */
    TUD_AUDIO_DESC_FEATURE_UNIT_ONE_CHANNEL(2, 1, AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_MUTE_POS, 0, 0),
    TUD_AUDIO_DESC_CLK_SRC(8, AUDIO_CLOCK_SOURCE_ATT_INT_FIX_CLK,
        (AUDIO_CTRL_R << AUDIO_CLOCK_SOURCE_CTRL_CLK_FRQ_POS) |
        (AUDIO_CTRL_R << AUDIO_CLOCK_SOURCE_CTRL_CLK_VAL_POS), 0, 0),
    TUD_AUDIO_DESC_INPUT_TERM(5, AUDIO_TERM_TYPE_USB_STREAMING, 0, 8, 2,
        AUDIO_CHANNEL_CONFIG_FRONT_LEFT | AUDIO_CHANNEL_CONFIG_FRONT_RIGHT, 0, 0, 0),
    TUD_AUDIO_DESC_FEATURE_UNIT_TWO_CHANNEL(6, 5,
        (AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_MUTE_POS) | (AUDIO_CTRL_RW << AUDIO_FEATURE_UNIT_CTRL_VOLUME_POS), 0, 0, 0),
    TUD_AUDIO_DESC_OUTPUT_TERM(7, AUDIO_TERM_TYPE_OUT_DESKTOP_SPEAKER, 0, 6, 8, 0, 0),
    TUD_AUDIO_DESC_STD_AS_INT(VK_ITF_AUDIO_STREAM, 0, 0, 3),
    TUD_AUDIO_DESC_STD_AS_INT(VK_ITF_AUDIO_STREAM, 1, 1, 3),
    TUD_AUDIO_DESC_CS_AS_INT(3, 0, AUDIO_FORMAT_TYPE_I, AUDIO_DATA_FORMAT_TYPE_I_PCM, 1,
        AUDIO_CHANNEL_CONFIG_NON_PREDEFINED, 0),
    TUD_AUDIO_DESC_TYPE_I_FORMAT(2, 16),
    TUD_AUDIO_DESC_STD_AS_ISO_EP(VK_EP_AUDIO,
        TUSB_XFER_ISOCHRONOUS | TUSB_ISO_EP_ATT_ASYNCHRONOUS | TUSB_ISO_EP_ATT_DATA, 34, 1),
    TUD_AUDIO_DESC_CS_AS_ISO_EP(AUDIO_CS_AS_ISO_DATA_EP_ATT_NON_MAX_PACKETS_OK,
        0, AUDIO_CS_AS_ISO_DATA_EP_LOCK_DELAY_UNIT_UNDEFINED, 0),
    TUD_AUDIO_DESC_STD_AS_INT(VK_ITF_AUDIO_PLAYBACK, 0, 0, 3),
    TUD_AUDIO_DESC_STD_AS_INT(VK_ITF_AUDIO_PLAYBACK, 1, 2, 3),
    TUD_AUDIO_DESC_CS_AS_INT(5, 0, AUDIO_FORMAT_TYPE_I, AUDIO_DATA_FORMAT_TYPE_I_PCM, 2,
        AUDIO_CHANNEL_CONFIG_FRONT_LEFT | AUDIO_CHANNEL_CONFIG_FRONT_RIGHT, 0),
    TUD_AUDIO_DESC_TYPE_I_FORMAT(2, 16),
    TUD_AUDIO_DESC_STD_AS_ISO_EP(VK_EP_PLAYBACK,
        TUSB_XFER_ISOCHRONOUS | TUSB_ISO_EP_ATT_ASYNCHRONOUS | TUSB_ISO_EP_ATT_DATA, 196, 1),
    TUD_AUDIO_DESC_CS_AS_ISO_EP(AUDIO_CS_AS_ISO_DATA_EP_ATT_NON_MAX_PACKETS_OK,
        0, AUDIO_CS_AS_ISO_DATA_EP_LOCK_DELAY_UNIT_UNDEFINED, 0),
    TUD_AUDIO_DESC_STD_AS_ISO_FB_EP(VK_EP_FEEDBACK, 3, 1),
    TUD_HID_DESCRIPTOR(VK_ITF_HID, 4, HID_ITF_PROTOCOL_KEYBOARD, sizeof(vk_hid_descriptor), VK_EP_HID, 8, 1),
};
_Static_assert(sizeof(vk_configuration_descriptor) == VK_CONFIG_LEN, "USB total length mismatch");
uint8_t const *tud_descriptor_device_cb(void) { return (const uint8_t *)&device; }
uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    return index == 0 ? vk_configuration_descriptor : NULL;
}
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
    return instance == 0 ? vk_hid_descriptor : NULL;
}
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;
    static uint16_t buffer[33];
    static const char *const strings[] = {NULL, "VoiceKey Prototype", "VoiceKey XVF3800 Audio",
        "VoiceKey XVF3800 Audio", "VoiceKey"};
    size_t count;
    if (index == 0) { buffer[1] = 0x0409; count = 1; }
    else {
        if (index >= sizeof(strings) / sizeof(strings[0])) return NULL;
        count = strlen(strings[index]);
        if (count > 32) count = 32;
        for (size_t i = 0; i < count; ++i) buffer[i + 1] = (uint8_t)strings[index][i];
    }
    buffer[0] = (TUSB_DESC_STRING << 8) | (2 * count + 2);
    return buffer;
}
