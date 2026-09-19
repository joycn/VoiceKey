#include "runtime.h"
#include <string.h>
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "model_path.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
static const esp_wn_iface_t *wn;
static model_iface_data_t *model;
static srmodel_list_t *models;
static int16_t *frame;
static int chunk;
static const char *model_name;
static void detect_task(void *arg) {
    (void)arg;
    uint32_t previous = UINT32_MAX;
    bool history = false;
    for (;;) {
        uint32_t epoch;
        size_t n = vk_runtime_wake_read(frame, chunk, &epoch);
        if (!n) { vTaskDelay(pdMS_TO_TICKS(5)); continue; }
        /* ESP-SR 2.2.0 wn9_hiesp clean() crashes in model_clean on this
           target. A fresh instance is the reset boundary; never reuse audio
           history across mute/fault epochs, and never clean a fresh model. */
        if (history && epoch != previous) {
            if (model) wn->destroy(model);
            model = wn->create(model_name, DET_MODE_90);
            if (!model) {
                ESP_LOGE("wake", "Model recreation failed; dropping frame");
                vTaskDelay(pdMS_TO_TICKS(100));
                continue;
            }
            history = false;
        }
        previous = epoch;
        history = true;
        uint16_t peak = 0;
        for (int i=0; i<chunk; ++i) {
            uint16_t magnitude = frame[i] < 0 ? (uint16_t)(-(int32_t)frame[i]) : (uint16_t)frame[i];
            if (magnitude > peak) peak = magnitude;
        }
        bool detected = wn->detect(model, frame) == WAKENET_DETECTED;
        vk_runtime_wake_observed(epoch, peak, detected);
        if (detected) vk_runtime_trigger(epoch);
    }
}
esp_err_t vk_wake_start(void) {
    models = esp_srmodel_init("model");
    if (!models) return ESP_FAIL;
    char *name = esp_srmodel_filter(models, ESP_WN_PREFIX, "nihaoxiaozhi_tts");
    if (!name || strcmp(name, "wn9_nihaoxiaozhi_tts") != 0) return ESP_ERR_NOT_FOUND;
    model_name = name;
    wn = esp_wn_handle_from_name(name);
    if (!wn) return ESP_ERR_NOT_SUPPORTED;
    model = wn->create(name, DET_MODE_90);
    if (!model) return ESP_ERR_NO_MEM;
    if (wn->get_samp_rate(model) != VK_RATE || wn->get_channel_num(model) != 1) return ESP_ERR_NOT_SUPPORTED;
    chunk = wn->get_samp_chunksize(model);
    if (chunk <= 0 || (unsigned)chunk > VK_WAKE_CAPACITY) return ESP_ERR_INVALID_SIZE;
    frame = heap_caps_malloc(chunk * sizeof(*frame), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!frame) return ESP_ERR_NO_MEM;
    ESP_LOGI("wake", "model=%s word=%s samples=%d", name, wn->get_word_name(model, 1), chunk);
    return xTaskCreatePinnedToCore(detect_task, "wakenet", 8192, NULL, 4, NULL, 1) == pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}
