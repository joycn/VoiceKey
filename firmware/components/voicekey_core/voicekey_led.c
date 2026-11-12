#include "voicekey_led.h"
#include <string.h>
static uint16_t u16(const uint8_t *p) { return p[0] | (uint16_t)p[1]<<8; }
static uint32_t u32(const uint8_t *p) { return u16(p) | (uint32_t)u16(p+2)<<16; }
static void put32(uint8_t *p,uint32_t v) { for(unsigned i=0;i<4;i++)p[i]=v>>(i*8); }
static const uint8_t offsets[]={0,1,2,3,4,7,13,49};
void vk_led_init(vk_led_t *s) {
    memset(s,0,sizeof(*s)); s->manual[0]=3;s->manual[1]=30;s->manual[2]=1;
    s->manual[4]=s->manual[5]=s->manual[6]=255;
    s->event_region=-1;
}
bool vk_led_patch_valid(uint8_t mask,const uint8_t *c) {
    if(!c || !mask || (mask & ~VK_LED_FIELD_MASK) || ((mask&1)&&c[0]>5) || ((mask&8)&&c[3]>1))return false;
    for(unsigned f=0;f<7;f++)if(!(mask&(1u<<f)))
        for(unsigned j=offsets[f];j<offsets[f+1];j++)if(c[j])return false;
    return true;
}
void vk_led_patch(vk_led_t *s,uint8_t mask,const uint8_t *c) {
    if(!vk_led_patch_valid(mask,c))return;
    for(unsigned f=0;f<7;f++)if(mask&(1u<<f))memcpy(s->manual+offsets[f],c+offsets[f],offsets[f+1]-offsets[f]);
    s->manual_on=true;
}
void vk_led_legacy(vk_led_t *s,bool enabled,uint8_t r,uint8_t g,uint8_t b) {
    s->manual_on=enabled;s->temporary=0;s->request_id=0;s->outcome=VK_LED_NONE;
    if(enabled){uint8_t c[49]={r||g||b?3:0,30,1,0,r,g,b};vk_led_patch(s,31,c);}
    else s->event_until=0;
}
static unsigned distance(unsigned a,unsigned b) { unsigned d=a>b?a-b:b-a;return d>180?360-d:d; }
static bool layout(vk_led_t *s,const uint8_t *b) {
    unsigned count=b[10],zero=u16(b+11);uint16_t masks[8]={0},used=0;
    if(!count||count>8||zero>=360||b[13]>1)return false;
    for(unsigned i=0;i<count;i++) {
        const uint8_t *p=b+14+i*5;unsigned region=p[0],center=u16(p+1),width=u16(p+3);
        if(region>=8||masks[region]||center>=360||!width||width>360)return false;
        uint16_t mask=0;unsigned nearest=0,best=361;
        for(unsigned j=0;j<12;j++) {
            unsigned angle=(zero+(b[13]?360-j*30:j*30))%360;
            unsigned d=distance(angle,center);
            if(d<best){best=d;nearest=j;}
            if(d*2<=width)mask|=1u<<j;
        }
        if(!mask)mask=1u<<nearest;
        if(mask&used)return false;
        used|=mask;masks[region]=mask;
    }
    for(unsigned i=14+count*5;i<54;i++)if(b[i])return false;
    memcpy(s->masks,masks,sizeof(masks));s->layout_count=count;s->layout_token=u32(b+54);
    return true;
}
bool vk_led_command(vk_led_t *s,const uint8_t *b,size_t n,uint64_t now) {
    if(!b||n!=VK_LED_REPORT_SIZE||memcmp(b,"VKLC",4)||b[4]!=3||!u32(b+6))return false;
    s->request_id=u32(b+6);s->request_op=b[5];s->outcome=VK_LED_FAILED;s->reason=4;
    unsigned end;
    switch(b[5]) { case 1:end=10;break;case 2:end=60;break;case 3:end=58;break;case 4:end=15;break;case 5:end=16;break;default:return false; }
    for(unsigned i=end;i<n;i++)if(b[i])return false;
    if(b[5]==1){s->manual_on=false;s->temporary=0;s->event_until=0;}
    else if(b[5]==2){if(!vk_led_patch_valid(b[10],b+11))return false;vk_led_patch(s,b[10],b+11);}
    else if(b[5]==3){if(!layout(s,b))return false;s->temporary=0;s->event_until=0;}
    else {
        uint32_t duration=u32(b+(b[5]==4?11:12));
        if(duration<100||duration>60000)return false;
        if(b[5]==4){if(b[10]>=12)return false;}
        else if(b[10]>=8||!s->masks[b[10]]||b[11]>3)return false;
        s->temporary=b[5];s->target=b[10];s->preview=b[5]==5?b[11]:0;
        s->started=now;s->until=now+duration;
    }
    s->outcome=b[5]==3?VK_LED_APPLIED:VK_LED_RECEIVED;s->reason=0;
    return true;
}
static void solid(vk_led_frame_t *f,uint32_t rgb) {
    memset(f,0,sizeof(*f));f->config[0]=rgb?3:0;f->config[1]=30;f->config[2]=1;
    f->config[4]=rgb>>16;f->config[5]=rgb>>8;f->config[6]=rgb;
}
static void arc(vk_led_frame_t *f,uint16_t mask,unsigned level,bool denied,bool on) {
    memset(f,0,sizeof(*f));f->config[0]=5;f->config[1]=255;f->config[2]=1;
    for(unsigned j=0;j<12;j++)if(on&&(mask&(1u<<j))) {
        f->config[13+j*3]=denied?level:0;
        f->config[14+j*3]=denied?level/2:0;
        f->config[15+j*3]=denied?0:level;
    }
}
static bool flash(uint64_t age) { return age<150 || (age>=300 && age<450); }
void vk_led_render(vk_led_t *s,const vk_led_inputs_t *in,uint32_t rgb,uint64_t now,vk_led_frame_t *f) {
    if(s->seen_inputs && s->was_connected && !in->connected) {
        s->temporary=0;s->event_until=0;
        if(s->request_op==4||s->request_op==5){s->outcome=VK_LED_EXPIRED;s->reason=6;}
    }
    if(s->temporary && now>=s->until){s->temporary=0;if(s->request_op==4||s->request_op==5){s->outcome=VK_LED_EXPIRED;s->reason=7;}}
    if(in->spatial&&in->valid&&in->owner>=0&&in->owner<8 &&
       (!s->seen_inputs||in->floor_generation!=s->floor_generation)) {
        s->event_region=in->owner;s->event=2;s->event_start=now;s->event_until=now+600;
    }
    if(in->spatial&&in->valid&&in->denied_region>=0&&in->denied_region<8&&
       in->denied_generation!=s->denied_generation) {
        s->event_region=in->denied_region;s->event=3;s->event_start=now;s->event_until=now+600;
    }
    s->seen_inputs=true;s->was_connected=in->connected;s->floor_generation=in->floor_generation;s->denied_generation=in->denied_generation;
    s->source=VK_LED_IDLE;s->display_target=0;s->remaining=s->temporary?s->until-now:0;
    solid(f,rgb);
    if(rgb==0x202020){f->config[0]=1;f->config[1]=255;f->config[4]=f->config[5]=f->config[6]=255;f->preserve_gamma=true;}
    else if(rgb==0x000020)f->config[0]=2;
    if(in->fault){solid(f,0x200020);s->source=VK_LED_FAULT;}
    else if(in->muted){solid(f,0x200000);s->source=VK_LED_MUTE;}
    else if(s->temporary) {
        s->remaining=s->until-now;s->source=s->temporary==4?VK_LED_IDENTIFY:VK_LED_PREVIEW;
        uint16_t mask=s->temporary==4?1u<<s->target:s->masks[s->target];
        s->display_target=s->temporary==5?s->target+1:0;
        arc(f,mask,s->temporary==5&&s->preview==0?15:30,s->preview==3,
            s->temporary==4||s->preview<2||flash(now-s->started));
    } else if(s->manual_on){memcpy(f->config,s->manual,49);f->preserve_gamma=false;s->source=VK_LED_MANUAL;}
    else if(in->spatial&&in->valid) {
        if(now<s->event_until && s->event_region>=0 && s->masks[s->event_region]) {
            s->source=VK_LED_EVENT;s->display_target=s->event_region+1;s->remaining=s->event_until-now;
            arc(f,s->masks[s->event_region],30,s->event==3,flash(now-s->event_start));
        } else if(in->owner>=0&&in->owner<8&&s->masks[in->owner]) {
            s->source=in->receiving?VK_LED_RECEIVING:VK_LED_OWNER;s->display_target=in->owner+1;
            arc(f,s->masks[in->owner],in->receiving?30:15,false,true);
        }
    }
    if(s->request_id && s->outcome!=VK_LED_FAILED && s->outcome!=VK_LED_EXPIRED && s->request_op!=3) {
        bool covered=in->fault||in->muted||(s->request_op==2&&s->temporary);
        if(covered){s->outcome=VK_LED_COVERED;s->reason=in->fault?1:in->muted?2:8;}
        else {s->reason=0;s->outcome=s->applied_valid&&!memcmp(f,&s->applied,sizeof(*f))?VK_LED_APPLIED:VK_LED_RECEIVED;}
    }
}
void vk_led_applied(vk_led_t *s,const vk_led_frame_t *f,bool ok) {
    s->applied_valid=ok;
    if(ok)s->applied=*f;
    if(s->request_id && s->outcome==VK_LED_RECEIVED){s->outcome=ok?VK_LED_APPLIED:VK_LED_FAILED;s->reason=ok?0:5;}
}
void vk_led_diagnostics(const vk_led_t *s,uint8_t *b) {
    memset(b,0,160);put32(b,s->request_id);b[4]=s->outcome;
    b[5]=s->temporary==4?3:s->temporary==5?2:s->manual_on?1:0;
    b[6]=s->source;b[7]=s->display_target;put32(b+8,s->remaining);b[12]=s->reason;
    b[13]=s->applied_valid;b[14]=s->layout_count;b[15]=s->preview;
    memcpy(b+16,s->applied.config,49);b[65]=s->applied.preserve_gamma;
    put32(b+68,s->layout_token);
    for(unsigned i=0;i<8;i++){b[72+i*2]=s->masks[i];b[73+i*2]=s->masks[i]>>8;}
    memcpy(b+104,s->manual,49);
}
