#include "voicekey_audio.h"
#include "voicekey_xvf.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static void fir_test(void) {
    double low = 2, high = 0, stop = 0;
    for (unsigned hz = 0; hz <= 24000; hz += 5) {
        double re = 0, im = 0;
        for (unsigned k = 0; k < VK_FIR_TAPS; ++k) {
            double angle = 2 * 3.141592653589793 * hz * k / 48000;
            re += vk_fir_coefficients[k] * cos(angle); im -= vk_fir_coefficients[k] * sin(angle);
        }
        double magnitude = hypot(re, im);
        if (hz <= 6500) { if (magnitude < low) low = magnitude; if (magnitude > high) high = magnitude; }
        if (hz >= 8000 && magnitude > stop) stop = magnitude;
    }
    double ripple = 20 * log10(high/low), rejection = -20 * log10(stop);
    assert(ripple <= 0.2 && rejection >= 60);
    printf("FIR actual float coefficients: passband ripple %.6f dB; stopband attenuation %.3f dB (5 Hz grid)\n", ripple, rejection);
    int32_t stereo[6000]; int16_t whole[1000], chunks[1000];
    uint32_t rng = 4;
    for (unsigned i = 0; i < 6000; ++i) { rng = rng * 1664525u + 1013904223u; stereo[i] = (int32_t)(rng / 2); }
    vk_decimator_t a, b; vk_decimator_reset(&a); vk_decimator_reset(&b);
    assert(vk_decimate(&a, stereo, 3000, whole, 1) == 1000);
    size_t offset = 0, n = 0;
    while (offset < 3000) { size_t k = offset % 137 + 1; if (k > 3000-offset) k = 3000-offset;
        n += vk_decimate(&b, stereo+2*offset, k, chunks+n, 1); offset += k; }
    assert(n == 1000 && !memcmp(whole, chunks, sizeof(whole)));
    vk_decimator_reset(&b); memset(stereo, 0, sizeof(stereo));
    vk_decimate(&b, stereo, 3000, chunks, 8); for (unsigned i=0;i<1000;++i) assert(chunks[i] == 0);
    for (unsigned i=0;i<6000;i+=2) stereo[i] = INT32_MAX;
    vk_decimate(&b, stereo, 3000, chunks, 8); assert(chunks[999] == INT16_MAX);
    for (unsigned i=0;i<6000;i+=2) stereo[i] = INT32_MIN;
    vk_decimate(&b, stereo, 3000, chunks, 8); assert(chunks[999] == INT16_MIN);
    /* Right slot is deliberately ignored, and left anti-alias filtering is actual production code. */
    vk_decimator_reset(&b); for (unsigned i=0;i<6000;i+=2) { stereo[i]=0; stereo[i+1]=INT32_MAX; }
    vk_decimate(&b, stereo, 3000, chunks, 1); for (unsigned i=0;i<1000;++i) assert(chunks[i] == 0);
}
static unsigned calls, read_calls; static int scenario;
static int fake(void *ctx, const uint8_t *tx, size_t nt, uint8_t *rx, size_t nr) {
    (void)ctx; calls++;
    assert(tx[0] == 35 && (tx[1] & 127) == 15);
    if (!nr) { assert(nt == 5 && tx[2] == 2 && tx[3] == 6 && tx[4] == 3); return 0; }
    assert(nt == 3 && tx[1] == (15|128) && tx[2] == 3 && nr == 3);
    read_calls++;
    if (scenario == 1) return -1;
    rx[0] = scenario == 2 || (scenario == 3 && read_calls == 1) ? 64 : 0;
    rx[1] = scenario == 4 ? 7 : 6; rx[2] = 3;
    if (scenario == 5) return 2;
    if (scenario == 6) rx[0] = 1;
    return nr;
}
static void protocol_test(void) {
    const uint8_t beam[] = {6,3};
    for (scenario=0;scenario<=6;++scenario) {
        vk_xvf_t x = {.transfer=fake}; calls=read_calls=0;
        bool result = vk_xvf_set(&x,35,15,beam,2);
        assert(result == (scenario==0 || scenario==3)); assert(calls <= 4);
        if (scenario==2) assert(x.busy == 3 && x.errors == 1);
    }
    assert(vk_xvf_fresh(true, 100, 200)); assert(!vk_xvf_fresh(true,100,201));
    assert(!vk_xvf_fresh(false,100,100)); assert(!vk_xvf_fresh(true,100,99));
}
static void playback_test(void) {
    vk_playback_t p; vk_playback_init(&p); int16_t in[1960]; int32_t out[1960];
    for(unsigned i=0;i<1960;++i) in[i]=32767;
    vk_playback_push(&p,in,49); assert(p.count == 0);
    vk_playback_active(&p,true); vk_playback_push(&p,in,49);
    vk_playback_render(&p,out,48); assert(p.count==1 && out[0]==32767*65536);
    vk_playback_control(&p,true,0); vk_playback_render(&p,out,48);
    for(unsigned i=0;i<96;++i) assert(out[i]==0);
    assert(p.missing == 47 && p.count==0);
    vk_playback_control(&p,false,-6*256); vk_playback_push(&p,in,49); vk_playback_render(&p,out,48);
    assert(out[0] > 1000000000 && out[0] < 1100000000);
    vk_playback_push(&p,in,960); assert(p.count==960 && p.dropped==1);
    vk_playback_active(&p,false); vk_playback_active(&p,true); vk_playback_render(&p,out,48);
    for(unsigned i=0;i<96;++i) assert(out[i]==0);
    /* Simulate actual I2S clocks at +/-1000ppm and feedback-driven host traffic. */
    for (int drift=-1;drift<=1;drift+=2) {
        vk_playback_init(&p); vk_playback_active(&p,true); vk_playback_push(&p,in,VK_PLAY_TARGET);
        double device_fraction=0, host_fraction=0; uint32_t consumed=0;
        for (unsigned ms=1;ms<=10000;++ms) {
            uint32_t fb=vk_playback_feedback(&p,(consumed / 48) * 48,(uint64_t)ms*1000);
            assert(fb >= 47.75*65536 && fb <= 48.25*65536);
            host_fraction += fb/65536.0; size_t host=(size_t)host_fraction; host_fraction-=host;
            vk_playback_push(&p,in,host);
            device_fraction += 48*(1+drift*0.001); size_t frames=(size_t)device_fraction; device_fraction-=frames;
            vk_playback_render(&p,out,frames); consumed+=frames;
            assert(p.count < VK_PLAY_FRAMES);
        }
        assert(p.dropped==0 && p.missing==0); assert(fabs(p.measured_rate-48*(1+drift*0.001))<0.05);
        vk_playback_render(&p,out,960); assert(p.count==0); /* host stall drains to silence */
        vk_playback_active(&p,false); vk_playback_push(&p,in,49); assert(p.count==0);
    }
}
static bool physical_mute, inactive_state, bad_version, fail_control;
static unsigned version_reads, gpo_reads, writes;
static int controller_transport(void *ctx, const uint8_t *tx, size_t nt, uint8_t *rx, size_t nr) {
    (void)ctx; assert(nt >= 3);
    if (fail_control) return -1;
    uint8_t r = tx[0], c = tx[1] & 127;
    if (!nr) {
        writes++;
        /* Strict allowlist excludes every GPIO/unmute write. */
        assert(r == 35 && ((c == 15 && nt == 5 && tx[3] == 6 && tx[4] == 3) || (c == 10 && nt == 4 && tx[3] == 0)));
        return 0;
    }
    assert(tx[1] & 128); assert(tx[2] == nr); memset(rx, 0, nr);
    if (r == 48 && c == 0) { assert(nr == 4); version_reads++; rx[1]=1; rx[3]=bad_version?9:8; }
    else if (r == 35 && c == 15) { assert(nr==3); rx[1]=6; rx[2]=3; }
    else if (r == 35 && c == 10) { assert(nr==2); }
    else if (r == 20 && c == 0) { assert(nr==6); gpo_reads++; rx[1]=1; rx[2]=physical_mute; rx[3]=1; rx[4]=1; rx[5]=1; }
    else { assert(r==35 && c==24 && nr==2); rx[1]=inactive_state; }
    return nr;
}
static void controller_test(void) {
    vk_xvf_t x = {.transfer=controller_transport}; vk_xvf_control_t s = {0};
    vk_xvf_poll(&x,&s,100); assert(s.valid && !s.muted && !vk_xvf_capture_closed(&s,200));
    assert(version_reads==1 && writes==2 && gpo_reads==1);
    assert(vk_xvf_capture_closed(&s,201));
    physical_mute=true; vk_xvf_poll(&x,&s,220); assert(s.valid && s.muted && vk_xvf_capture_closed(&s,220));
    physical_mute=false; inactive_state=true; vk_xvf_poll(&x,&s,240); assert(!s.i2s_active && vk_xvf_capture_closed(&s,240));
    inactive_state=false; fail_control=true; vk_xvf_poll(&x,&s,260); assert(!s.valid && !s.configured && vk_xvf_capture_closed(&s,260));
    fail_control=false; bad_version=true; vk_xvf_poll(&x,&s,280); assert(!s.valid && vk_xvf_capture_closed(&s,280));
    bad_version=false; vk_xvf_poll(&x,&s,300); assert(s.valid && !vk_xvf_capture_closed(&s,300));
    assert(writes==4); /* only initial + successful recovery configuration */
}

int main(void) { fir_test(); protocol_test(); controller_test(); playback_test(); puts("PASS: FIR chunk/reset/saturation/response, XVF status/busy/short/timeout/stale, playback lifecycle and clock drift"); }
