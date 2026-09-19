/* Actual capture task, registered driver callbacks, and runtime admission. */
#include "../firmware/main/runtime.c"
#include "../firmware/main/audio_input.c"
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
static vk_usb_hooks_t hooks;
static i2s_event_callbacks_t tx_cb, rx_cb;
static void (*capture_task)(void *);
static jmp_buf finished;
static unsigned read_step, rx_disables, rx_enables;
static uint64_t clock_us = 100000;
static bool mic_muted, clock_fault_next_poll;
static int32_t buffers[3][96];
static unsigned dma_index;
static int16_t host[288];
static int16_t pre_mute_pcm[48];
static void dma_tick(void) {
    clock_us += 1000;
    i2s_event_data_t e = {.dma_buf=buffers[dma_index++%3],.size=sizeof(buffers[0])};
    assert(tx_cb.on_sent && !tx_cb.on_sent(tx,&e,NULL));
}
int64_t esp_timer_get_time(void) { return clock_us; }
size_t esp_psram_get_size(void) { return 8u*1024*1024; }
uint32_t esp_get_free_heap_size(void) { return 100000; }
uint32_t esp_get_minimum_free_heap_size(void) { return 90000; }
const esp_app_desc_t *esp_app_get_description(void) { static esp_app_desc_t d; return &d; }
void vk_board_status(vk_board_status_t *b) {
    if(clock_fault_next_poll){clock_us+=6000;clock_fault_next_poll=false;}
    *b=(vk_board_status_t){.valid=true,.muted=mic_muted,.i2s_active=true,.updated_ms=clock_us/1000}; }
