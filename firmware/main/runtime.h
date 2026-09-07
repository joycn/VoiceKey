#pragma once
#include "voicekey_core.h"
#include "esp_err.h"
void vk_runtime_init(void);
void vk_runtime_inputs(void);
uint32_t vk_runtime_audio_epoch(void);
void vk_runtime_audio_ok(bool ok);
void vk_runtime_publish(const int16_t *samples, size_t count, uint32_t epoch);
size_t vk_runtime_wake_read(int16_t *samples, size_t count, uint32_t *epoch);
void vk_runtime_trigger(uint32_t epoch);
void vk_runtime_ui(bool wake_ok);
void vk_runtime_log(void);
esp_err_t vk_runtime_usb_start(void);
esp_err_t vk_audio_start(void);
esp_err_t vk_wake_start(void);
