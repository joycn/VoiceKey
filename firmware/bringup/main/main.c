#include "runtime.h"
#include "board.h"
#include "esp_psram.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
static void stage(const char *name, esp_err_t result) {
    printf("BRINGUP %s: %s heap=%lu psram=%lu\n", name, esp_err_to_name(result),
        (unsigned long)esp_get_free_heap_size(), (unsigned long)esp_psram_get_size());
    fflush(stdout);
}
void app_main(void) {
    stage("app_main / PSRAM boot", ESP_OK);
    vTaskDelay(pdMS_TO_TICKS(5000));
    vk_runtime_init(); stage("runtime", ESP_OK);
    esp_err_t board=vk_board_init(); stage("board",board);
    vk_runtime_init_status(board,VK_INIT_PENDING,VK_INIT_PENDING,false);
    esp_err_t audio=board==ESP_OK ? vk_board_xmos_init() : board;
    stage("XMOS control task",audio);
    if(audio==ESP_OK) audio=vk_audio_start();
    stage("I2S start",audio);
    vTaskDelay(pdMS_TO_TICKS(2000));
    vk_runtime_init_status(board,audio,VK_INIT_PENDING,false);
    stage("before WakeNet",ESP_OK);
    esp_err_t wake=vk_wake_start(); stage("after WakeNet",wake);
    vk_runtime_init_status(board,audio,wake,true);
    for(int i=10;i>0;i--) {
        printf("BRINGUP USB takeover in %d seconds\n",i); fflush(stdout);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    /* Native USB PHY is shared: serial console disappears at this point. */
    esp_err_t usb=vk_runtime_usb_start();
    if(usb!=ESP_OK) stage("USB failed",usb);
    for(;;) {vk_runtime_inputs();vk_runtime_ui(vk_runtime_wake_ready());vTaskDelay(pdMS_TO_TICKS(5));}
}
