/* Executes actual runtime adapter/ISR handoff with only board/OS/USB transport
   replaced; no claim of interrupt timing or hardware DMA emulation. */
#include "../firmware/main/runtime.c"
#include "voicekey_xvf.h"
#include <assert.h>
#include <stdio.h>
static vk_board_status_t simulated = {.valid=true,.i2s_active=true,.version={1,0,8}};
static uint64_t clock_ms = 100;
static vk_usb_hooks_t captured;
static unsigned rgb;
size_t esp_psram_get_size(void) { return 8u*1024*1024; }
uint32_t esp_get_free_heap_size(void) { return 123456; }
uint32_t esp_get_minimum_free_heap_size(void) { return 100000; }
int64_t esp_timer_get_time(void) { return clock_ms*1000; }
const esp_app_desc_t *esp_app_get_description(void) { static const esp_app_desc_t d = {.app_elf_sha256={0x12,0x34}}; return &d; }
void vk_board_status(vk_board_status_t *out) { *out=simulated; out->valid=vk_xvf_fresh(out->valid,out->updated_ms,clock_ms); }
bool vk_board_muted(void) { vk_board_status_t b; vk_board_status(&b); return !b.valid || b.muted || !b.i2s_active; }
static bool button_down;
bool vk_board_button(void) { return button_down; }
void vk_board_led(uint8_t r,uint8_t g,uint8_t b) { rgb=((unsigned)r<<16)|((unsigned)g<<8)|b; }
esp_err_t vk_usb_start(const vk_usb_hooks_t *h) { captured=*h; return ESP_OK; }
int main(void) {
    vk_runtime_init(); vk_runtime_usb_start(); captured.connected(true);
    state.audio_ok=true; capture_clock_ok=true; simulated.updated_ms=clock_ms;
    button_down=true; vk_runtime_ui(true);
    clock_ms+=40; simulated.updated_ms=clock_ms; vk_runtime_ui(true);
    assert(!state.pending && !state.suppressed);
    button_down=false; vk_runtime_ui(true);
    clock_ms+=10; button_down=true; simulated.updated_ms=clock_ms; vk_runtime_ui(true);
    clock_ms+=40; simulated.updated_ms=clock_ms; vk_runtime_ui(true);
    assert(!state.pending); /* a bouncing release must not arm */
    button_down=false; vk_runtime_ui(true);
    clock_ms+=30; simulated.updated_ms=clock_ms; vk_runtime_ui(true);
    assert(!state.pending); /* release never emits a shortcut */
    button_down=true; vk_runtime_ui(true);
    clock_ms+=29; simulated.updated_ms=clock_ms; vk_runtime_ui(true); assert(!state.pending);
    clock_ms++; simulated.updated_ms=clock_ms; vk_runtime_ui(true); assert(state.pending);
    button_down=false; vk_runtime_ui(true);
    clock_ms+=30; simulated.updated_ms=clock_ms; vk_runtime_ui(true);
    clock_ms=100;

    vk_runtime_init(); assert(vk_runtime_usb_start()==ESP_OK); simulated.updated_ms=100; capture_clock_ok=true;
    captured.connected(true); captured.streaming(true); captured.playback_active(true); vk_runtime_audio_ok(true);
    int16_t source[512]; for(unsigned i=0;i<512;++i) source[i]=1234;
    uint32_t epoch=vk_runtime_audio_epoch(); vk_runtime_publish(source,512,epoch);
    assert(state.usb.count==512 && state.wake.count==512);
    uint32_t inference=state.wake_epoch;
    int32_t dma[3][96];
    vk_runtime_playback_dma(dma[0],48); clock_ms++; vk_runtime_playback_dma(dma[1],48);
    for(unsigned i=0;i<251;i++) { clock_ms++; vk_runtime_playback_dma(dma[i%3],48); }
    captured.playback_packet(source,256);
    for(unsigned i=0;i<3;++i) { vk_runtime_playback_dma(dma[i],48); assert(dma[i][0]!=0); }
    simulated.muted=true; vk_runtime_inputs(); assert(vk_runtime_audio_epoch()!=epoch && !state.usb.count && !state.wake.count);
    vk_runtime_trigger(inference); assert(!state.pending);
    vk_runtime_playback_dma(dma[0],48); assert(dma[0][0]!=0); /* mute does not stop playback */
    captured.playback_active(false);
    for(unsigned i=0;i<3;++i) for(unsigned j=0;j<96;++j) assert(dma[i][j]==0);
    captured.playback_active(true); vk_runtime_playback_dma(dma[0],48); assert(!dma[0][0]);
    simulated.muted=false; simulated.updated_ms=clock_ms; capture_clock_ok=true; clock_ms+=101; vk_runtime_inputs();
    epoch=vk_runtime_audio_epoch(); vk_runtime_publish(source,512,epoch); assert(!state.wake.count);
    vk_runtime_trigger(state.wake_epoch); assert(!state.pending);
    simulated.updated_ms=clock_ms; capture_clock_ok=true; vk_runtime_inputs(); assert(vk_runtime_audio_epoch()!=epoch);
    vk_runtime_publish(source,512,epoch); assert(!state.wake.count); /* stale capture rejected */
    assert(vk_runtime_capture_recover(vk_runtime_capture_generation()));
    epoch=vk_runtime_audio_epoch(); vk_runtime_publish(source,512,epoch); assert(state.wake.count==512);
    inference=state.wake_epoch; vk_runtime_audio_ok(false); /* actual RX overflow/failure adapter */
    assert(!state.wake.count && !state.usb.count); vk_runtime_trigger(inference); assert(!state.pending);
    vk_runtime_audio_ok(true); vk_runtime_publish(source,512,epoch); assert(!state.wake.count);
    simulated.valid=false; vk_runtime_inputs(); assert(state.muted);
    simulated.valid=true; vk_runtime_inputs();
    vk_runtime_init_status(0, 0, 0x105, true); assert(!vk_runtime_wake_ready());
    uint8_t diag[VK_DIAGNOSTIC_SIZE]; assert(captured.diagnostics(diag,sizeof(diag))==sizeof(diag) && diag[0]==3 && diag[1]==1 && diag[3]==8);
    assert(diag[56]==0x12 && diag[57]==0x34 && diag[72]==5 && diag[73]==1);
    assert((diag[7]&32) && !(diag[7]&16));
    assert(diag[78]==0x80 && diag[80]==0x40 && diag[81]==0xe2 && diag[82]==1);
    vk_runtime_init_status(0,0,0,true); assert(vk_runtime_wake_ready());
    vk_runtime_playback_dma(dma[0],48); clock_ms++; vk_runtime_playback_dma(dma[1],48);
    for(unsigned i=0;i<251;i++) { clock_ms++; vk_runtime_playback_dma(dma[i%3],48); }
    captured.playback_packet(source,128); vk_runtime_playback_dma(dma[0],48); assert(dma[0][0]!=0);
    captured.connected(false); assert(!dma[0][0] && !playback.count && !playback.active);
    vk_runtime_ui(true); (void)rgb;
    /* Measure actual completion counts: startup and a 16k wrong-clock profile
       must close capture; 48k then qualifies, a gap revokes qualification. */
    transport_fault(); last_dma_us=0; clock_ms=1000;
    for(unsigned i=0;i<=300;i++) { vk_runtime_playback_dma(dma[i%3],48); clock_ms+=3; }
    assert(!capture_clock_ok && measured_rate_hz==16000);
    captured.connected(true); captured.playback_active(true);
    captured.playback_packet(source,256);
    vk_runtime_playback_dma(dma[0],48);
    assert(!playback.active && !playback.count && dma[0][0]==0);
    assert(captured.diagnostics(diag,sizeof(diag))==sizeof(diag) && !(diag[97]&2));
    for(unsigned i=0;i<600;i++) { vk_runtime_playback_dma(dma[i%3],48); clock_ms++; }
    assert(capture_clock_ok && measured_rate_hz==48000);
    assert(playback.active && !playback.count);
    captured.playback_packet(source,256);
    clock_ms++; vk_runtime_playback_dma(dma[0],48); assert(dma[0][0]!=0);
    for(unsigned i=0;i<300;i++) { clock_ms+=3; vk_runtime_playback_dma(dma[i%3],48); }
    assert(!capture_clock_ok && !playback.active && !playback.count);
    captured.playback_packet(source,256); assert(!playback.count);
    for(unsigned i=0;i<600;i++) { clock_ms++; vk_runtime_playback_dma(dma[i%3],48); }
    assert(capture_clock_ok && playback.active && !playback.count);
    for(unsigned i=0;i<3;i++) assert(dma[i][0]==0);
    simulated.updated_ms=clock_ms; simulated.control.aec_valid=true;
    simulated.control.aec_ms=clock_ms; simulated.control.aec_converged=1;
    simulated.control.ref_gain_bits=0x3f800000;
    memcpy(simulated.control.build,"fixture INT build",17);
    captured.diagnostics(diag,sizeof(diag)); assert((diag[97]&6)==6 && diag[112]==1 && diag[124]=='f');
    clock_ms+=6; simulated.updated_ms=clock_ms; vk_runtime_inputs(); assert(!capture_clock_ok && state.muted);
    simulated.control.aec_ms=clock_ms-1501;
    captured.diagnostics(diag,sizeof(diag)); assert(!(diag[97]&4));
    simulated.valid=true; simulated.updated_ms=clock_ms; state.muted=false; state.audio_ok=true;
    vk_runtime_wake_observed(state.wake_epoch,32768,true);
    captured.diagnostics(diag,sizeof(diag));
    assert(diag[174]==4 && (diag[175]&1) && diag[180]==1 && diag[184]==1);
    assert(diag[188]==0 && diag[189]==128);
    state.wake_epoch++;
    captured.diagnostics(diag,sizeof(diag)); assert(!(diag[175]&1) && !diag[188] && !diag[189]);
    vk_runtime_wake_observed(state.wake_epoch,1234,false);
    clock_ms+=501; simulated.updated_ms=clock_ms;
    captured.diagnostics(diag,sizeof(diag)); assert(!(diag[175]&1) && diag[180]==2 && diag[184]==1);
    simulated.led_valid=true; simulated.led_updated_ms=clock_ms;
    simulated.agc_valid=true; simulated.agc_updated_ms=clock_ms;
    simulated.agc_gain_bits=0x3d000000; /* 0.03125: observed attenuation must not be clipped to 1. */
    simulated.control.led_power=true; simulated.led_fallback=true;
    simulated.led_settings[0]=1; simulated.led_settings[1]=10;
    simulated.led_settings[2]=5; simulated.led_settings[3]=1;
    captured.diagnostics(diag,sizeof(diag));
    assert(diag[156]==7 && diag[157]==1 && diag[158]==10 && diag[159]==5 && diag[160]==1);
    assert(diag[165]==1 && diag[169]==0x3d);
    clock_ms+=2001; simulated.updated_ms=clock_ms;
    captured.diagnostics(diag,sizeof(diag)); assert(!(diag[156]&1));
    assert(!diag[165]);
    puts("PASS: actual runtime cache/epoch/new-wake gates, DMA mute independence, stop/restart/disconnect clearing and diagnostic identity");
}
