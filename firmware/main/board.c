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
    uint32_t last_led = UINT32_MAX;
    uint8_t led_pending[4]={0}, led_step=0;
    uint64_t led_query_ms=0, led_started_ms=0;
    uint64_t agc_query_ms=0;
    vk_board_status_t next = {.muted = true};
    TickType_t tick = xTaskGetTickCount();
    for (;;) {
        if (!controller.configured) last_led = UINT32_MAX;
        vk_xvf_poll(&protocol, &controller, esp_timer_get_time() / 1000);
        next.valid = controller.valid; next.muted = controller.muted; next.i2s_active = controller.i2s_active;
        if (!next.valid) next.agc_valid=false;
        for (unsigned i = 0; i < 3; ++i) next.version[i] = controller.version[i];
        next.updated_ms = controller.updated_ms;
        next.control = controller;
        if (!next.valid) last_led = UINT32_MAX;
        portENTER_CRITICAL(&mux); uint32_t rgb = desired_led; portEXIT_CRITICAL(&mux);
        /* All I2C, including LEDs, belongs to this task. Mute overrides UI. */
        if (!next.valid || !next.i2s_active) rgb = 0x200020;
        else if (next.muted) rgb = 0x200000;
        if (!next.valid || rgb!=last_led) { next.led_valid=false; led_step=0; }
        if (next.valid && rgb != last_led) {
            /* XMOS owns animation: configure once per state change, no frame traffic.
               White = idle breathe; blue UI token = trigger rainbow. */
            uint8_t effect = rgb==0x202020 ? 1 : rgb==0x000020 ? 2 : rgb ? 3 : 0;
            uint32_t color_rgb = effect==1 ? 0xffffff : rgb;
            uint8_t color[] = {color_rgb, color_rgb>>8, color_rgb>>16, 0};
            uint8_t brightness=30, speed=1, gamma=0;
            /* Apply effect first and brightness last, as in Seeed's example.
               Mute/effect changes can replace XMOS LED settings: initialization
               alone cannot keep the requested brightness after a transition. */
            if (vk_xvf_set(&protocol,20,12,&effect,1) &&
                vk_xvf_set(&protocol,20,16,color,4) &&
                vk_xvf_set(&protocol,20,15,&speed,1) &&
                vk_xvf_set(&protocol,20,14,&gamma,1) &&
                vk_xvf_set(&protocol,20,13,&brightness,1)) {
                last_led=rgb;
            } else {
                next.valid = false; next.control.error = VK_XVF_IO; next.control.aec_valid = false; last_led = UINT32_MAX;
            }
        }
        /* Read one cosmetic parameter at most every 250ms, away from writes.
           A diagnostic read failure invalidates only this snapshot. */
        uint64_t led_now=esp_timer_get_time()/1000;
        if (next.valid && rgb==last_led && led_now-led_query_ms>=250) {
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
                    uint8_t expected_effect=rgb==0x202020 ? 1 : rgb==0x000020 ? 2 : rgb ? 3 : 0;
                    if (led_pending[0]!=expected_effect || led_pending[1]!=30 ||
                        led_pending[2]!=1 || led_pending[3]!=0) last_led=UINT32_MAX;
                }
            } else { next.led_valid=false; led_step=0; }
        } else if (next.valid && rgb==last_led && led_now-agc_query_ms>=1000) {
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
