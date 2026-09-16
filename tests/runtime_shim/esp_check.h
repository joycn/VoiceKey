#pragma once
#include "esp_err.h"
#include <assert.h>
#define ESP_RETURN_ON_ERROR(expression, tag, ...) do { (void)(tag); esp_err_t e_=(expression); if(e_ != ESP_OK) return e_; } while (0)
#define ESP_ERROR_CHECK(expression) assert((expression) == ESP_OK)
