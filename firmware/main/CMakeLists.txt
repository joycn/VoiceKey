idf_component_register(SRCS "app_main.c" "board.c" "runtime.c" "audio_input.c" "wake_engine.c"
    INCLUDE_DIRS "."
    REQUIRES voicekey_core voicekey_usb esp_driver_gpio esp_driver_i2c esp_driver_i2s esp_timer esp_psram esp-sr led_strip)
