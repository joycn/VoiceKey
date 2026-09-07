#pragma once
#include <stdint.h>
enum { VK_ITF_AUDIO_CONTROL, VK_ITF_AUDIO_STREAM, VK_ITF_HID, VK_ITF_COUNT };
#define VK_EP_AUDIO 0x81
#define VK_EP_HID 0x82
extern const uint8_t vk_configuration_descriptor[];
extern const uint8_t vk_hid_descriptor[];
