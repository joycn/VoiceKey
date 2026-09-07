#pragma once
#include <stdint.h>
int xTaskCreatePinnedToCore(void (*fn)(void *), const char *, uint32_t, void *, unsigned, void *, int);
