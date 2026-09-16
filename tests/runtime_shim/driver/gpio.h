#pragma once
#include "esp_err.h"
#include <stdint.h>
typedef struct { uint64_t pin_bit_mask; int mode, pull_up_en; } gpio_config_t;
#define GPIO_MODE_INPUT 1
#define GPIO_PULLUP_ENABLE 1
esp_err_t gpio_config(const gpio_config_t *);
int gpio_get_level(int pin);
