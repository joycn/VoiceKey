#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "voicekey_core.h"

/* Runtime hooks: bounded, nonblocking; called by the USB task, not ISR. */
typedef struct {
    size_t (*audio_packet)(int16_t *samples);
    void (*connected)(bool value);
    void (*streaming)(bool value);
    void (*host_mute)(bool value);
    void (*playback_active)(bool value);
    void (*playback_packet)(const int16_t *samples, size_t frames);
    void (*playback_control)(bool mute, int16_t volume_db256);
    uint32_t (*feedback)(uint64_t now_us);
    size_t (*diagnostics)(uint8_t *buffer, size_t capacity);
    vk_hid_action_t (*hid_next)(uint64_t now_ms);
    void (*hid_commit)(vk_hid_action_t action, uint64_t now_ms);
} vk_usb_hooks_t;
esp_err_t vk_usb_start(const vk_usb_hooks_t *hooks);
