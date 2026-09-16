#include "voicekey_xvf.h"
#include <string.h>
bool vk_xvf_read(vk_xvf_t *x, uint8_t r, uint8_t c, uint8_t *out, size_t n) {
    if (!n || n > 8 || !out || !x->transfer) return false;
    uint8_t tx[] = {r, c | 0x80, (uint8_t)(n + 1)}, rx[9];
    for (unsigned attempt = 0; attempt < VK_XVF_ATTEMPTS; ++attempt) {
        int got = x->transfer(x->ctx, tx, sizeof(tx), rx, n + 1);
        if (got < 1) { x->errors++; return false; }
        if (rx[0] == 64) { x->busy++; continue; }
        if (got != (int)n + 1 || rx[0] != 0) { x->errors++; return false; }
        memcpy(out, rx + 1, n); return true;
    }
    x->errors++; return false;
}
bool vk_xvf_set(vk_xvf_t *x, uint8_t r, uint8_t c, const uint8_t *data, size_t n) {
    if (!n || n > 8 || !x->transfer) return false;
    uint8_t tx[11] = {r, c, (uint8_t)n}, readback[8];
    memcpy(tx + 3, data, n);
    if (x->transfer(x->ctx, tx, n + 3, NULL, 0) != 0) { x->errors++; return false; }
    if (!vk_xvf_read(x, r, c, readback, n)) return false;
    if (memcmp(data, readback, n)) { x->errors++; return false; }
    return true;
}
bool vk_xvf_fresh(bool valid, uint64_t updated, uint64_t now) {
    return valid && now >= updated && now - updated <= VK_XVF_CACHE_MS;
}
void vk_xvf_poll(vk_xvf_t *x, vk_xvf_control_t *s, uint64_t now) {
    if (!s->configured) {
        const uint8_t beam[] = {6, 3}, unpacked = 0;
        s->configured = vk_xvf_read(x, 48, 0, s->version, 3) &&
            s->version[0] == 1 && s->version[1] == 0 && s->version[2] == 8 &&
            vk_xvf_set(x, 35, 15, beam, sizeof(beam)) && vk_xvf_set(x, 35, 10, &unpacked, 1);
    }
    uint8_t gpio[5], inactive;
    s->valid = s->configured && vk_xvf_read(x, 20, 0, gpio, 5) && vk_xvf_read(x, 35, 24, &inactive, 1);
    s->muted = !s->valid || gpio[1] != 0;
    s->i2s_active = s->valid && inactive == 0;
    s->updated_ms = now;
    if (!s->valid) s->configured = false;
}
bool vk_xvf_capture_closed(const vk_xvf_control_t *s, uint64_t now) {
    return !vk_xvf_fresh(s->valid, s->updated_ms, now) || s->muted || !s->i2s_active;
}
