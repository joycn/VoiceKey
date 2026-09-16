#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef void *i2s_chan_handle_t;
typedef struct { void *dma_buf; size_t size; } i2s_event_data_t;
typedef bool (*i2s_callback_t)(i2s_chan_handle_t, i2s_event_data_t *, void *);
typedef struct { i2s_callback_t on_sent, on_recv_q_ovf; } i2s_event_callbacks_t;
typedef struct { unsigned dma_desc_num, dma_frame_num; bool auto_clear_before_cb; } i2s_chan_config_t;
typedef struct { struct {unsigned rate;} clk_cfg; unsigned slot_cfg;
    struct { int mclk,bclk,ws,dout,din; } gpio_cfg; } i2s_std_config_t;
#define I2S_NUM_0 0
#define I2S_ROLE_SLAVE 0
#define I2S_GPIO_UNUSED -1
#define I2S_CHANNEL_DEFAULT_CONFIG(port, role) ((i2s_chan_config_t){0})
#define I2S_STD_CLK_DEFAULT_CONFIG(rate_) { .rate=(rate_) }
#define I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(bits, mode) 32
esp_err_t i2s_new_channel(const i2s_chan_config_t *, i2s_chan_handle_t *, i2s_chan_handle_t *);
esp_err_t i2s_channel_init_std_mode(i2s_chan_handle_t, const i2s_std_config_t *);
esp_err_t i2s_channel_register_event_callback(i2s_chan_handle_t,const i2s_event_callbacks_t *,void *);
esp_err_t i2s_channel_enable(i2s_chan_handle_t);
esp_err_t i2s_channel_disable(i2s_chan_handle_t);
esp_err_t i2s_channel_read(i2s_chan_handle_t,void *,size_t,size_t *,uint32_t);
