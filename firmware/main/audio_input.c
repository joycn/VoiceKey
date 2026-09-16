#include "runtime.h"
#include "board.h"
#include "voicekey_audio.h"
#include "driver/i2s_std.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "sdkconfig.h"
#include <stdatomic.h>
static i2s_chan_handle_t rx, tx;
static atomic_bool overflow;
static bool rx_overflow(i2s_chan_handle_t handle, i2s_event_data_t *event, void *ctx) {
    (void)handle; (void)event; (void)ctx;
    vk_runtime_capture_fault_isr(); atomic_store(&overflow, true); return false;
}
static const char *TAG = "audio";
static bool sent(i2s_chan_handle_t handle, i2s_event_data_t *event, void *ctx) {
    (void)handle; (void)ctx;
    /* Refill the just-consumed buffer. No TX software queue or repeated old DMA
       data: IDF clears before this callback, and every shortage renders zeros. */
    vk_runtime_playback_dma(event->dma_buf, event->size / 8);
    return false;
}
static void capture(void *arg) {
    (void)arg;
    int32_t stereo[144 * 2];
    int16_t mono[48];
    vk_decimator_t fir;
    vk_decimator_reset(&fir);
    bool healthy = false;
    uint32_t previous_epoch = UINT32_MAX, previous_generation = UINT32_MAX;
    for (;;) {
        if (atomic_exchange(&overflow, false)) {
            healthy = false; vk_runtime_audio_ok(false); vk_decimator_reset(&fir);
        }
        vk_runtime_inputs();
        uint32_t epoch = vk_runtime_audio_epoch();
        if (epoch != previous_epoch) {
            vk_decimator_reset(&fir);
            ESP_ERROR_CHECK(i2s_channel_disable(rx));
            ESP_ERROR_CHECK(i2s_channel_enable(rx));
            previous_epoch = epoch;
        }
        uint32_t generation = vk_runtime_capture_generation();
        if (generation != previous_generation) {
            healthy = false; vk_decimator_reset(&fir); previous_generation = generation;
        }
        size_t bytes = 0;
        esp_err_t err = i2s_channel_read(rx, stereo, sizeof(stereo), &bytes, 20);
        if (err != ESP_OK || bytes == 0 || bytes % 8) {
            healthy = false;
            vk_runtime_audio_ok(false);
            vk_decimator_reset(&fir);
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }
        if (atomic_load(&overflow) || generation != vk_runtime_capture_generation()) {
            healthy = false; vk_decimator_reset(&fir); continue;
        }
        if (!healthy) {
            healthy = vk_runtime_capture_recover(generation);
            vk_decimator_reset(&fir);
            continue; /* discard first block after input recovery */
        }
        if (atomic_load(&overflow) || vk_board_muted() || epoch != vk_runtime_audio_epoch()) {
            vk_decimator_reset(&fir); continue;
        }
        size_t n = vk_decimate(&fir, stereo, bytes / 8, mono, CONFIG_VOICEKEY_PCM_GAIN);
        vk_runtime_publish(mono, n, epoch);
    }
}
esp_err_t vk_audio_start(void) {
    i2s_chan_config_t ch = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_SLAVE);
    ch.dma_desc_num = 3;
    ch.dma_frame_num = 48;
    ch.auto_clear_before_cb = true;
    ESP_RETURN_ON_ERROR(i2s_new_channel(&ch, &tx, &rx), TAG, "duplex channels");
    i2s_std_config_t cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(48000),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {.mclk = I2S_GPIO_UNUSED, .bclk = VK_PIN_I2S_BCLK, .ws = VK_PIN_I2S_WS,
            .dout = VK_PIN_I2S_DOUT, .din = VK_PIN_I2S_DIN},
    };
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(tx, &cfg), TAG, "TX config");
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(rx, &cfg), TAG, "RX config");
    i2s_event_callbacks_t callbacks = {.on_sent = sent};
    ESP_RETURN_ON_ERROR(i2s_channel_register_event_callback(tx, &callbacks, NULL), TAG, "TX callback");
    i2s_event_callbacks_t rx_callbacks = {.on_recv_q_ovf = rx_overflow};
    ESP_RETURN_ON_ERROR(i2s_channel_register_event_callback(rx, &rx_callbacks, NULL), TAG, "RX callback");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(tx), TAG, "TX enable");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(rx), TAG, "RX enable");
    return xTaskCreatePinnedToCore(capture, "pcm", 6144, NULL, 8, NULL, 0) == pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}
