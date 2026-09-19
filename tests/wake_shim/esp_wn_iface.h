#pragma once
#include <stddef.h>
typedef struct { int identity; } model_iface_data_t;
enum { DET_MODE_90, WAKENET_DETECTED=1 };
typedef struct {
 model_iface_data_t *(*create)(const void *,int);
 void (*destroy)(model_iface_data_t *);
 void (*clean)(model_iface_data_t *);
 int (*detect)(model_iface_data_t *,int16_t *);
 int (*get_samp_rate)(model_iface_data_t *);
 int (*get_channel_num)(model_iface_data_t *);
 int (*get_samp_chunksize)(model_iface_data_t *);
 const char *(*get_word_name)(model_iface_data_t *,int);
} esp_wn_iface_t;
