#include "board.h"
#include "runtime.h"
#include "voicekey_xvf.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_check.h"
#include <string.h>
#include <math.h>
static const char *TAG = "xvf3800";
static i2c_master_dev_handle_t xmos;
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
static vk_board_status_t cache = {.muted = true};
static uint32_t desired_led;
static vk_led_t led = {.manual={3,30,1,0,255,255,255},.event_region=-1};
static int transfer(void *ctx, const uint8_t *tx, size_t txlen, uint8_t *rx, size_t rxlen) {
    (void)ctx;
    /* Official I2C protocol: STOP after write, independent read. A successful
       write has no status response; settings are checked by explicit readback.
       IDF reports NACK/short transaction as error, never accept partial buffers. */
    if (i2c_master_transmit(xmos, tx, txlen, 10) != ESP_OK) return -1;
    /* XMOS processes control requests asynchronously. Yield between command
       and response (also after a setting write before its readback command).
       Busy retries pass through here too, rather than hammering the bus.
       Only this task waits; cache freshness continues to gate capture. */
    vTaskDelay(pdMS_TO_TICKS(2));
    if (rxlen && i2c_master_receive(xmos, rx, rxlen, 10) != ESP_OK) return -1;
    return (int)rxlen;
}
static uint64_t control_clock(void *ctx) {
    (void)ctx; return esp_timer_get_time() / 1000;
}
static void retry_wait(void *ctx) {
    (void)ctx;
    vTaskDelay(pdMS_TO_TICKS(10));
}
static void control(void *arg) {
    (void)arg;
    vk_xvf_t protocol = {.transfer = transfer, .retry_wait = retry_wait, .clock_ms = control_clock};
    vk_xvf_control_t controller = {0};
    vk_led_frame_t last_frame={0}; bool have_frame=false;
    uint64_t last_frame_ms=0;
    uint8_t led_pending[4]={0}, led_step=0;
    uint64_t led_query_ms=0, led_started_ms=0;
    uint64_t agc_query_ms=0;
    vk_board_status_t next = {.muted = true};
    TickType_t tick = xTaskGetTickCount();
    for (;;) {
        if (!controller.configured) have_frame=false;
        /* Timestamp before the transaction; safety polling follows direction,
           so added I/O cannot rejuvenate an old physical-mute sample. */
        next.direction=(vk_observation_t){0};
        if (controller.configured && vk_runtime_direction_enabled())
            next.direction=vk_xvf_direction(&protocol,esp_timer_get_time()/1000);
        vk_xvf_poll(&protocol, &controller, esp_timer_get_time() / 1000);
        next.valid = controller.valid; next.muted = controller.muted; next.i2s_active = controller.i2s_active;
        if (!next.valid) next.agc_valid=false;
        for (unsigned i = 0; i < 3; ++i) next.version[i] = controller.version[i];
        next.updated_ms = controller.updated_ms;
        next.control = controller;
        if (!next.valid) {have_frame=false;portENTER_CRITICAL(&mux);led.applied_valid=false;portEXIT_CRITICAL(&mux);}
        vk_led_inputs_t input; vk_runtime_led_inputs(&input);
        input.fault = input.fault || !next.valid || !next.i2s_active;
        input.muted = next.muted;
        uint64_t render_now=esp_timer_get_time()/1000;
        vk_led_frame_t frame;
        portENTER_CRITICAL(&mux);
        vk_led_render(&led,&input,desired_led,render_now,&frame);
        uint32_t request=led.request_id;
        portEXIT_CRITICAL(&mux);
        bool changed=!have_frame || memcmp(&frame,&last_frame,sizeof(frame));
        uint8_t *config=frame.config;
        uint8_t effect=config[0],brightness=config[1],speed=config[2],gamma=config[3];
        if (!next.valid || changed) { next.led_valid=false; led_step=0; }
        /* Safety indicators bypass the animation rate limit. All other writes
           are coalesced to <=10 frames/s, regardless of host command frequency. */
        bool due=effect!=5 || !have_frame || input.fault || input.muted || render_now-last_frame_ms>=100;
        if (next.valid && changed && due) {
            uint8_t color[]={config[6],config[5],config[4],0},doa[8]={0},ring[48]={0};
            for(unsigned i=0;i<2;i++)for(unsigned j=0;j<3;j++)doa[i*4+j]=config[7+i*3+2-j];
            for(unsigned i=0;i<12;i++)for(unsigned j=0;j<3;j++)ring[i*4+j]=config[13+i*3+2-j];
            /* Ring animation frames only update the color payload when controls
               have not changed; do not restart the effect every frame. */
            bool colors_only=have_frame && effect==5 && last_frame.config[0]==5 &&
                !memcmp(config,last_frame.config,13) && frame.preserve_gamma==last_frame.preserve_gamma;
            bool ok=(colors_only || (vk_xvf_set(&protocol,20,12,&effect,1) &&
                vk_xvf_set(&protocol,20,16,color,4) && vk_xvf_set(&protocol,20,15,&speed,1) &&
                (frame.preserve_gamma || vk_xvf_set(&protocol,20,14,&gamma,1)) &&
                vk_xvf_set(&protocol,20,13,&brightness,1))) &&
                (effect!=4 || vk_xvf_set(&protocol,20,17,doa,sizeof(doa))) &&
                (effect!=5 || vk_xvf_set(&protocol,20,19,ring,sizeof(ring)));
            portENTER_CRITICAL(&mux);
            if(led.request_id==request)vk_led_applied(&led,&frame,ok);
            else {led.applied_valid=ok;if(ok)led.applied=frame;}
            portEXIT_CRITICAL(&mux);
            last_frame_ms=render_now;
            if(ok){last_frame=frame;have_frame=true;}
            else {next.valid=false;next.control.error=VK_XVF_IO;next.control.aec_valid=false;have_frame=false;}
        }
        bool settled=have_frame&&!memcmp(&frame,&last_frame,sizeof(frame));
        /* Read one cosmetic parameter at most every 250ms, away from writes.
           A diagnostic read failure invalidates only this snapshot. */
        uint64_t led_now=esp_timer_get_time()/1000;
        if (next.valid && settled && led_now-led_query_ms>=250) {
            const uint8_t commands[]={12,13,15,14};
            led_query_ms=led_now;
            if (!led_step) led_started_ms=led_now;
            if (vk_xvf_read(&protocol,20,commands[led_step],&led_pending[led_step],1)) {
                if (++led_step==4) {
                    memcpy(next.led_settings,led_pending,4);
                    next.led_updated_ms=led_started_ms; next.led_valid=true; led_step=0;
                    /* Detect autonomous XMOS setting changes without restarting
                       a healthy animation on every poll. Retry only after a
                       complete, successful, low-frequency snapshot. */
                    if (led_pending[0]!=effect || led_pending[1]!=brightness ||
                        led_pending[2]!=speed || (!frame.preserve_gamma && led_pending[3]!=gamma)) have_frame=false;
                }
            } else { next.led_valid=false; led_step=0; }
        } else if (next.valid && settled && led_now-agc_query_ms>=1000) {
            /* Read-only telemetry, separate from LED transactions. Failure must
               not retain an old gain as current or reconfigure audio. */
            uint8_t raw[4]; float gain;
            agc_query_ms=led_now;
            next.agc_valid=false;
            if (vk_xvf_read(&protocol,17,13,raw,sizeof(raw))) {
                memcpy(&gain,raw,sizeof(gain));
                if (isfinite(gain) && gain>=0) {
                    memcpy(&next.agc_gain_bits,raw,sizeof(raw));
                    next.agc_updated_ms=esp_timer_get_time()/1000;
                    next.agc_valid=true;
                }
            }
        }
        next.led_fallback=false;
        /* Publish once, only after all required readbacks succeeded. A failed
           LED retry must never expose a transient healthy control cache. */
        next.errors = protocol.errors; next.busy = protocol.busy;
        portENTER_CRITICAL(&mux); cache = next; portEXIT_CRITICAL(&mux);
        vk_runtime_inputs();
        vTaskDelayUntil(&tick, pdMS_TO_TICKS(20));
    }
}
esp_err_t vk_board_init(void) {
    gpio_config_t inputs = {.pin_bit_mask = 1ULL << VK_PIN_BUTTON,
        .mode = GPIO_MODE_INPUT, .pull_up_en = GPIO_PULLUP_ENABLE};
    return gpio_config(&inputs);
}
esp_err_t vk_board_xmos_init(void) {
    i2c_master_bus_config_t cfg = {.i2c_port = I2C_NUM_0, .sda_io_num = VK_PIN_I2C_SDA,
        .scl_io_num = VK_PIN_I2C_SCL, .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7, .flags.enable_internal_pullup = true};
    i2c_master_bus_handle_t bus;
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&cfg, &bus), TAG, "I2C bus");
    i2c_device_config_t dev = {.dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x2c, .scl_speed_hz = 100000};
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(bus, &dev, &xmos), TAG, "XVF3800 device");
    return xTaskCreatePinnedToCore(control, "xvf-control", 4096, NULL, 6, NULL, 0) == pdPASS ? ESP_OK : ESP_ERR_NO_MEM;
}
void vk_board_status(vk_board_status_t *out) {
    portENTER_CRITICAL(&mux); *out = cache; portEXIT_CRITICAL(&mux);
    out->valid = vk_xvf_fresh(out->valid, out->updated_ms, esp_timer_get_time() / 1000);
}
bool vk_board_muted(void) {
    vk_board_status_t s; vk_board_status(&s); return !s.valid || !s.i2s_active || s.muted;
}
bool vk_board_button(void) { return gpio_get_level(VK_PIN_BUTTON) == 0; }
void vk_board_led(uint8_t r, uint8_t g, uint8_t b) {
    portENTER_CRITICAL(&mux); desired_led = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b; portEXIT_CRITICAL(&mux);
}

void vk_board_led_override(bool enabled,uint8_t r,uint8_t g,uint8_t b) {
    portENTER_CRITICAL(&mux);vk_led_legacy(&led,enabled,r,g,b);portEXIT_CRITICAL(&mux);
}
void vk_board_led_config(uint8_t mask,const uint8_t *config) {
    portENTER_CRITICAL(&mux);vk_led_patch(&led,mask,config);portEXIT_CRITICAL(&mux);
}
void vk_board_led_command(const uint8_t *report,size_t n) {
    uint64_t now=esp_timer_get_time()/1000;
    portENTER_CRITICAL(&mux);vk_led_command(&led,report,n,now);portEXIT_CRITICAL(&mux);
}
void vk_board_led_diagnostics(uint8_t *out) {
    portENTER_CRITICAL(&mux);vk_led_diagnostics(&led,out);portEXIT_CRITICAL(&mux);
}
