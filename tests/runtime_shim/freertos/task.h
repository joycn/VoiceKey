#pragma once
#include "freertos/FreeRTOS.h"
int xTaskCreatePinnedToCore(void (*fn)(void *), const char *, uint32_t, void *, unsigned, void *, int);
TickType_t xTaskGetTickCount(void);
void vTaskDelay(TickType_t ticks);
void vTaskDelayUntil(TickType_t *previous, TickType_t period);
