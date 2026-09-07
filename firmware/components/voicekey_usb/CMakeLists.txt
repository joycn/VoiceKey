idf_component_register(SRCS "usb_device.c" "usb_descriptors.c"
    INCLUDE_DIRS "include"
    REQUIRES tinyusb esp_hw_support esp_timer esp_driver_gpio voicekey_core
    PRIV_REQUIRES usb
    WHOLE_ARCHIVE) # TinyUSB discovers these application callbacks through the linker.
idf_component_get_property(tusb_lib espressif__tinyusb COMPONENT_LIB)
target_include_directories(${tusb_lib} PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")
target_compile_options(${COMPONENT_LIB} PRIVATE -Werror=missing-prototypes)
