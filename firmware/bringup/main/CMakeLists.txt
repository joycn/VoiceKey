idf_component_register(SRCS "main.c" "../../main/runtime.c" "../../main/board.c" "../../main/audio_input.c" "../../main/wake_engine.c"
    INCLUDE_DIRS "../../main"
    REQUIRES voicekey_core voicekey_usb esp_driver_gpio esp_driver_i2c esp_driver_i2s esp_timer esp_psram esp-sr esp_app_format)
