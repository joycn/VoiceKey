#include "runtime.h"
#include "board.h"
#include "driver/i2s_std.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "sdkconfig.h"
static i2s_chan_handle_t rx;
static const char *TAG = "audio";
static void capture(void *arg) {
    (void)arg;
    int32_t stereo[160 * 2];
    int16_t mono[160];
    bool healthy = false;
    uint32_t previous_epoch = vk_runtime_audio_epoch();
    for (;;) {
        uint32_t epoch = vk_runtime_audio_epoch();
        if (epoch != previous_epoch) {
            /* IDF disable/enable resets the RX DMA message queue as well as our FIFOs. */
            ESP_ERROR_CHECK(i2s_channel_disable(rx));
            ESP_ERROR_CHECK(i2s_channel_enable(rx));
            previous_epoch = epoch;
        }
        size_t bytes = 0;
        esp_err_t err = i2s_channel_read(rx, stereo, sizeof(stereo), &bytes, 100);
        if (err != ESP_OK || bytes == 0 || bytes % 8) {
            if (healthy) ESP_LOGE(TAG, "PCM input lost: %s, bytes=%u", esp_err_to_name(err), (unsigned)bytes);
            healthy = false;
            vk_runtime_audio_ok(false);
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }
        if (!healthy) {
            healthy = true;
            vk_runtime_audio_ok(true);
            /* First block after recovery predates generation change; discard it. */
            continue;
        }
        size_t frames = bytes / 8;
        for (size_t i = 0; i < frames; ++i)
            mono[i] = vk_pcm_convert(stereo[2 * i + CONFIG_VOICEKEY_PCM_CHANNEL], CONFIG_VOICEKEY_PCM_GAIN);
        vk_runtime_publish(mono, frames, epoch);
    }
}
esp_err_t vk_audio_start(void) {
    i2s_chan_config_t ch = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_SLAVE);
    ch.dma_desc_num = 8;
    ch.dma_frame_num = 160;
    ESP_RETURN_ON_ERROR(i2s_new_channel(&ch, NULL, &rx), TAG, "RX channel");
    i2s_std_config_t cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(VK_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {.mclk = I2S_GPIO_UNUSED, .bclk = VK_PIN_I2S_BCLK, .ws = VK_PIN_I2S_WS,
            .dout = I2S_GPIO_UNUSED, .din = VK_PIN_I2S_DIN},
    };
    ESP_RETURN_ON_ERROR(i2s_channel_init_std_mode(rx, &cfg), TAG, "I2S config");
    ESP_RETURN_ON_ERROR(i2s_channel_enable(rx), TAG, "I2S enable");
    return xTaskCreatePinnedToCore(capture, "pcm", 4096, NULL, 8, NULL, 0) == pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}
