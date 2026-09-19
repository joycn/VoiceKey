#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#define VK_XVF_ATTEMPTS 3u
#define VK_XVF_CACHE_MS 100u
#define VK_XVF_BUILD_BYTES 50u
#define VK_XVF_AEC_CACHE_MS 1500u
enum { VK_XVF_OK, VK_XVF_IO, VK_XVF_VERSION, VK_XVF_PROFILE, VK_XVF_FORMAT, VK_XVF_METADATA };
/* Transfer returns exact received length, or -1 on timeout/NACK. Write has rxlen=0.
   Protocol writes have no response phase; readback verifies every setting. */
typedef int (*vk_xvf_transfer_t)(void *ctx, const uint8_t *tx, size_t txlen, uint8_t *rx, size_t rxlen);
typedef struct {
    vk_xvf_transfer_t transfer;
    void *ctx;
    uint32_t errors, busy;
    uint64_t (*clock_ms)(void *ctx);
    void (*retry_wait)(void *ctx); /* bounded task-side wait, never an ISR */
} vk_xvf_t;
bool vk_xvf_read(vk_xvf_t *x, uint8_t resource, uint8_t command, uint8_t *out, size_t n);
bool vk_xvf_set(vk_xvf_t *x, uint8_t resource, uint8_t command, const uint8_t *data, size_t n);
bool vk_xvf_fresh(bool valid, uint64_t updated_ms, uint64_t now_ms);
typedef struct {
    bool configured, valid, muted, i2s_active, led_power;
    uint8_t version[3], error, build[VK_XVF_BUILD_BYTES], usb_depth[2], packed[2], upsample[2];
    bool profile_ok, aec_valid;
    uint32_t aec_converged, aec_bypass, ref_gain_bits;
    uint64_t format_ms, aec_ms, aec_started_ms;
    uint8_t format_step, aec_step;
    uint32_t pending_aec[3];
    uint64_t updated_ms;
} vk_xvf_control_t;
/* One production control poll; caller owns transport and provides monotonic time. */
void vk_xvf_poll(vk_xvf_t *protocol, vk_xvf_control_t *state, uint64_t now_ms);
bool vk_xvf_capture_closed(const vk_xvf_control_t *state, uint64_t now_ms);
