#include "voicekey_led.h"
#include "voicekey_xvf.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t b[384];static uint32_t seq;
static void p16(uint8_t *p,unsigned v){p[0]=v;p[1]=v>>8;}
static void p32(uint8_t *p,unsigned v){p16(p,v);p16(p+2,v>>16);}
static void cmd(unsigned op){memset(b,0,sizeof(b));memcpy(b,"VKLC",4);b[4]=3;b[5]=op;p32(b+6,++seq);}
static void layout(vk_led_t *s) {
    cmd(3);b[10]=4;
    for(unsigned i=0;i<4;i++){b[14+i*5]=i;p16(b+15+i*5,i*90);p16(b+17+i*5,40);}
    assert(vk_led_command(s,b,sizeof(b),0));
    assert(s->masks[0]==1 && s->masks[1]==8 && s->masks[2]==64 && s->masks[3]==512);
}
static void geometry(void) {
    vk_led_t s;vk_led_init(&s);layout(&s);vk_led_t previous=s;
    p16(b+20,0);assert(!vk_led_command(&s,b,sizeof(b),0));assert(!memcmp(s.masks,previous.masks,sizeof(s.masks)));
    layout(&s);b[13]=1;assert(vk_led_command(&s,b,sizeof(b),0));assert(s.masks[1]==512 && s.masks[3]==8);
    cmd(3);b[10]=1;p16(b+15,359);p16(b+17,1);assert(vk_led_command(&s,b,sizeof(b),0));assert(s.masks[0]==1);
    p16(b+15,0);p16(b+17,80);assert(vk_led_command(&s,b,sizeof(b),0));assert(s.masks[0]==(1|2|2048));
    p16(b+11,360);assert(!vk_led_command(&s,b,sizeof(b),0));
    cmd(4);b[10]=12;p32(b+11,1000);assert(!vk_led_command(&s,b,sizeof(b),0));
    b[10]=0;p32(b+11,99);assert(!vk_led_command(&s,b,sizeof(b),0));
    p32(b+11,1000);b[383]=1;assert(!vk_led_command(&s,b,sizeof(b),0));
}
static void behavior(void) {
    vk_led_t s;vk_led_init(&s);layout(&s);vk_led_frame_t f;
    vk_led_inputs_t in={.connected=true,.spatial=true,.valid=true,.owner=0,.denied_region=-1,.floor_generation=1};
    vk_led_render(&s,&in,0x202020,0,&f);assert(s.source==VK_LED_EVENT);
    vk_led_render(&s,&in,0x202020,700,&f);assert(s.source==VK_LED_OWNER && f.config[15]==15 && !f.config[24]);
    in.receiving=true;vk_led_render(&s,&in,0x202020,800,&f);assert(s.source==VK_LED_RECEIVING && f.config[15]==30);
    in.receiving=false;vk_led_render(&s,&in,0x202020,900,&f);assert(s.display_target==1); /* other voice does not move owner */
    cmd(5);b[10]=1;b[11]=1;p32(b+12,1000);assert(vk_led_command(&s,b,sizeof(b),1000));
    vk_led_render(&s,&in,0x202020,1000,&f);assert(s.source==VK_LED_PREVIEW&&f.config[24]==30);
    assert(in.owner==0 && in.floor_generation==1); /* rendering never mutates audio */
    vk_led_applied(&s,&f,true);assert(s.outcome==VK_LED_APPLIED);
    in.muted=true;vk_led_render(&s,&in,0x202020,1100,&f);assert(s.source==VK_LED_MUTE && s.outcome==VK_LED_COVERED);
    in.fault=true;vk_led_render(&s,&in,0x202020,1200,&f);assert(s.source==VK_LED_FAULT);
    in.muted=in.fault=false;in.owner=2;in.floor_generation++;
    vk_led_render(&s,&in,0x202020,1300,&f);assert(s.source==VK_LED_PREVIEW);
    vk_led_render(&s,&in,0x202020,2100,&f);assert(s.source==VK_LED_OWNER && s.display_target==3 && s.outcome==VK_LED_EXPIRED);
    cmd(5);b[10]=1;b[11]=3;p32(b+12,1000);assert(vk_led_command(&s,b,sizeof(b),2200));
    vk_led_render(&s,&in,0x202020,2200,&f);assert(f.config[22]==30 && f.config[23]==15 && !f.config[24]);
    in.connected=false;vk_led_render(&s,&in,0x202020,2300,&f);assert(!s.temporary && s.outcome==VK_LED_EXPIRED && s.reason==6);
    in.valid=false;vk_led_render(&s,&in,0x202020,2400,&f);assert(s.source==VK_LED_IDLE);
    cmd(2);b[10]=2;b[12]=17;assert(vk_led_command(&s,b,sizeof(b),2500));
    assert(s.manual[1]==17 && s.manual[0]==3);
    vk_led_render(&s,&in,0x202020,2500,&f);assert(s.source==VK_LED_MANUAL && f.config[1]==17);
    vk_led_applied(&s,&f,false);assert(s.outcome==VK_LED_FAILED && s.reason==5);
    cmd(1);assert(vk_led_command(&s,b,sizeof(b),2600));vk_led_render(&s,&in,0x202020,2600,&f);assert(s.source==VK_LED_IDLE);
    uint8_t status[160];vk_led_diagnostics(&s,status);assert(!status[5]);
}
static uint8_t stored[48];static unsigned transfers;static bool corrupt;
static int transfer(void *ctx,const uint8_t *tx,size_t nt,uint8_t *rx,size_t nr) {
    (void)ctx;transfers++;assert(tx[0]==20&&(tx[1]&127)==19);
    if(!nr){assert(nt==51&&tx[2]==48);memcpy(stored,tx+3,48);return 0;}
    assert(nt==3&&nr==49&&tx[2]==49);rx[0]=0;memcpy(rx+1,stored,48);if(corrupt)rx[48]^=1;return nr;
}
int main(void) {
    geometry();behavior();
    vk_xvf_t x={.transfer=transfer};uint8_t data[49];for(unsigned i=0;i<49;i++)data[i]=i;
    assert(vk_xvf_set(&x,20,19,data,48));assert(transfers==2);
    assert(!vk_xvf_set(&x,20,19,data,49));assert(transfers==2);
    corrupt=true;assert(!vk_xvf_set(&x,20,19,data,48));
    puts("PASS: LED layout geometry, priority, expiry, local floor binding, rejection and 48-byte verified ring writes");
}
