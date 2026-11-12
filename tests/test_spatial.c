#include "voicekey_spatial.h"
#include <assert.h>
#include <stdio.h>
static void sample(vk_spatial_t *s, unsigned a, bool speech, uint64_t t) {
    vk_spatial_observe(s,(vk_observation_t){true,speech,a,t},t);
}
int main(void) {
    vk_spatial_t s; vk_spatial_init(&s);
    vk_spatial_config_t c=vk_spatial_defaults(); c.enabled=true;
    c.count=2; c.regions[1]=(vk_region_t){90,25};
    assert(vk_spatial_configure(&s,&c));
    assert(vk_region_map(&c,330)==0 && vk_region_map(&c,30)==0 && vk_region_map(&c,31)==-1);
    assert(vk_angle_calibrate(350,20)==10 && vk_angle_calibrate(10,-20)==350);
    c.regions[1]=(vk_region_t){20,25}; assert(vk_region_map(&c,0)==-1);
    c.regions[1]=(vk_region_t){90,25};
    uint32_t gen=s.generation; c.count=9;
    assert(!vk_spatial_configure(&s,&c) && s.generation==gen && s.config.count==2); c.count=2;
    sample(&s,0,true,0); sample(&s,1,true,100); sample(&s,359,true,200);
    assert(s.owner==-1); sample(&s,0,true,250); assert(s.owner==0);
    assert(vk_spatial_gate(&s,250)==VK_GATE_ALLOW);
    gen=s.generation; sample(&s,90,true,300);
    assert(s.owner==0 && s.generation==gen && vk_spatial_gate(&s,300)==VK_GATE_OTHER_REGION);
    sample(&s,25,true,350); assert(vk_spatial_gate(&s,350)==VK_GATE_OUTSIDE_LOCK);
    for(unsigned t=400;t<2250;t+=50) sample(&s,90,true,t);
    sample(&s,90,true,2250); assert(s.owner==-1); /* other speaker did not refresh lease */
    for(unsigned t=2300;t<=2500;t+=50) sample(&s,90,true,t);
    assert(s.owner==1);
    vk_spatial_tick(&s,2601); assert(s.owner==-1 && vk_spatial_gate(&s,2601)==VK_GATE_UNAVAILABLE);
    sample(&s,0,true,2700);
    vk_spatial_observe(&s,(vk_observation_t){true,true,0,2700},2750);
    assert(s.owner==-1 && s.candidate_since==2700);
    sample(&s,0,true,3000); assert(s.owner==-1 && s.candidate_since==3000);
    sample(&s,360,true,3010); assert(!s.observation.valid && s.candidate==-1);
    vk_spatial_observe(&s,(vk_observation_t){true,true,0,4000},3020);
    assert(!s.observation.valid);
    vk_spatial_init(&s); c.policy=VK_FLOOR_MANUAL; assert(vk_spatial_configure(&s,&c));
    sample(&s,0,true,0); assert(vk_floor_grant(&s,0,0)); gen=s.generation;
    for(unsigned t=50;t<4000;t+=50) sample(&s,0,false,t);
    assert(s.owner==0 && s.generation==gen && vk_spatial_gate(&s,3950)==VK_GATE_SILENCE);
    assert(!vk_floor_request(&s,1,false,3950));
    assert(vk_floor_request(&s,1,true,3950) && s.owner==1 && s.generation!=gen);
    assert(!vk_floor_release(&s,0) && vk_floor_release(&s,1));
    assert(!vk_floor_grant(&s,9,3950));
    sample(&s,90,true,4000); assert(vk_floor_grant(&s,1,4000));
    vk_spatial_tick(&s,3999); assert(s.owner==-1);
    vk_spatial_init(&s); assert(vk_spatial_configure(&s,&c));
    for(unsigned t=100;t<=500;t+=50) sample(&s,90,true,t);
    assert(vk_event_attribute(&s,125,490,500)==1);
    assert(vk_event_attribute(&s,0,490,500)==-1);
    assert(vk_event_attribute(&s,125,490,700)==-1);
    sample(&s,0,true,550); assert(vk_event_attribute(&s,450,550,550)==-1);
    sample(&s,0,false,600); assert(vk_event_attribute(&s,550,600,600)==-1);
    for(unsigned t=650;t<=5000;t+=50) sample(&s,0,true,t);
    assert(vk_event_attribute(&s,125,5000,5000)==-1);
    assert(vk_event_attribute(&s,4800,5000,5000)==0);
    c.enabled=false; assert(vk_spatial_configure(&s,&c));
    assert(vk_spatial_gate(&s,9000)==VK_GATE_BYPASS && !vk_floor_grant(&s,0,9000));
    assert(vk_source_supported(VK_SOURCE_AUTO_PROCESSED) && !vk_source_supported(VK_SOURCE_FIXED_DIRECTION));
    puts("spatial primitives: PASS");
}
