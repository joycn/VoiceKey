#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#define VK_PIN_BUTTON 0
#define VK_PIN_I2C_SDA 5
#define VK_PIN_I2C_SCL 6
#define VK_PIN_I2S_BCLK 8
#define VK_PIN_I2S_WS 7
#define VK_PIN_I2S_DIN 43
#define VK_PIN_I2S_DOUT 44
typedef struct {
    bool valid, muted, i2s_active;
    uint8_t version[3];
    uint64_t updated_ms;
    uint32_t errors, busy;
} vk_board_status_t;
esp_err_t vk_board_init(void);
esp_err_t vk_board_xmos_init(void);
void vk_board_status(vk_board_status_t *status);
bool vk_board_muted(void);
bool vk_board_button(void);
void vk_board_led(uint8_t red, uint8_t green, uint8_t blue);
