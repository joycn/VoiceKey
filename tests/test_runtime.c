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
bool vk_board_button(void) { return false; }
void vk_board_led(uint8_t r,uint8_t g,uint8_t b) { rgb=((unsigned)r<<16)|((unsigned)g<<8)|b; }
esp_err_t vk_usb_start(const vk_usb_hooks_t *h) { captured=*h; return ESP_OK; }
int main(void) {
    vk_runtime_init(); assert(vk_runtime_usb_start()==ESP_OK); simulated.updated_ms=100;
    captured.connected(true); captured.streaming(true); captured.playback_active(true); vk_runtime_audio_ok(true);
    int16_t source[512]; for(unsigned i=0;i<512;++i) source[i]=1234;
    uint32_t epoch=vk_runtime_audio_epoch(); vk_runtime_publish(source,512,epoch);
    assert(state.usb.count==512 && state.wake.count==512);
    uint32_t inference=state.wake_epoch;
    int32_t dma[3][96];
    vk_runtime_playback_dma(dma[0],48); clock_ms++; vk_runtime_playback_dma(dma[1],48);
    captured.playback_packet(source,256);
    for(unsigned i=0;i<3;++i) { vk_runtime_playback_dma(dma[i],48); assert(dma[i][0]!=0); }
    simulated.muted=true; vk_runtime_inputs(); assert(vk_runtime_audio_epoch()!=epoch && !state.usb.count && !state.wake.count);
    vk_runtime_trigger(inference); assert(!state.pending);
    vk_runtime_playback_dma(dma[0],48); assert(dma[0][0]!=0); /* mute does not stop playback */
    captured.playback_active(false);
    for(unsigned i=0;i<3;++i) for(unsigned j=0;j<96;++j) assert(dma[i][j]==0);
    captured.playback_active(true); vk_runtime_playback_dma(dma[0],48); assert(!dma[0][0]);
    simulated.muted=false; simulated.updated_ms=100; clock_ms=201; vk_runtime_inputs();
    epoch=vk_runtime_audio_epoch(); vk_runtime_publish(source,512,epoch); assert(!state.wake.count);
    vk_runtime_trigger(state.wake_epoch); assert(!state.pending);
    simulated.updated_ms=201; vk_runtime_inputs(); assert(vk_runtime_audio_epoch()!=epoch);
    vk_runtime_publish(source,512,epoch); assert(!state.wake.count); /* stale capture rejected */
    assert(vk_runtime_capture_recover(vk_runtime_capture_generation()));
    epoch=vk_runtime_audio_epoch(); vk_runtime_publish(source,512,epoch); assert(state.wake.count==512);
    inference=state.wake_epoch; vk_runtime_audio_ok(false); /* actual RX overflow/failure adapter */
    assert(!state.wake.count && !state.usb.count); vk_runtime_trigger(inference); assert(!state.pending);
    vk_runtime_audio_ok(true); vk_runtime_publish(source,512,epoch); assert(!state.wake.count);
    simulated.valid=false; vk_runtime_inputs(); assert(state.muted);
    simulated.valid=true; vk_runtime_inputs();
    vk_runtime_init_status(0, 0, 0x105, true); assert(!vk_runtime_wake_ready());
    uint8_t diag[VK_DIAGNOSTIC_SIZE]; assert(captured.diagnostics(diag,sizeof(diag))==sizeof(diag) && diag[0]==2 && diag[1]==1 && diag[3]==8);
    assert(diag[56]==0x12 && diag[57]==0x34 && diag[72]==5 && diag[73]==1);
    assert((diag[7]&32) && !(diag[7]&16));
    assert(diag[78]==0x80 && diag[80]==0x40 && diag[81]==0xe2 && diag[82]==1);
    vk_runtime_init_status(0,0,0,true); assert(vk_runtime_wake_ready());
    vk_runtime_playback_dma(dma[0],48); clock_ms++; vk_runtime_playback_dma(dma[1],48);
    captured.playback_packet(source,128); vk_runtime_playback_dma(dma[0],48); assert(dma[0][0]!=0);
    captured.connected(false); assert(!dma[0][0] && !playback.count && !playback.active);
    vk_runtime_ui(true); (void)rgb;
    puts("PASS: actual runtime cache/epoch/new-wake gates, DMA mute independence, stop/restart/disconnect clearing and diagnostic identity");
}
