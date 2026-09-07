#pragma once
#include "esp_err.h"
typedef void *usb_phy_handle_t;
typedef struct { int controller, target, otg_mode, otg_speed; } usb_phy_config_t;
#define USB_PHY_CTRL_OTG 1
#define USB_PHY_TARGET_INT 1
#define USB_OTG_MODE_DEVICE 1
#define USB_PHY_SPEED_FULL 1
esp_err_t usb_new_phy(const usb_phy_config_t *, usb_phy_handle_t *);
