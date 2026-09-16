/* Runs the real board task through I2C failures and LED retries. */
#include "../firmware/main/board.c"
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
static void (*board_task)(void *);
static jmp_buf finished;
static unsigned cycle, publishes, led_writes[6], versions;
static uint8_t request[12], color[4], effect;
int64_t esp_timer_get_time(void) { return 100000+(int64_t)cycle*20000; }
TickType_t xTaskGetTickCount(void) { return 100; }
void vk_runtime_inputs(void) {
    vk_board_status_t s;vk_board_status(&s);publishes++;
    assert(s.valid == (cycle==0 || cycle>=4));
}
void vTaskDelayUntil(TickType_t *previous,TickType_t period) {
    assert(period==20);*previous+=period;
    assert(publishes==cycle+1); if(++cycle==6)longjmp(finished,1);
}
int xTaskCreatePinnedToCore(void (*fn)(void *),const char *name,uint32_t stack,void *arg,unsigned priority,void *handle,int core) {
    (void)name;(void)stack;(void)arg;(void)priority;(void)handle;(void)core;board_task=fn;return pdPASS;
}
esp_err_t gpio_config(const gpio_config_t *c) { assert(c->pin_bit_mask==1 && c->mode==GPIO_MODE_INPUT);return ESP_OK; }
int gpio_get_level(int pin) { assert(pin==0);return 1; }
esp_err_t i2c_new_master_bus(const i2c_master_bus_config_t *c,i2c_master_bus_handle_t *b) { assert(c->sda_io_num==5 && c->scl_io_num==6);*b=(void *)1;return ESP_OK; }
esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t b,const i2c_device_config_t *c,i2c_master_dev_handle_t *d) { (void)b;assert(c->device_address==0x2c);*d=(void *)2;return ESP_OK; }
esp_err_t i2c_master_transmit(i2c_master_dev_handle_t d,const uint8_t *bytes,size_t n,int timeout) {
    (void)d;assert(timeout==10 && n<=sizeof(request));memcpy(request,bytes,n);
    if(cycle>=2 && cycle<=4) { vk_board_status_t s;vk_board_status(&s);assert(!s.valid); } /* no premature healthy cache */
    if(cycle==1 && bytes[0]==20 && bytes[1]==0x80)return ESP_FAIL;
    if(!(bytes[1]&0x80) && bytes[0]==20) {
        led_writes[cycle]++;
        if(cycle==2 || cycle==3)return ESP_FAIL;
        if(bytes[1]==16)memcpy(color,bytes+3,4);else {assert(bytes[1]==12);effect=bytes[3];}
    }
    return ESP_OK;
}
esp_err_t i2c_master_receive(i2c_master_dev_handle_t d,uint8_t *bytes,size_t n,int timeout) {
    (void)d;assert(timeout==10 && n==request[2]);memset(bytes,0,n);
    uint8_t r=request[0],c=request[1]&127;
    if(r==48 && c==0){versions++;bytes[1]=1;bytes[3]=8;}
    else if(r==35 && c==15){bytes[1]=6;bytes[2]=3;}
    else if(r==20 && c==16)memcpy(bytes+1,color,4);
    else if(r==20 && c==12)bytes[1]=effect;
    else assert((r==20 && c==0) || (r==35 && (c==10 || c==24)));
    return ESP_OK;
}
int main(void) {
    assert(vk_board_init()==ESP_OK && vk_board_xmos_init()==ESP_OK && board_task);
    vk_board_led(0,0,32);
    if(!setjmp(finished))board_task(NULL);
    assert(versions==2 && led_writes[0]==2 && led_writes[1]==0 && led_writes[2]==1 && led_writes[3]==1 && led_writes[4]==2 && led_writes[5]==0);
    puts("PASS: actual board task coherent cache publication, persistent LED failure and LED reapplication after control reconfiguration");
}
