#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
#define VK_PIN_MUTE 3
#define VK_PIN_BUTTON 0
#define VK_PIN_XMOS_RESET 4
#define VK_PIN_I2C_SDA 5
#define VK_PIN_I2C_SCL 6
#define VK_PIN_I2S_REFERENCE 10
#define VK_PIN_I2S_BCLK 13
#define VK_PIN_I2S_WS 14
#define VK_PIN_I2S_DIN 15
#define VK_PIN_LED 21
#define VK_PIN_LED_POWER 45
#define VK_PIN_AMP_ENABLE 47
esp_err_t vk_board_init(void);
esp_err_t vk_board_xmos_init(void);
bool vk_board_muted(void);
bool vk_board_button(void);
void vk_board_led(uint8_t red, uint8_t green, uint8_t blue);
