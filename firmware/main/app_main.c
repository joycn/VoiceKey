#include "runtime.h"
#include "board.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_psram.h"
#include <stdatomic.h>

static atomic_bool wake_ready;
static void status_task(void *arg) {
    (void)arg;
    int64_t next_log = 0;
    for (;;) {
        vk_runtime_ui(atomic_load(&wake_ready));
        int64_t now = esp_timer_get_time();
        if (now >= next_log) { vk_runtime_log(); next_log = now + 5000000; }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
void app_main(void) {
    vk_runtime_init();
    esp_err_t board = vk_board_init();
    if (board != ESP_OK) ESP_LOGE("voicekey", "Board init failed: %s", esp_err_to_name(board));
    vk_runtime_inputs();
    /* Mute indication must remain responsive during XMOS/model initialization. */
    if (xTaskCreatePinnedToCore(status_task, "status", 4096, NULL, 3, NULL, 0) != pdPASS)
        ESP_ERROR_CHECK(ESP_ERR_NO_MEM);
    ESP_ERROR_CHECK(vk_runtime_usb_start());
    esp_err_t audio = board == ESP_OK ? vk_board_xmos_init() : board;
    if (audio == ESP_OK) audio = vk_audio_start();
    if (audio != ESP_OK) ESP_LOGE("voicekey", "Audio unavailable; USB remains silent: %s", esp_err_to_name(audio));
    esp_err_t wake = vk_wake_start();
    atomic_store(&wake_ready, wake == ESP_OK);
    if (wake != ESP_OK) ESP_LOGE("voicekey", "Wake engine unavailable: %s", esp_err_to_name(wake));
    ESP_LOGI("voicekey", "PSRAM=%u bytes; UART0 diagnostics; no network services", (unsigned)esp_psram_get_size());
}
