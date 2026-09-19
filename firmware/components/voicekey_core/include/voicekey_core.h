#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define VK_RATE 16000u
#define VK_USB_CAPACITY 512u
#define VK_WAKE_CAPACITY 4096u
#define VK_HID_HOLD_MS 30u
#define VK_TRIGGER_TTL_MS 200u
#define VK_LED_MS 5000u
#define VK_TRIGGER_KEY 0x16u /* USB HID S */
#define VK_TRIGGER_MODIFIERS 0x0eu /* left Shift | Alt (Option) | GUI (Command) */

typedef struct {
    int16_t *data;
    size_t capacity, head, count;
    uint64_t dropped, missing;
} vk_fifo_t;
void vk_fifo_init(vk_fifo_t *q, int16_t *storage, size_t capacity);
void vk_fifo_clear(vk_fifo_t *q);
size_t vk_fifo_push(vk_fifo_t *q, const int16_t *samples, size_t count);
size_t vk_fifo_pop(vk_fifo_t *q, int16_t *out, size_t count);
int16_t vk_pcm_convert(int32_t sample, unsigned gain);

typedef enum { VK_HID_NONE, VK_HID_RELEASE, VK_HID_PRESS } vk_hid_action_t;
typedef struct {
    vk_fifo_t usb, wake;
    int16_t usb_storage[VK_USB_CAPACITY], wake_storage[VK_WAKE_CAPACITY];
    bool muted, audio_ok, connected, streaming, host_mute;
    bool pending, key_down, neutral_needed, triggered_before;
    uint32_t audio_epoch, wake_epoch, cooldown_ms;
    uint64_t requested_ms, last_trigger_ms, pressed_ms, led_until_ms;
    uint64_t triggers, suppressed;
} vk_state_t;

/* Caller serializes every access. Expensive I/O and inference stay outside lock. */
void vk_init(vk_state_t *s, uint32_t cooldown_ms);
void vk_set_mute(vk_state_t *s, bool muted);
void vk_set_audio_ok(vk_state_t *s, bool ok);
void vk_set_connected(vk_state_t *s, bool connected);
void vk_set_streaming(vk_state_t *s, bool streaming);
void vk_set_host_mute(vk_state_t *s, bool muted);
void vk_publish(vk_state_t *s, const int16_t *samples, size_t count, uint32_t audio_epoch);
size_t vk_usb_read(vk_state_t *s, int16_t *samples, size_t count);
size_t vk_usb_packet_samples(const vk_state_t *s);
size_t vk_wake_read(vk_state_t *s, int16_t *samples, size_t count, uint32_t *epoch);
bool vk_request_trigger(vk_state_t *s, uint64_t now_ms, uint32_t wake_epoch);
vk_hid_action_t vk_hid_next(vk_state_t *s, uint64_t now_ms);
void vk_hid_commit(vk_state_t *s, vk_hid_action_t action, uint64_t now_ms);
