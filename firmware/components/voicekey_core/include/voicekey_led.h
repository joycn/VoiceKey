#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define VK_LED_CONFIG_SIZE 49
#define VK_LED_FIELD_MASK 0x7f
#define VK_LED_REPORT_SIZE 384
/* config: effect, brightness, speed, gamma, RGB, 2 DOA RGBs, 12 ring RGBs. */
typedef struct {
    bool fault, muted, connected, spatial, valid, receiving;
    int owner, denied_region;
    uint32_t floor_generation, denied_generation;
} vk_led_inputs_t;
typedef struct { uint8_t config[49]; bool preserve_gamma; } vk_led_frame_t;
enum { VK_LED_NONE, VK_LED_RECEIVED, VK_LED_APPLIED, VK_LED_COVERED, VK_LED_FAILED, VK_LED_EXPIRED };
enum { VK_LED_IDLE, VK_LED_FAULT, VK_LED_MUTE, VK_LED_IDENTIFY, VK_LED_PREVIEW,
       VK_LED_MANUAL, VK_LED_EVENT, VK_LED_OWNER, VK_LED_RECEIVING };
typedef struct {
    uint8_t manual[49]; bool manual_on;
    uint16_t masks[8]; uint8_t layout_count; uint32_t layout_token;
    uint8_t temporary, preview, target; uint64_t until, started;
    uint32_t request_id; uint8_t outcome, reason, request_op;
    uint32_t floor_generation, denied_generation; bool seen_inputs, was_connected;
    int event_region; uint8_t event; uint64_t event_start, event_until;
    uint8_t source, display_target; uint32_t remaining;
    vk_led_frame_t applied; bool applied_valid;
} vk_led_t;
void vk_led_init(vk_led_t *s);
bool vk_led_patch_valid(uint8_t mask, const uint8_t *config);
void vk_led_patch(vk_led_t *s, uint8_t mask, const uint8_t *config);
void vk_led_legacy(vk_led_t *s, bool enabled, uint8_t r, uint8_t g, uint8_t b);
bool vk_led_command(vk_led_t *s, const uint8_t *report, size_t n, uint64_t now);
void vk_led_render(vk_led_t *s, const vk_led_inputs_t *input, uint32_t idle_rgb, uint64_t now, vk_led_frame_t *out);
void vk_led_applied(vk_led_t *s, const vk_led_frame_t *frame, bool ok);
void vk_led_diagnostics(const vk_led_t *s, uint8_t *out160);
