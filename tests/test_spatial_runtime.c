/* Production runtime routing, using synthetic board/clock and USB transport. */
#include "../firmware/main/runtime.c"
#include <assert.h>
#include <stdio.h>
static uint64_t clock_ms=1000;
static vk_board_status_t board={.valid=true,.i2s_active=true};
static vk_usb_hooks_t hooks;
int64_t esp_timer_get_time(void){return clock_ms*1000;}
size_t esp_psram_get_size(void){return 8388608;}
uint32_t esp_get_free_heap_size(void){return 100000;}
uint32_t esp_get_minimum_free_heap_size(void){return 90000;}
const esp_app_desc_t *esp_app_get_description(void){static const esp_app_desc_t d={0};return &d;}
void vk_board_status(vk_board_status_t *out){*out=board;}
bool vk_board_muted(void){return board.muted;}
bool vk_board_button(void){return false;}
void vk_board_led(uint8_t r,uint8_t g,uint8_t b){(void)r;(void)g;(void)b;}
esp_err_t vk_usb_start(const vk_usb_hooks_t *h){hooks=*h;return ESP_OK;}
static void observe(unsigned angle,bool speech) {
    clock_ms+=50;board.updated_ms=clock_ms;
    board.direction=(vk_observation_t){true,speech,angle,clock_ms};
    (void)vk_runtime_route_generation();
}
int main(void){
    vk_runtime_init();vk_runtime_usb_start();hooks.connected(true);hooks.streaming(true);
#ifdef CONFIG_VOICEKEY_SPATIAL
    assert(vk_runtime_direction_enabled() && state.output_blocked);
#else
    assert(!vk_runtime_direction_enabled() && !state.output_blocked);
#endif
    state.audio_ok=true;capture_clock_ok=true;
    vk_spatial_config_t c=vk_spatial_defaults();c.enabled=true;c.count=2;
    c.regions[1]=(vk_region_t){90,25};c.policy=VK_FLOOR_MANUAL;
    assert(vk_runtime_voice_configure(&c) && vk_runtime_direction_enabled());
    int16_t samples[48],out[48];for(unsigned i=0;i<48;i++)samples[i]=1234;
    observe(0,true);assert(vk_runtime_floor_request(0,false));
    uint32_t audio=state.audio_epoch,route=vk_runtime_route_generation();
    vk_runtime_publish_routed(samples,48,audio,route);assert(state.usb.count==48 && state.wake.count==48);
    uint32_t old_wake=state.wake_epoch;
    observe(90,true);assert(state.output_blocked && !state.usb.count && !state.wake.count && spatial.owner==0);
    assert(state.audio_epoch==audio); /* direction transitions never reset physical RX */
    vk_runtime_publish_routed(samples,48,audio,route);assert(!state.wake.count); /* old read token */
    route=vk_runtime_route_generation();vk_runtime_publish_routed(samples,48,audio,route);
    assert(!state.usb.count && state.wake.count==48); /* all-seat keyword listener */
    assert(hooks.audio_packet(out)==16);for(unsigned i=0;i<16;i++)assert(!out[i]);
    vk_runtime_trigger(old_wake);assert(!state.pending && spatial.owner==0);
    assert(vk_runtime_floor_request(1,true) && spatial.owner==1 && !state.output_blocked);
    route=vk_runtime_route_generation();vk_runtime_publish_routed(samples,48,audio,route);assert(state.usb.count==48);
    observe(90,false);assert(spatial.owner==1 && state.output_blocked);
    vk_fifo_clear(&state.wake);route=vk_runtime_route_generation();
    vk_runtime_publish_routed(samples,48,audio,route);
    uint32_t epoch;assert(vk_runtime_wake_read(out,48,&epoch)==48);
    for(unsigned i=0;i<48;i++)assert(!out[i]);
    assert(vk_runtime_floor_release(1) && spatial.owner==-1);
    c.policy=VK_FLOOR_KEYWORD;assert(vk_runtime_voice_configure(&c));
    for(unsigned i=0;i<7;i++)observe(0,true);
    vk_runtime_trigger(state.wake_epoch);assert(spatial.owner==0 && state.pending);
    state.pending=false;uint64_t triggers=state.triggers;
    old_wake=state.wake_epoch;
    for(unsigned i=0;i<7;i++)observe(90,true);
    vk_runtime_trigger(old_wake);assert(spatial.owner==0);
    vk_runtime_trigger(state.wake_epoch);assert(spatial.owner==1 && !state.pending && state.triggers==triggers);
    old_wake=state.wake_epoch;
    assert(vk_runtime_floor_request(0,true));
    vk_runtime_trigger(old_wake);assert(spatial.owner==0); /* explicit grant beats delayed keyword */
    assert(vk_runtime_floor_request(1,true));
    uint8_t diag[VK_DIAGNOSTIC_SIZE];hooks.diagnostics(diag,sizeof(diag));
    assert(diag[0]==5 && (diag[192]&15)==15 && diag[194]==2 && diag[195]==2 && diag[212]==2);
    hooks.connected(false);assert(spatial.owner==-1);
    for(unsigned i=0;i<7;i++)observe(90,true);
    vk_runtime_trigger(state.wake_epoch);assert(spatial.owner==-1 && !state.pending);
    hooks.connected(true);hooks.streaming(true);assert(vk_runtime_floor_request(1,true));
    old_wake=state.wake_epoch;board.muted=true;(void)vk_runtime_route_generation();
    assert(state.muted && spatial.owner==-1 && state.output_blocked && !state.wake.count && !state.usb.count);
    vk_runtime_trigger(old_wake);assert(!state.pending);
    board.muted=false;observe(90,true);assert(spatial.owner==-1);
    assert(vk_runtime_floor_request(1,true));clock_ms+=101;(void)vk_runtime_route_generation();
    assert(spatial.owner==-1 && state.output_blocked && listener_region==-1);
    hooks.diagnostics(diag,sizeof(diag));assert(!(diag[192]&10));
    c.observe_only=true;assert(vk_runtime_voice_configure(&c));
    observe(180,true); /* unregistered direction remains audible in observation-only mode */
    vk_runtime_publish(samples,48,state.audio_epoch);
    assert(!state.output_blocked && state.usb.count==48 && state.wake.count==48 && spatial.owner==-1);
    assert(!vk_runtime_floor_request(0,true));
    hooks.diagnostics(diag,sizeof(diag));assert((diag[192]&17)==17);
    c.observe_only=false;
    c.enabled=false;assert(vk_runtime_voice_configure(&c));
    assert(!vk_runtime_direction_enabled() && !state.output_blocked);
    vk_runtime_publish(samples,48,state.audio_epoch);assert(state.usb.count==48 && state.wake.count==48);
    puts("spatial runtime: PASS (independent listener/output, tokens, floor, stale inference, mute, diagnostics)");
}

void vk_board_led_override(bool enabled,uint8_t r,uint8_t g,uint8_t b) {(void)enabled;(void)r;(void)g;(void)b;}

void vk_board_led_config(uint8_t mask,const uint8_t *config) {(void)mask;(void)config;}

void vk_board_led_command(const uint8_t *report,size_t n) {(void)report;(void)n;}
void vk_board_led_diagnostics(uint8_t *out) {memset(out,0,160);}
