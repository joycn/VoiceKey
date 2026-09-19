/* Maintenance-only image: USB Serial/JTAG, internal RAM, read-only XMOS.
   No audio, model, TinyUSB, tuning writes, DAC writes or XMOS flashing. */
#include <stdio.h>
#include "driver/i2c_master.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
static i2c_master_dev_handle_t xmos;
static void query(const char *name, uint8_t resource, uint8_t command, size_t count) {
    uint8_t request[] = {resource, command | 0x80, count + 1}, response[51];
    for (unsigned attempt=0; attempt<3; ++attempt) {
        esp_err_t e=i2c_master_transmit(xmos,request,sizeof(request),20);
        if (e==ESP_OK) e=i2c_master_receive(xmos,response,count+1,20);
        if (e!=ESP_OK) { printf("%s: transport=%s\n",name,esp_err_to_name(e)); return; }
        if (response[0]==64) { vTaskDelay(pdMS_TO_TICKS(10)); continue; }
        printf("%s: status=%u",name,response[0]);
        if (response[0]==0) for(size_t i=1;i<=count;i++) printf(" %02x",response[i]);
        putchar('\n'); return;
    }
    printf("%s: busy retry limit\n",name);
}
void app_main(void) {
    esp_chip_info_t chip; esp_chip_info(&chip);
    uint32_t flash_size=0; esp_err_t flash=esp_flash_get_size(NULL,&flash_size);
    /* Give the host time to open the maintenance console after reboot. */
    vTaskDelay(pdMS_TO_TICKS(3000));
    i2c_master_bus_config_t cfg={.i2c_port=I2C_NUM_0,.sda_io_num=5,.scl_io_num=6,
        .clk_source=I2C_CLK_SRC_DEFAULT,.glitch_ignore_cnt=7,.flags.enable_internal_pullup=true};
    i2c_master_bus_handle_t bus;
    esp_err_t e=i2c_new_master_bus(&cfg,&bus);
    if(e==ESP_OK) {
        i2c_device_config_t dev={.dev_addr_length=I2C_ADDR_BIT_LEN_7,.device_address=0x2c,.scl_speed_hz=100000};
        e=i2c_master_bus_add_device(bus,&dev,&xmos);
    }
    for (;;) {
        printf("\nVoiceKey MAINTENANCE: reset=%d cores=%u revision=%u flash=%lu (%s) heap=%lu; PSRAM intentionally disabled\n",
            esp_reset_reason(),chip.cores,chip.revision,(unsigned long)flash_size,
            esp_err_to_name(flash),(unsigned long)esp_get_free_heap_size());
        if(e!=ESP_OK) printf("I2C init=%s\n",esp_err_to_name(e));
        else {
            query("VERSION",48,0,3); query("BLD_MSG",48,1,50);
            query("USB_BIT_DEPTH",48,8,2); query("GPO_READ_VALUES",20,0,5);
            query("I2S_INACTIVE",35,24,1); query("OP_L",35,15,2);
            query("OP_PACKED",35,13,2); query("OP_UPSAMPLE",35,14,2);
            query("AEC_CONVERGED",33,3,4); query("SHF_BYPASS",33,70,1);
            query("REF_GAIN",35,1,4);
        }
        fflush(stdout); vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
