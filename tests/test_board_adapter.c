/* Runs the real board task through I2C failures and LED retries. */
#include "../firmware/main/board.c"
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
static void (*board_task)(void *);
static jmp_buf finished;
static unsigned cycle, publishes, led_writes[240], versions;
static uint8_t request[51], ring_color[48], doa_color[8], color[4], effect, brightness, speed, led_gamma=1;
static unsigned settle_ticks, settling_delays, forced_busy, retry_delays;
static unsigned direction_reads,ring_writes,last_ring_cycle;
bool vk_runtime_direction_enabled(void) { return cycle>=150; }
void vTaskDelay(TickType_t ticks) {
    assert(ticks==2 || ticks==10);
    if(ticks==10)retry_delays++;
    else {settle_ticks+=ticks; settling_delays++;}
}
int64_t esp_timer_get_time(void) { return 100000+(int64_t)cycle*20000; }
TickType_t xTaskGetTickCount(void) { return 100; }
void vk_runtime_inputs(void) {
    vk_board_status_t s;vk_board_status(&s);publishes++;
    if(cycle<150) assert(!direction_reads && !s.direction.valid);
    else if(cycle==170) assert(!s.direction.valid && s.valid);
    else assert(s.direction.valid && s.direction.speech && s.direction.degrees==90 && s.direction.at_ms==100+cycle*20);
    if(cycle==12 || cycle==14)assert(effect==3 && color[0]==0x56 && color[1]==0x34 && color[2]==0x12);
    if(cycle==13)assert(effect==3 && color[2]==32 && color[1]==0 && color[0]==0); /* mute wins */
    if(cycle==15)assert(effect==3 && color[0]==32 && color[1]==32 && color[2]==32); /* no breathe token */
    if(cycle==16)assert(effect==0);
    if(cycle==17)assert(effect==1); /* auto restores */
    if(cycle==0 || cycle==4 || cycle==7)assert(effect==2);
    if(cycle==6)assert(effect==1 && color[0]==255 && color[1]==255 && color[2]==255);
    if(cycle==8 || cycle==9)assert(effect==3);
    if(cycle>=4 && cycle<71 && !(cycle>=20 && cycle<=39))assert(brightness==(effect==1 ? 255 : 30) && led_gamma==0);
    if(cycle==0 || cycle==4 || cycle==7)assert(speed==1);
    if(cycle==6 || cycle==10 || cycle==11)assert(speed==1);
    if(cycle==8 || cycle==9)assert(speed==1);

    if(cycle==31)assert(effect==4 && doa_color[0]==3 && doa_color[1]==2 && doa_color[2]==1 && doa_color[4]==6);
    if(cycle==33)assert(effect==1 && brightness==99 && speed==7 && led_gamma==1 && color[0]==255 && !color[1] && !color[2]);
    if(cycle==35)assert(effect==1 && brightness==120 && speed==7 && led_gamma==1);
    if(cycle>=230)assert(s.led_valid && s.led_settings[0]==1 && s.led_settings[1]==255 && s.led_settings[2]==1 && s.led_settings[3]==1 && s.control.led_power);
    if(cycle>=50 && cycle<95)assert(s.agc_valid && s.agc_gain_bits==0x3d000000);
    if(cycle>=95 && cycle<145)assert(!s.agc_valid);
    if(cycle>=145)assert(s.agc_valid && s.agc_gain_bits==0x3d000000);
    assert(s.valid == (cycle==0 || cycle>=4));
}
void vTaskDelayUntil(TickType_t *previous,TickType_t period) {
    assert(period==20);*previous+=period;
    assert(publishes==cycle+1); if(cycle==5) vk_board_led(32,32,32);
    if(cycle==6) vk_board_led(0,0,32);
    if(cycle==7) vk_board_led(32,0,0);
    if(cycle==8) vk_board_led(32,0,32);
    if(cycle==9) vk_board_led(32,32,32);
    if(cycle==11)vk_board_led_override(true,0x12,0x34,0x56);
    if(cycle==14)vk_board_led_override(true,32,32,32);
    if(cycle==15)vk_board_led_override(true,0,0,0);
    if(cycle==16)vk_board_led_override(false,0,0,0);
    if(cycle==19){uint8_t c[49]={5,30,1,0};for(unsigned i=0;i<12;i++){c[13+i*3]=i;c[14+i*3]=i+1;c[15+i*3]=i+2;}vk_board_led_config(79,c);}
    if(cycle>=22&&cycle<29){uint8_t c[49]={0};c[13]=cycle;vk_board_led_config(64,c);}
    if(cycle==30){uint8_t c[49]={4,0,0,0,0,0,0,1,2,3,4,5,6};vk_board_led_config(33,c);}
    if(cycle==32){uint8_t c[49]={1,99,7,1,0,0,255};vk_board_led_config(31,c);}
    if(cycle==34){uint8_t c[49]={0,120};vk_board_led_config(2,c);}
    if(cycle==36)vk_board_led_override(true,32,0,0);
    if(cycle==38)vk_board_led_override(false,0,0,0);
    if(cycle==70){effect=5;brightness=127;speed=8;led_gamma=1;}
    if(++cycle==240)longjmp(finished,1);
}
int xTaskCreatePinnedToCore(void (*fn)(void *),const char *name,uint32_t stack,void *arg,unsigned priority,void *handle,int core) {
    (void)name;(void)stack;(void)arg;(void)priority;(void)handle;(void)core;board_task=fn;return pdPASS;
}
esp_err_t gpio_config(const gpio_config_t *c) { assert(c->pin_bit_mask==1 && c->mode==GPIO_MODE_INPUT);return ESP_OK; }
int gpio_get_level(int pin) { assert(pin==0);return 1; }
esp_err_t i2c_new_master_bus(const i2c_master_bus_config_t *c,i2c_master_bus_handle_t *b) { assert(c->sda_io_num==5 && c->scl_io_num==6);*b=(void *)1;return ESP_OK; }
esp_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t b,const i2c_device_config_t *c,i2c_master_dev_handle_t *d) { (void)b;assert(c->device_address==0x2c);*d=(void *)2;return ESP_OK; }
esp_err_t i2c_master_transmit(i2c_master_dev_handle_t d,const uint8_t *bytes,size_t n,int timeout) {
    (void)d;assert(timeout==10 && n<=sizeof(request));memcpy(request,bytes,n);
    settle_ticks=0;
    if(cycle>=2 && cycle<=4) { vk_board_status_t s;vk_board_status(&s);assert(!s.valid); } /* no premature healthy cache */
    if(cycle==1 && bytes[0]==20 && bytes[1]==0x80)return ESP_FAIL;
    if(!(bytes[1]&0x80) && bytes[0]==20) {
        led_writes[cycle]++;
        if(cycle==2 || cycle==3)return ESP_FAIL;
        if(bytes[1]==19){
            assert(n==51);if(ring_writes)assert(cycle-last_ring_cycle>=5);
            ring_writes++;last_ring_cycle=cycle;memcpy(ring_color,bytes+3,48);
        }
        else if(bytes[1]==17){assert(n==11);memcpy(doa_color,bytes+3,8);}
        else if(bytes[1]==16)memcpy(color,bytes+3,4);
        else if(bytes[1]==13)brightness=bytes[3];
        else if(bytes[1]==15) { assert(bytes[3]==1 || bytes[3]==7); speed=bytes[3]; }
        else if(bytes[1]==14) { assert(bytes[3]<=1); led_gamma=bytes[3]; }
        else {assert(bytes[1]==12);effect=bytes[3];brightness=127;}

    }
    return ESP_OK;
}
esp_err_t i2c_master_receive(i2c_master_dev_handle_t d,uint8_t *bytes,size_t n,int timeout) {
    (void)d;assert(timeout==10 && n==request[2]);memset(bytes,0,n);
    /* The asynchronous target cannot answer immediately after command write. */
    assert(settle_ticks>=2);
    uint8_t r=request[0],c=request[1]&127;
    /* First status read stays busy twice; each retry must yield again. */
    if(cycle==0 && r==20 && c==0 && forced_busy<2) {
        forced_busy++;bytes[0]=64;return ESP_OK;
    }
    if(r==48 && c==0){versions++;bytes[1]=1;bytes[3]=8;}
    else if(r==48 && c==1)memcpy(bytes+1,"fixture INT build",17);
    else if(r==48 && c==8)assert(n==3);
    else if(r==35 && c==13)assert(n==3);
    else if(r==35 && c==14){bytes[1]=1;bytes[2]=1;}
    else if(r==33 && c==3){bytes[1]=1;assert(n==5);}
    else if(r==33 && c==70)assert(n==2);
    else if((r==35 && (c==0 || c==1 || c==26)) || (r==17 && (c==11 || c==30)) || (r==33 && c==36)) {
        assert(n==5);
        uint32_t v=r==35 ? (c==0?0x42b40000:c==1?0x41000000:12) :
                   r==17 ? (c==11?0x42800000:0x44a28000) : 0x3f800000;
        for(unsigned i=0;i<4;i++)bytes[1+i]=(uint8_t)(v>>(8*i));
    }
    else if(r==35 && c==15){bytes[1]=6;bytes[2]=3;}
    else if(r==20 && c==19)memcpy(bytes+1,ring_color,48);
    else if(r==20 && c==17)memcpy(bytes+1,doa_color,8);
    else if(r==20 && c==16)memcpy(bytes+1,color,4);
    else if(r==20 && c==13)bytes[1]=brightness;
    else if(r==20 && c==15)bytes[1]=speed;
    else if(r==20 && c==14)bytes[1]=led_gamma;
    else if(r==20 && c==0){bytes[4]=1;if(cycle==13)bytes[2]=1;}
    else if(r==20 && c==12)bytes[1]=effect;
    else if(r==20 && c==18){assert(n==5);direction_reads++;if(cycle==170)return ESP_FAIL;bytes[1]=90;bytes[3]=1;}
    else if(r==17 && c==13){assert(n==5);if(cycle==95)return ESP_FAIL;bytes[4]=0x3d;}
    else assert((r==20 && c==0) || (r==35 && (c==10 || c==24)));
    return ESP_OK;
}
int main(void) {
    assert(vk_board_init()==ESP_OK && vk_board_xmos_init()==ESP_OK && board_task);
    vk_board_led(0,0,32);
    if(!setjmp(finished))board_task(NULL);
    assert(versions==2 && led_writes[0]==5 && led_writes[1]==0 && led_writes[2]==1 && led_writes[3]==1 && led_writes[4]==5 && led_writes[5]==0);
    unsigned recovered_writes=0;
    for(unsigned i=71;i<240;i++)recovered_writes+=led_writes[i];
    assert(recovered_writes==4); /* One restoration, no periodic animation restart. */
    assert(forced_busy==2 && settling_delays>2 && cache.busy==2 && retry_delays==2);
    assert(direction_reads==90 && ring_writes>=2 && ring_writes<=3);
    puts("PASS: actual board task coherent cache publication, persistent LED failure and LED reapplication after control reconfiguration");
}

void vk_runtime_led_inputs(vk_led_inputs_t *out) {*out=(vk_led_inputs_t){.connected=true,.owner=-1,.denied_region=-1};}
