#include "voicekey_spatial.h"
#include <string.h>

vk_spatial_config_t vk_spatial_defaults(void) {
    return (vk_spatial_config_t){.count=1,.regions={{0,30}},.policy=VK_FLOOR_FIRST,
        .confirm_ms=250,.tolerance=20,.release_ms=2000};
}
void vk_spatial_init(vk_spatial_t *s) {
    memset(s,0,sizeof(*s)); s->config=vk_spatial_defaults();
    s->owner=s->candidate=VK_REGION_NONE;
}
bool vk_spatial_configure(vk_spatial_t *s, const vk_spatial_config_t *c) {
    if (!c || !c->count || c->count>VK_REGIONS_MAX || c->offset<-359 || c->offset>359 ||
        c->policy<VK_FLOOR_MANUAL || c->policy>VK_FLOOR_KEYWORD ||
        !c->tolerance || c->tolerance>180 || !c->confirm_ms || c->confirm_ms>2000 ||
        c->release_ms<100 || c->release_ms>60000) return false;
    for (unsigned i=0;i<c->count;i++)
        if (c->regions[i].center>=360 || !c->regions[i].half_width || c->regions[i].half_width>180) return false;
    uint32_t next=s->generation+1;
    vk_spatial_config_t copy=*c;
    vk_spatial_init(s); s->config=copy; s->generation=next; return true;
}
uint16_t vk_angle_calibrate(uint16_t angle, int16_t offset) {
    int value=((int)angle+offset)%360; return value<0 ? value+360 : value;
}
unsigned vk_angle_distance(uint16_t a, uint16_t b) {
    a%=360; b%=360;
    unsigned d=a>b ? a-b : b-a; return d>180 ? 360-d : d;
}
bool vk_observation_fresh(const vk_observation_t *o, uint64_t now) {
    return o->valid && o->degrees<360 && now>=o->at_ms && now-o->at_ms<=VK_OBSERVATION_TTL_MS;
}
int vk_region_map(const vk_spatial_config_t *c, uint16_t angle) {
    int found=VK_REGION_NONE;
    if (angle>=360 || c->count>VK_REGIONS_MAX) return found;
    for (unsigned i=0;i<c->count;i++) if (vk_angle_distance(angle,c->regions[i].center)<=c->regions[i].half_width) {
        if (found!=VK_REGION_NONE) return VK_REGION_NONE;
        found=(int)i;
    }
    return found;
}
int vk_spatial_region(const vk_spatial_t *s, uint64_t now) {
    if (!s->have_observation || !vk_observation_fresh(&s->observation,now) || !s->observation.speech) return VK_REGION_NONE;
    return vk_region_map(&s->config,vk_angle_calibrate(s->observation.degrees,s->config.offset));
}
void vk_floor_revoke(vk_spatial_t *s) {
    if (s->owner!=VK_REGION_NONE) s->generation++;
    s->owner=s->candidate=VK_REGION_NONE;
}
bool vk_floor_grant(vk_spatial_t *s, int region, uint64_t now) {
    if (!s->config.enabled || s->config.observe_only || region<0 || region>=s->config.count ||
        (s->have_time && now<s->last_now_ms)) return false;
    s->generation++; s->owner=region; s->candidate=VK_REGION_NONE;
    s->locked_angle=s->config.regions[region].center;
    if (vk_spatial_region(s,now)==region)
        s->locked_angle=vk_angle_calibrate(s->observation.degrees,s->config.offset);
    s->last_match_ms=now; return true;
}
bool vk_floor_request(vk_spatial_t *s, int region, bool preempt, uint64_t now) {
    if (!s->config.enabled || s->config.observe_only || region<0 || region>=s->config.count ||
        (s->have_time && now<s->last_now_ms)) return false;
    if (region==s->owner && region!=VK_REGION_NONE) return true;
    return (s->owner==VK_REGION_NONE || preempt) && vk_floor_grant(s,region,now);
}
bool vk_floor_release(vk_spatial_t *s, int region) {
    if (region<0 || region!=s->owner) return false;
    vk_floor_revoke(s); return true;
}
void vk_spatial_tick(vk_spatial_t *s, uint64_t now) {
    if (s->have_time && now<s->last_now_ms) {
        s->observation.valid=false; s->history_count=0;
    }
    s->last_now_ms=now; s->have_time=true;
    if (!s->config.enabled) return;
    if (!s->have_observation || !vk_observation_fresh(&s->observation,now)) {
        vk_floor_revoke(s); return;
    }
    if (s->config.policy==VK_FLOOR_FIRST && s->owner!=VK_REGION_NONE &&
        now>=s->last_match_ms && now-s->last_match_ms>=s->config.release_ms) vk_floor_revoke(s);
}
vk_gate_reason_t vk_spatial_gate(const vk_spatial_t *s, uint64_t now) {
    if (!s->config.enabled || s->config.observe_only) return VK_GATE_BYPASS;
    if (!s->have_observation || !vk_observation_fresh(&s->observation,now)) return VK_GATE_UNAVAILABLE;
    if (!s->observation.speech) return VK_GATE_SILENCE;
    int region=vk_spatial_region(s,now);
    if (region==VK_REGION_NONE) return VK_GATE_UNKNOWN_REGION;
    if (s->owner==VK_REGION_NONE) return VK_GATE_NO_OWNER;
    if (region!=s->owner) return VK_GATE_OTHER_REGION;
    if (s->config.policy==VK_FLOOR_FIRST &&
        vk_angle_distance(vk_angle_calibrate(s->observation.degrees,s->config.offset),s->locked_angle)>s->config.tolerance)
        return VK_GATE_OUTSIDE_LOCK;
    return VK_GATE_ALLOW;
}
void vk_spatial_observe(vk_spatial_t *s, vk_observation_t o, uint64_t now) {
    vk_spatial_tick(s,now);
    /* Duplicate valid snapshots cannot advance the stable-candidate timer. */
    if (s->have_observation && o.at_ms==s->observation.at_ms && o.valid &&
        s->observation.valid && o.degrees==s->observation.degrees && o.speech==s->observation.speech) return;
    bool gap=s->have_observation && (o.at_ms<=s->observation.at_ms ||
        o.at_ms-s->observation.at_ms>VK_OBSERVATION_TTL_MS);
    if (s->have_observation && o.at_ms<=s->observation.at_ms) o.valid=false;
    if (!vk_observation_fresh(&o,now)) o.valid=false;
    if (gap) s->candidate=VK_REGION_NONE;
    s->observation=o; s->have_observation=true;
    s->history[s->history_head]=o;
    s->history_head=(s->history_head+1)%VK_DIRECTION_HISTORY;
    if (s->history_count<VK_DIRECTION_HISTORY) s->history_count++;
    if (!o.valid) { vk_floor_revoke(s); return; }
    int region=vk_spatial_region(s,now);
    if (vk_spatial_gate(s,now)==VK_GATE_ALLOW) s->last_match_ms=o.at_ms;
    if (s->owner!=VK_REGION_NONE || s->config.policy!=VK_FLOOR_FIRST || !s->config.enabled || s->config.observe_only) return;
    if (region==VK_REGION_NONE) { s->candidate=VK_REGION_NONE; return; }
    uint16_t angle=vk_angle_calibrate(o.degrees,s->config.offset);
    if (s->candidate!=region || vk_angle_distance(angle,s->candidate_angle)>s->config.tolerance) {
        s->candidate=region; s->candidate_since=o.at_ms; s->candidate_angle=angle; return;
    }
    if (o.at_ms-s->candidate_since>=s->config.confirm_ms) {
        uint16_t anchor=s->candidate_angle;
        if (vk_floor_grant(s,region,now)) { s->locked_angle=anchor; s->last_match_ms=o.at_ms; }
    }
}
int vk_event_attribute(const vk_spatial_t *s, uint64_t start, uint64_t end, uint64_t now) {
    if (start>end || end>now || now-end>VK_OBSERVATION_TTL_MS || !s->history_count) return VK_REGION_NONE;
    int result=VK_REGION_NONE; bool covered=false; uint64_t previous=0;
    for (unsigned i=0;i<s->history_count;i++) {
        const vk_observation_t *o=&s->history[(s->history_head+VK_DIRECTION_HISTORY-s->history_count+i)%VK_DIRECTION_HISTORY];
        if (o->at_ms>end) break;
        /* Select the last observation at/before the start, then all inside. */
        if (o->at_ms<start && i+1<s->history_count) {
            const vk_observation_t *next=&s->history[(s->history_head+VK_DIRECTION_HISTORY-s->history_count+i+1)%VK_DIRECTION_HISTORY];
            if (next->at_ms<=start) continue;
        }
        if (!covered && (o->at_ms>start || start-o->at_ms>VK_OBSERVATION_TTL_MS)) return VK_REGION_NONE;
        if (!o->valid || !o->speech || o->degrees>=360 ||
            (covered && (o->at_ms<=previous || o->at_ms-previous>VK_OBSERVATION_TTL_MS))) return VK_REGION_NONE;
        int region=vk_region_map(&s->config,vk_angle_calibrate(o->degrees,s->config.offset));
        if (region==VK_REGION_NONE || (covered && region!=result)) return VK_REGION_NONE;
        result=region; previous=o->at_ms; covered=true;
    }
    return covered && end-previous<=VK_OBSERVATION_TTL_MS ? result : VK_REGION_NONE;
}
bool vk_source_supported(vk_source_t source) { return source==VK_SOURCE_AUTO_PROCESSED; }
