#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#define VK_XVF_ATTEMPTS 3u
#define VK_XVF_CACHE_MS 100u
/* Transfer returns exact received length, or -1 on timeout/NACK. Write has rxlen=0.
   Protocol writes have no response phase; readback verifies every setting. */
typedef int (*vk_xvf_transfer_t)(void *ctx, const uint8_t *tx, size_t txlen, uint8_t *rx, size_t rxlen);
typedef struct { vk_xvf_transfer_t transfer; void *ctx; uint32_t errors, busy; } vk_xvf_t;
bool vk_xvf_read(vk_xvf_t *x, uint8_t resource, uint8_t command, uint8_t *out, size_t n);
bool vk_xvf_set(vk_xvf_t *x, uint8_t resource, uint8_t command, const uint8_t *data, size_t n);
bool vk_xvf_fresh(bool valid, uint64_t updated_ms, uint64_t now_ms);
typedef struct {
    bool configured, valid, muted, i2s_active;
    uint8_t version[3];
    uint64_t updated_ms;
} vk_xvf_control_t;
/* One production control poll; caller owns transport and provides monotonic time. */
void vk_xvf_poll(vk_xvf_t *protocol, vk_xvf_control_t *state, uint64_t now_ms);
bool vk_xvf_capture_closed(const vk_xvf_control_t *state, uint64_t now_ms);
