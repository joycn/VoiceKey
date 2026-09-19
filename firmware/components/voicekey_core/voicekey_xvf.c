#include "voicekey_xvf.h"
#include <string.h>
bool vk_xvf_read(vk_xvf_t *x, uint8_t r, uint8_t c, uint8_t *out, size_t n) {
    if (!n || n > VK_XVF_BUILD_BYTES || !out || !x->transfer) return false;
    uint8_t tx[] = {r, c | 0x80, (uint8_t)(n + 1)}, rx[VK_XVF_BUILD_BYTES + 1];
    for (unsigned attempt = 0; attempt < VK_XVF_ATTEMPTS; ++attempt) {
        int got = x->transfer(x->ctx, tx, sizeof(tx), rx, n + 1);
        if (got < 1) { x->errors++; return false; }
        if (rx[0] == 64) {
            x->busy++;
            if (attempt + 1 < VK_XVF_ATTEMPTS && x->retry_wait) x->retry_wait(x->ctx);
            continue;
        }
        if (got != (int)n + 1 || rx[0] != 0) { x->errors++; return false; }
        memcpy(out, rx + 1, n); return true;
    }
    x->errors++; return false;
}
bool vk_xvf_set(vk_xvf_t *x, uint8_t r, uint8_t c, const uint8_t *data, size_t n) {
    if (!n || n > 8 || !data || !x->transfer) return false;
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
static uint32_t le32(const uint8_t *b) {
    return (uint32_t)b[0] | (uint32_t)b[1]<<8 | (uint32_t)b[2]<<16 | (uint32_t)b[3]<<24;
}
static bool format(vk_xvf_t *x, vk_xvf_control_t *s) {
    uint8_t left[2], input;
    if (!vk_xvf_read(x,35,13,s->packed,2) || !vk_xvf_read(x,35,14,s->upsample,2) ||
        !vk_xvf_read(x,35,15,left,2) || !vk_xvf_read(x,35,10,&input,1)) return false;
    if (s->packed[0] || s->packed[1] || s->upsample[0]!=1 || s->upsample[1]!=1 ||
        left[0]!=6 || left[1]!=3 || input) { s->error=VK_XVF_FORMAT; return false; }
    return true;
}
void vk_xvf_poll(vk_xvf_t *x, vk_xvf_control_t *s, uint64_t now) {
    s->error=VK_XVF_IO;
    bool initializing=!s->configured;
    if (!s->configured) {
        s->profile_ok=false; s->aec_valid=false;
        memset(s->build,0,sizeof(s->build));
        if (!vk_xvf_read(x,48,0,s->version,3)) goto fault;
        if (s->version[0]!=1 || s->version[1]!=0 || s->version[2]!=8) {
            s->error=VK_XVF_VERSION; goto fault;
        }
        if (!vk_xvf_read(x,48,1,s->build,sizeof(s->build)) || !vk_xvf_read(x,48,8,s->usb_depth,2)) goto fault;
        /* No undocumented build-name assumptions: INT reports both depths zero.
           BLD_MSG is diagnostic metadata, not proof of the pinned binary hash. */
        if (!s->build[0]) { s->error=VK_XVF_METADATA; goto fault; }
        for (size_t i=0;i<sizeof(s->build) && s->build[i];++i)
            if (s->build[i]<32 || s->build[i]>126) { s->error=VK_XVF_METADATA; goto fault; }
        if (s->usb_depth[0] || s->usb_depth[1]) { s->error=VK_XVF_PROFILE; goto fault; }
        s->profile_ok=true;
        const uint8_t beam[]={6,3}, zero[]={0,0}, up[]={1,1};
        if (!vk_xvf_set(x,35,15,beam,2) || !vk_xvf_set(x,35,10,zero,1) ||
            !vk_xvf_set(x,35,13,zero,2) || !vk_xvf_set(x,35,14,up,2)) goto fault;
        if (!format(x,s)) goto fault;
        s->format_ms=now; s->configured=true;
        s->format_step=0; s->aec_step=0;
    }
    if (initializing) {
        uint8_t converged[4], bypass, gain[4];
        if (!vk_xvf_read(x,33,3,converged,4) || !vk_xvf_read(x,33,70,&bypass,1) ||
            !vk_xvf_read(x,35,1,gain,4)) goto fault;
        s->aec_converged=le32(converged); s->aec_bypass=bypass; s->ref_gain_bits=le32(gain);
        if (s->aec_converged>1 || bypass>1 || (s->ref_gain_bits&0x7f800000)==0x7f800000) goto fault;
        s->aec_ms=now; s->aec_valid=true;
    }
    /* At most one low-rate read per steady-state poll. Read safety inputs
       afterwards, so slow housekeeping cannot refresh an old mute sample. */
    if (!initializing && (s->aec_step || !s->aec_valid || now-s->aec_ms>=1000)) {
        if (!s->aec_step) { s->aec_step=1; s->aec_started_ms=now; }
        uint8_t value[4];
        uint8_t step=s->aec_step;
        if (!vk_xvf_read(x,step==3?35:33,step==1?3:step==2?70:1,value,step==2?1:4)) goto fault;
        uint32_t v=step==2?value[0]:le32(value);
        if ((step<3 && v>1) || (step==3 && (v&0x7f800000)==0x7f800000)) goto fault;
        s->pending_aec[step-1]=v;
        if (step==3) {
            s->aec_converged=s->pending_aec[0]; s->aec_bypass=s->pending_aec[1];
            s->ref_gain_bits=s->pending_aec[2]; s->aec_ms=s->aec_started_ms;
            s->aec_valid=true; s->aec_step=0;
        } else s->aec_step++;
    } else if (!initializing && now-s->format_ms>=100) {
        const uint8_t commands[]={13,14,15,10};
        const uint8_t expected[][2]={{0,0},{1,1},{6,3},{0,0}};
        uint8_t value[2], step=s->format_step;
        size_t n=step==3?1:2;
        if (!vk_xvf_read(x,35,commands[step],value,n)) goto fault;
        if (memcmp(value,expected[step],n)) { s->error=VK_XVF_FORMAT; goto fault; }
        s->format_step=(step+1)%4; s->format_ms=now;
    }
    uint8_t gpio[5], inactive;
    uint64_t safety_ms=x->clock_ms?x->clock_ms(x->ctx):now;
    if (!vk_xvf_read(x,20,0,gpio,5) || !vk_xvf_read(x,35,24,&inactive,1)) goto fault;
    s->led_power=gpio[3]!=0; s->muted=gpio[1]!=0; s->i2s_active=inactive==0;
    s->valid=s->aec_valid; s->updated_ms=safety_ms; s->error=VK_XVF_OK; return;
fault:
    s->valid=false; s->configured=false; s->muted=true; s->i2s_active=false;
    s->aec_valid=false; s->aec_step=0; s->updated_ms=now;
}
bool vk_xvf_capture_closed(const vk_xvf_control_t *s, uint64_t now) {
    return !vk_xvf_fresh(s->valid, s->updated_ms, now) || s->muted || !s->i2s_active;
}
