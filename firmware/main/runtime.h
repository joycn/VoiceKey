#pragma once
#include "voicekey_core.h"
#include "esp_err.h"
#define VK_INIT_PENDING 0x80000000u
void vk_runtime_init(void);
void vk_runtime_inputs(void);
uint32_t vk_runtime_audio_epoch(void);
void vk_runtime_audio_ok(bool ok);
void vk_runtime_publish(const int16_t *samples, size_t count, uint32_t epoch);
size_t vk_runtime_wake_read(int16_t *samples, size_t count, uint32_t *epoch);
void vk_runtime_trigger(uint32_t epoch);
void vk_runtime_wake_observed(uint32_t epoch, uint16_t peak, bool detected);
void vk_runtime_ui(bool wake_ok);
void vk_runtime_log(void);
esp_err_t vk_runtime_usb_start(void);
esp_err_t vk_audio_start(void);
esp_err_t vk_wake_start(void);

/* I2S DMA callback: bounded, integer-only, ISR-safe. */
void vk_runtime_playback_dma(int32_t *stereo, size_t frames);

/* Overflow admission closes synchronously, including other-core inference. */
void vk_runtime_capture_fault_isr(void);
uint32_t vk_runtime_capture_generation(void);
bool vk_runtime_capture_recover(uint32_t generation);
void vk_runtime_init_status(uint32_t board, uint32_t audio, uint32_t wake, bool complete);
bool vk_runtime_wake_ready(void);
