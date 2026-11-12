#include "voicekey_xvf.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned calls, waits, mode;
static int transfer(void *ctx,const uint8_t *tx,size_t nt,uint8_t *rx,size_t nr) {
    (void)ctx; calls++; assert(nt==3 && tx[0]==20 && tx[1]==(18|128) && tx[2]==5 && nr==5);
    memset(rx,0,nr); rx[1]=0x67; rx[2]=1; rx[3]=1; /*359 degrees*/
    if(mode==1) return -1;
    if(mode==2) return 4;
    if(mode==3) rx[0]=64;
    if(mode==4) rx[1]=0x68; /*360 invalid*/
    if(mode==5) rx[3]=2;
    if(mode==6 && calls==1) rx[0]=64;
    return 5;
}
static void retry(void *ctx){(void)ctx;waits++;}
int main(void) {
    vk_xvf_t x={.transfer=transfer,.retry_wait=retry};
    vk_observation_t o=vk_xvf_direction(&x,42);
    assert(o.valid && o.speech && o.degrees==359 && o.at_ms==42);
    for(mode=1;mode<=5;mode++) {
        calls=waits=0; o=vk_xvf_direction(&x,100); assert(!o.valid && o.at_ms==100);
        if(mode==3) assert(calls==3 && waits==2);
    }
    mode=6;calls=waits=0;o=vk_xvf_direction(&x,200);assert(o.valid && calls==2 && waits==1);
    puts("direction protocol: PASS");
}
