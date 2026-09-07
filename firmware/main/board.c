#include "board.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "led_strip.h"

static const char *TAG = "board";
static led_strip_handle_t strip;
static i2c_master_dev_handle_t xmos;

esp_err_t vk_board_init(void) {
    gpio_config_t inputs = {.pin_bit_mask = (1ULL << VK_PIN_MUTE) | (1ULL << VK_PIN_BUTTON),
        .mode = GPIO_MODE_INPUT, .pull_up_en = GPIO_PULLUP_ENABLE};
    ESP_RETURN_ON_ERROR(gpio_config(&inputs), TAG, "input pins");
    gpio_config_t outputs = {.pin_bit_mask = (1ULL << VK_PIN_XMOS_RESET) |
        (1ULL << VK_PIN_LED_POWER) | (1ULL << VK_PIN_AMP_ENABLE) | (1ULL << VK_PIN_I2S_REFERENCE), .mode = GPIO_MODE_OUTPUT};
    ESP_RETURN_ON_ERROR(gpio_config(&outputs), TAG, "output pins");
    gpio_set_level(VK_PIN_AMP_ENABLE, 0);
    /* XMOS clocks its reference input continuously. With no local playback,
       constant-low serial data is valid zero PCM and must not float. */
    gpio_set_level(VK_PIN_I2S_REFERENCE, 0);
    gpio_set_level(VK_PIN_XMOS_RESET, 0);
    gpio_set_level(VK_PIN_LED_POWER, 1);
    led_strip_config_t cfg = {.strip_gpio_num = VK_PIN_LED, .max_leds = 12,
        .led_model = LED_MODEL_WS2812, .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB};
    led_strip_rmt_config_t rmt = {.clk_src = RMT_CLK_SRC_DEFAULT, .resolution_hz = 10000000,
        .mem_block_symbols = 64, .flags.with_dma = false};
    ESP_RETURN_ON_ERROR(led_strip_new_rmt_device(&cfg, &rmt, &strip), TAG, "LED strip");
    return led_strip_clear(strip);
}
static esp_err_t xmos_query(const uint8_t *request, size_t req_len, uint8_t *reply, size_t reply_len) {
    /* Protocol uses a write STOP followed by a separate read, matching upstream. */
    ESP_RETURN_ON_ERROR(i2c_master_transmit(xmos, request, req_len, 100), TAG, "XMOS request");
    ESP_RETURN_ON_ERROR(i2c_master_receive(xmos, reply, reply_len, 100), TAG, "XMOS response");
    return reply[0] == 0 ? ESP_OK : ESP_ERR_INVALID_RESPONSE;
}
esp_err_t vk_board_xmos_init(void) {
    i2c_master_bus_config_t cfg = {.i2c_port = I2C_NUM_0, .sda_io_num = VK_PIN_I2C_SDA,
        .scl_io_num = VK_PIN_I2C_SCL, .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7, .flags.enable_internal_pullup = true};
    i2c_master_bus_handle_t bus;
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&cfg, &bus), TAG, "I2C bus");
    i2c_device_config_t dev = {.dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x42, .scl_speed_hz = 100000};
    ESP_RETURN_ON_ERROR(i2c_master_bus_add_device(bus, &dev, &xmos), TAG, "XMOS device");
    gpio_set_level(VK_PIN_XMOS_RESET, 1);
    vTaskDelay(pdMS_TO_TICKS(1));
    gpio_set_level(VK_PIN_XMOS_RESET, 0);
    vTaskDelay(pdMS_TO_TICKS(3000));
    const uint8_t req[] = {240, 0x80 | 88, 4};
    uint8_t version[4] = {0};
    ESP_RETURN_ON_ERROR(xmos_query(req, sizeof(req), version, sizeof(version)), TAG, "XMOS version");
    ESP_LOGI(TAG, "XMOS %u.%u.%u (firmware preserved)", version[1], version[2], version[3]);
    const uint8_t registers[] = {0x30, 0x40};
    const uint8_t stages[] = {4, 3}; /* AGC and NS; selected channel goes to BOTH consumers. */
    for (unsigned i = 0; i < 2; ++i) {
        uint8_t set[] = {241, registers[i], 1, stages[i]};
        ESP_RETURN_ON_ERROR(i2c_master_transmit(xmos, set, sizeof(set), 100), TAG, "set stage");
        uint8_t get[] = {241, registers[i] | 0x80, 2}, reply[2] = {0};
        ESP_RETURN_ON_ERROR(xmos_query(get, sizeof(get), reply, sizeof(reply)), TAG, "get stage");
        ESP_RETURN_ON_FALSE(reply[1] == stages[i], ESP_ERR_INVALID_RESPONSE, TAG, "stage readback");
    }
    return ESP_OK;
}
bool vk_board_muted(void) { return gpio_get_level(VK_PIN_MUTE) != 0; }
bool vk_board_button(void) { return gpio_get_level(VK_PIN_BUTTON) == 0; }
void vk_board_led(uint8_t r, uint8_t g, uint8_t b) {
    if (!strip) return;
    for (unsigned i = 0; i < 12; ++i) led_strip_set_pixel(strip, i, r, g, b);
    led_strip_refresh(strip);
}
