#pragma once
#include "voicekey_core.h"
#define VK_FIR_TAPS 181u
#define VK_PLAY_FRAMES 960u
#define VK_PLAY_TARGET 240u
/* Float FIR operates on full 32-bit input before final saturating s16 conversion. */
typedef struct { float history[VK_FIR_TAPS]; size_t head; unsigned phase; } vk_decimator_t;
extern const float vk_fir_coefficients[VK_FIR_TAPS];
void vk_decimator_reset(vk_decimator_t *d);
size_t vk_decimate(vk_decimator_t *d, const int32_t *stereo, size_t frames, int16_t *out, unsigned gain);
typedef struct {
    int16_t samples[VK_PLAY_FRAMES * 2];
    size_t head, count;
    bool active, muted;
    int16_t volume_db256;
    uint32_t gain_q16;
    uint32_t epoch, dropped, missing, consumed;
    uint32_t feedback_last_consumed;
    uint64_t feedback_last_us;
    double measured_rate;
} vk_playback_t;
void vk_playback_init(vk_playback_t *p);
void vk_playback_active(vk_playback_t *p, bool active);
void vk_playback_control(vk_playback_t *p, bool mute, int16_t volume);
void vk_playback_push(vk_playback_t *p, const int16_t *samples, size_t frames);
void vk_playback_render(vk_playback_t *p, int32_t *stereo, size_t frames);
/* Consumption is DMA on_sent frames, including silence, not queue-pop count. */
uint32_t vk_playback_feedback(vk_playback_t *p, uint32_t consumed, uint64_t now_us);
