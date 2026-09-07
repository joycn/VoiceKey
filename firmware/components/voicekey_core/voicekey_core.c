#include "voicekey_core.h"
#include <limits.h>
#include <string.h>

void vk_fifo_init(vk_fifo_t *q, int16_t *storage, size_t capacity) {
    *q = (vk_fifo_t){.data = storage, .capacity = capacity};
}
void vk_fifo_clear(vk_fifo_t *q) { q->head = q->count = 0; }
size_t vk_fifo_push(vk_fifo_t *q, const int16_t *samples, size_t count) {
    size_t dropped = 0;
    if (!q->capacity) { q->dropped += count; return count; }
    if (count >= q->capacity) {
        dropped = q->count + count - q->capacity;
        samples += count - q->capacity;
        count = q->capacity;
        vk_fifo_clear(q);
    } else if (q->count + count > q->capacity) {
        dropped = q->count + count - q->capacity;
        q->head = (q->head + dropped) % q->capacity;
        q->count -= dropped;
    }
    size_t tail = (q->head + q->count) % q->capacity;
    size_t first = count < q->capacity - tail ? count : q->capacity - tail;
    memcpy(q->data + tail, samples, first * sizeof(*samples));
    memcpy(q->data, samples + first, (count - first) * sizeof(*samples));
    q->count += count;
    q->dropped += dropped;
    return dropped;
}
size_t vk_fifo_pop(vk_fifo_t *q, int16_t *out, size_t count) {
    size_t got = count < q->count ? count : q->count;
    if (!got) return 0;
    size_t first = got < q->capacity - q->head ? got : q->capacity - q->head;
    memcpy(out, q->data + q->head, first * sizeof(*out));
    memcpy(out + first, q->data, (got - first) * sizeof(*out));
    q->head = (q->head + got) % q->capacity;
    q->count -= got;
    return got;
}
int16_t vk_pcm_convert(int32_t sample, unsigned gain) {
    /* Avoid implementation-defined negative right shifts and signed overflow. */
    int64_t v = (int64_t)sample * gain / 65536;
    if (v > INT16_MAX) v = INT16_MAX;
    if (v < INT16_MIN) v = INT16_MIN;
    return (int16_t)v;
}
static void invalidate(vk_state_t *s) {
    s->audio_epoch++;
    s->wake_epoch++;
    vk_fifo_clear(&s->usb);
    vk_fifo_clear(&s->wake);
    s->pending = false;
    s->led_until_ms = 0;
}
void vk_init(vk_state_t *s, uint32_t cooldown_ms) {
    memset(s, 0, sizeof(*s));
    vk_fifo_init(&s->usb, s->usb_storage, VK_USB_CAPACITY);
    vk_fifo_init(&s->wake, s->wake_storage, VK_WAKE_CAPACITY);
    s->cooldown_ms = cooldown_ms;
    s->neutral_needed = true;
}
void vk_set_mute(vk_state_t *s, bool muted) {
    if (s->muted != muted) { s->muted = muted; invalidate(s); }
}
void vk_set_audio_ok(vk_state_t *s, bool ok) {
    if (s->audio_ok != ok) { s->audio_ok = ok; invalidate(s); }
}
void vk_set_connected(vk_state_t *s, bool connected) {
    if (s->connected == connected) return;
    s->connected = connected;
    s->streaming = false;
    s->key_down = false;
    s->neutral_needed = true;
    invalidate(s);
}
void vk_set_streaming(vk_state_t *s, bool streaming) {
    if (s->streaming != streaming) {
        s->streaming = streaming;
        vk_fifo_clear(&s->usb);
    }
}
void vk_set_host_mute(vk_state_t *s, bool muted) {
    s->host_mute = muted;
    vk_fifo_clear(&s->usb);
}
void vk_publish(vk_state_t *s, const int16_t *samples, size_t count, uint32_t epoch) {
    if (s->muted || !s->audio_ok || epoch != s->audio_epoch) return;
    if (s->streaming && s->connected && !s->host_mute) vk_fifo_push(&s->usb, samples, count);
    if (vk_fifo_push(&s->wake, samples, count)) s->wake_epoch++;
}
size_t vk_usb_packet_samples(const vk_state_t *s) {
    if (s->muted || s->host_mute || !s->audio_ok) return 16;
    /* Async endpoint permits +/- one sample around 16/ms for clock drift. */
    if (s->usb.count > 320) return 17;
    if (s->usb.count < 32) return 15;
    return 16;
}
size_t vk_usb_read(vk_state_t *s, int16_t *out, size_t count) {
    memset(out, 0, count * sizeof(*out));
    if (s->muted || s->host_mute || !s->audio_ok || !s->streaming || !s->connected) return 0;
    size_t got = vk_fifo_pop(&s->usb, out, count);
    s->usb.missing += count - got;
    return got;
}
size_t vk_wake_read(vk_state_t *s, int16_t *out, size_t count, uint32_t *epoch) {
    *epoch = s->wake_epoch;
    if (s->muted || !s->audio_ok || s->wake.count < count) return 0;
    return vk_fifo_pop(&s->wake, out, count);
}
bool vk_request_trigger(vk_state_t *s, uint64_t now, uint32_t epoch) {
    if (!s->connected || s->muted || !s->audio_ok || epoch != s->wake_epoch ||
        s->pending || s->key_down || (s->triggered_before && now - s->last_trigger_ms < s->cooldown_ms)) {
        s->suppressed++;
        return false;
    }
    s->pending = true;
    s->requested_ms = now;
    s->led_until_ms = now + VK_LED_MS;
    return true;
}
vk_hid_action_t vk_hid_next(vk_state_t *s, uint64_t now) {
    if (!s->connected) return VK_HID_NONE;
    if (s->neutral_needed) return VK_HID_RELEASE;
    if (s->key_down) {
        if (s->muted || !s->audio_ok || now - s->pressed_ms >= VK_HID_HOLD_MS) return VK_HID_RELEASE;
        return VK_HID_NONE;
    }
    if (s->muted || !s->audio_ok || (s->pending && now - s->requested_ms > VK_TRIGGER_TTL_MS)) s->pending = false;
    return s->pending ? VK_HID_PRESS : VK_HID_NONE;
}
void vk_hid_commit(vk_state_t *s, vk_hid_action_t action, uint64_t now) {
    if (action == VK_HID_RELEASE) {
        if (s->key_down && !s->muted && s->audio_ok) s->led_until_ms = now + VK_LED_MS;
        s->key_down = false;
        s->neutral_needed = false;
    }
    if (action == VK_HID_PRESS) {
        s->key_down = true;
        s->pending = false;
        s->pressed_ms = s->last_trigger_ms = now;
        s->triggered_before = true;
        s->triggers++;
    }
}