bool vk_board_muted(void) { return mic_muted; }
bool vk_board_button(void) { return false; }
void vk_board_led(uint8_t r,uint8_t g,uint8_t b) { (void)r;(void)g;(void)b; }
esp_err_t vk_usb_start(const vk_usb_hooks_t *h) { hooks=*h; return ESP_OK; }
int xTaskCreatePinnedToCore(void (*fn)(void *),const char *name,uint32_t stack,void *arg,unsigned priority,void *handle,int core) {
    (void)name;(void)stack;(void)arg;(void)priority;(void)handle;(void)core;capture_task=fn;return pdPASS;
}
void vTaskDelay(TickType_t delay) { for (unsigned i=0;i<delay;++i) dma_tick(); }
esp_err_t i2s_new_channel(const i2s_chan_config_t *c,i2s_chan_handle_t *t,i2s_chan_handle_t *r) {
    assert(c->dma_desc_num==3 && c->dma_frame_num==48 && c->auto_clear_before_cb);*t=(void *)1;*r=(void *)2;return ESP_OK;
}
esp_err_t i2s_channel_init_std_mode(i2s_chan_handle_t h,const i2s_std_config_t *c) {
    (void)h;assert(c->clk_cfg.rate==48000 && c->gpio_cfg.bclk==8 && c->gpio_cfg.ws==7 && c->gpio_cfg.din==43 && c->gpio_cfg.dout==44);return ESP_OK;
}
esp_err_t i2s_channel_register_event_callback(i2s_chan_handle_t h,const i2s_event_callbacks_t *c,void *arg) { (void)arg;if(h==tx)tx_cb=*c;else rx_cb=*c;return ESP_OK; }
esp_err_t i2s_channel_enable(i2s_chan_handle_t h) { if(h==rx)rx_enables++;return ESP_OK; }
esp_err_t i2s_channel_disable(i2s_chan_handle_t h) { assert(h==rx);rx_disables++;return ESP_OK; }
esp_err_t i2s_channel_read(i2s_chan_handle_t h,void *data,size_t requested,size_t *bytes,uint32_t timeout) {
    assert(h==rx && requested==1152 && timeout==20);
    unsigned step=read_step++;
    if(step==1) assert(state.audio_ok && state.wake.count==0 && rx_disables>=2);
    if(step==2) {
        assert(state.wake.count==48);
        for(unsigned i=0;i<48;i++)pre_mute_pcm[i]=state.wake.data[(state.wake.head+i)%state.wake.capacity];
    }
    if(step==3) assert(!state.audio_ok && state.wake.count==0);
    if(step==4) assert(state.audio_ok && state.wake.count==0);
    if(step==5) assert(state.wake.count==48);
    if(step==6) assert(!state.audio_ok && !state.wake.count);
    if(step==7) assert(state.audio_ok && !state.wake.count);
    if(step==8) assert(state.wake.count==24);
    if(step==9) assert(!state.audio_ok && !state.wake.count);
    if(step==10) assert(state.audio_ok && !state.wake.count);
    if(step==11) assert(!state.audio_ok && !state.wake.count && !playback.active); /* fault BETWEEN reads */
    if(step==12) assert(state.audio_ok && !state.wake.count && playback.active);
    if(step==13) {
        assert(state.wake.count==48 && playback.active);
        mic_muted=true; vk_runtime_inputs(); assert(!state.wake.count);
    }
    if(step==14) { assert(state.muted && !state.wake.count); mic_muted=false; vk_runtime_inputs(); }
    if(step==17) assert(!state.audio_ok && !state.wake.count);
    if(step==18) assert(state.audio_ok && !state.wake.count);
    if(step==19) {
        assert(state.wake.count==48 && playback.active && rx_disables>=6);
        int32_t fresh[288];int16_t expected[48]; vk_decimator_t fir;vk_decimator_reset(&fir);
        for(unsigned i=0;i<288;i++)fresh[i]=(i%2)?0:0x10000000;
        assert(vk_decimate(&fir,fresh,144,expected,1)==48);
        for(unsigned i=0;i<48;i++)assert(state.wake.data[(state.wake.head+i)%state.wake.capacity]==expected[i]);
        longjmp(finished,1);
    }
    if(step==11) for(unsigned i=0;i<251;i++)dma_tick(); /* clock requalification during recovery read */
    hooks.playback_packet(host,144);
    for(unsigned i=0;i<3;i++)dma_tick(); /* TX stays healthy through RX-only errors. */
    *bytes=requested;
    for(unsigned i=0;i<requested/4;i++)((int32_t *)data)[i]=(i%2)?0:0x10000000;
    if(step==2) {
        uint32_t inference=state.wake_epoch, generation=vk_runtime_capture_generation();
        assert(rx_cb.on_recv_q_ovf && !rx_cb.on_recv_q_ovf(rx,NULL,NULL));
        assert(!state.audio_ok && !state.usb.count && !state.wake.count && playback.active);
        vk_runtime_trigger(inference); assert(!state.pending); /* other-core completion BEFORE task resumes */
        assert(!vk_runtime_capture_recover(generation));
        int16_t silence[17];hooks.audio_packet(silence);for(unsigned i=0;i<16;i++)assert(silence[i]==0);
    }
    if(step==10)clock_fault_next_poll=true;
    if(step==16) {
        assert(state.wake.count==48);
        /* Identical incoming PCM must have identical amplitude after mute.
           Exercise the real capture task and runtime, including epoch/FIR reset. */
        bool nonzero=false;
        for(unsigned i=0;i<48;i++) {
            assert(state.wake.data[(state.wake.head+i)%state.wake.capacity]==pre_mute_pcm[i]);
            if(pre_mute_pcm[i])nonzero=true;
        }
        assert(nonzero);
        clock_us+=6000;vk_runtime_inputs();assert(!state.audio_ok && !playback.active && !playback.count);
        hooks.playback_packet(host,144);assert(!playback.count);
        dma_tick();assert(!playback.active);dma_tick();assert(!playback.active && !playback.count);
        for(unsigned i=0;i<250;i++)dma_tick();
        /* This fault happened DURING the registered read, so generation rejects it. */
    }
    if(step==5)*bytes=7; /* malformed partial frame */
    if(step==6)*bytes=75*8; /* valid partial read accepted as recovery block, discarded */
    if(step==7)*bytes=73*8; /* valid partial read: preserves FIR phase */
    if(step==8){*bytes=20*8;return ESP_ERR_TIMEOUT;} /* timeout even with partial bytes discards */
    return ESP_OK;
}
int main(void) {
    for(unsigned i=0;i<288;i++)host[i]=100;
    vk_runtime_init();assert(vk_runtime_usb_start()==ESP_OK);hooks.connected(true);hooks.streaming(true);
    assert(vk_audio_start()==ESP_OK && capture_task && tx_cb.on_sent && rx_cb.on_recv_q_ovf);
    hooks.playback_active(true);dma_tick();dma_tick();assert(!playback.active);
    for(unsigned i=0;i<250;i++)dma_tick();
    assert(capture_clock_ok);
    if(!setjmp(finished))capture_task(NULL);
    hooks.playback_packet(host,144);dma_tick();assert(playback.count>0);
    clock_us+=6000;vk_runtime_inputs();assert(!transport_ready && !playback.active && !playback.count && !state.audio_ok);
    for(unsigned i=0;i<3;i++)for(unsigned j=0;j<96;j++)assert(buffers[i][j]==0);
    hooks.playback_packet(host,144);assert(playback.count==0);
    dma_tick();assert(!transport_ready && !playback.active); /* TX restarts BEFORE paused RX task */
    hooks.playback_packet(host,144);assert(!playback.count);
    dma_tick();assert(transport_ready && !playback.active && !playback.count && !state.audio_ok);
    for(unsigned i=0;i<251;i++)dma_tick();
    assert(playback.active && !playback.count);
    for(unsigned i=0;i<3;i++)for(unsigned j=0;j<96;j++)assert(buffers[i][j]==0);
    for(unsigned i=0;i<288;i++)host[i]=200;
    hooks.playback_packet(host,144);dma_tick();assert(buffers[(dma_index-1)%3][0]==200*65536);
    assert(vk_runtime_capture_recover(vk_runtime_capture_generation()));
    mic_muted=true;vk_runtime_inputs();assert(playback.active && transport_ready);
    /* Completion callbacks cannot retract bytes consumed before they run.
       Model the boundary: the exact-5ms poll does not expire the clock, and
       a restart before the next poll can consume a prefilled DMA block. */
    for(unsigned i=0;i<3;i++){hooks.playback_packet(host,144);dma_tick();}
    clock_us=(uint64_t)last_dma_us+5000;vk_runtime_inputs();assert(transport_ready);
    assert(buffers[dma_index%3][0]==200*65536); /* data hardware can consume BEFORE callback */
    dma_tick();assert(!transport_ready && !playback.active && !playback.count);
    for(unsigned i=0;i<3;i++)for(unsigned j=0;j<96;j++)assert(buffers[i][j]==0);
    puts("PASS: actual audio task/callback overflow admission, partial/timeout reads, RX epochs/recovery/FIR reset and TX-before-RX clock recovery without stale replay");
}
