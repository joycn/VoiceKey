#include "voicekey_audio.h"
#include <math.h>
#include <string.h>
const float vk_fir_coefficients[VK_FIR_TAPS] = {
#include "fir_coefficients.inc"
};
void vk_decimator_reset(vk_decimator_t *d) { memset(d, 0, sizeof(*d)); }
size_t vk_decimate(vk_decimator_t *d, const int32_t *stereo, size_t frames, int16_t *out, unsigned gain) {
    size_t n = 0;
    for (size_t i = 0; i < frames; ++i) {
        d->history[d->head] = (float)stereo[2 * i] / 65536.0f;
        d->head = (d->head + 1) % VK_FIR_TAPS;
        if (++d->phase != 3) continue;
        d->phase = 0;
        float value = 0;
        size_t j = d->head;
        for (size_t k = 0; k < VK_FIR_TAPS; ++k) {
            j = j ? j - 1 : VK_FIR_TAPS - 1;
            value += d->history[j] * vk_fir_coefficients[k];
        }
        value *= gain;
        if (value > 32767) value = 32767;
        if (value < -32768) value = -32768;
        out[n++] = (int16_t)value;
    }
    return n;
}
void vk_playback_init(vk_playback_t *p) { memset(p, 0, sizeof(*p)); p->gain_q16 = 65536; p->measured_rate = 48; }
void vk_playback_active(vk_playback_t *p, bool active) {
    p->active = active; p->head = p->count = 0; p->epoch++;
    p->feedback_last_us = 0; p->measured_rate = 48;
}
void vk_playback_control(vk_playback_t *p, bool mute, int16_t volume) {
    p->muted = mute; p->volume_db256 = volume;
    p->gain_q16 = (uint32_t)(powf(10, (float)volume / (256 * 20)) * 65536);
}
void vk_playback_push(vk_playback_t *p, const int16_t *samples, size_t frames) {
    if (!p->active) return;
    if (frames > VK_PLAY_FRAMES) { p->dropped += frames - VK_PLAY_FRAMES; samples += 2 * (frames - VK_PLAY_FRAMES); frames = VK_PLAY_FRAMES; }
    if (p->count + frames > VK_PLAY_FRAMES) {
        /* A host stall or overflow discards the entire stale backlog. */
        p->dropped += p->count; p->head = p->count = 0;
    }
    for (size_t i = 0; i < frames; ++i) {
        size_t j = (p->head + p->count) % VK_PLAY_FRAMES;
        p->samples[2*j] = samples[2*i]; p->samples[2*j+1] = samples[2*i+1]; p->count++;
    }
}
void vk_playback_render(vk_playback_t *p, int32_t *stereo, size_t frames) {
    memset(stereo, 0, frames * 2 * sizeof(*stereo));
    if (!p->active) return;
    size_t got = p->count < frames ? p->count : frames;
    for (size_t i = 0; i < got; ++i) {
        if (!p->muted) for (size_t ch = 0; ch < 2; ++ch) {
            int64_t s = (int64_t)p->samples[2 * p->head + ch] * p->gain_q16;
            if (s > INT32_MAX) s = INT32_MAX;
            if (s < INT32_MIN) s = INT32_MIN;
            stereo[2*i+ch] = (int32_t)s;
        }
        p->head = (p->head + 1) % VK_PLAY_FRAMES; p->count--;
    }
    p->missing += frames - got;
    if (got < frames) p->head = p->count = 0;
}
uint32_t vk_playback_feedback(vk_playback_t *p, uint32_t consumed, uint64_t now) {
    if (!p->feedback_last_us) { p->feedback_last_us = now; p->feedback_last_consumed = consumed; }
    uint64_t elapsed = now - p->feedback_last_us;
    if (elapsed >= 128000) {
        double rate = (uint32_t)(consumed - p->feedback_last_consumed) * 1000.0 / elapsed;
        /* 128ms averages 48-frame DMA quantization while retaining real drift.
           Reject a stopped clock or scheduling outlier; bounded correction. */
        if (rate >= 47 && rate <= 49) p->measured_rate += (rate - p->measured_rate) / 8;
        p->feedback_last_us = now; p->feedback_last_consumed = consumed;
    }
    double adjust = ((double)VK_PLAY_TARGET - p->count) / 4096;
    if (adjust > 0.125) adjust = 0.125;
    if (adjust < -0.125) adjust = -0.125;
    double rate = p->measured_rate + adjust;
    if (rate < 47.75) rate = 47.75;
    if (rate > 48.25) rate = 48.25;
    return (uint32_t)(rate * 65536); /* TinyUSB API takes 16.16, emits FS 10.14. */
}
