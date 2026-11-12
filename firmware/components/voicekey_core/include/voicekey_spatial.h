#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define VK_REGIONS_MAX 8u
#define VK_REGION_NONE (-1)
#define VK_OBSERVATION_TTL_MS 100u
#define VK_DIRECTION_HISTORY 64u

typedef struct { bool valid, speech; uint16_t degrees; uint64_t at_ms; } vk_observation_t;
typedef struct { uint16_t center, half_width; } vk_region_t;
typedef enum { VK_FLOOR_MANUAL, VK_FLOOR_FIRST, VK_FLOOR_KEYWORD } vk_floor_policy_t;
typedef struct {
    bool enabled, observe_only;
    int16_t offset;
    uint8_t count;
    vk_region_t regions[VK_REGIONS_MAX];
    vk_floor_policy_t policy;
    uint16_t confirm_ms, tolerance;
    uint32_t release_ms;
} vk_spatial_config_t;
typedef enum {
    VK_GATE_BYPASS, VK_GATE_UNAVAILABLE, VK_GATE_SILENCE,
    VK_GATE_UNKNOWN_REGION, VK_GATE_NO_OWNER, VK_GATE_OTHER_REGION,
    VK_GATE_OUTSIDE_LOCK, VK_GATE_ALLOW
} vk_gate_reason_t;
typedef struct {
    vk_spatial_config_t config;
    vk_observation_t observation;
    vk_observation_t history[VK_DIRECTION_HISTORY];
    unsigned history_head, history_count;
    int owner, candidate;
    uint16_t locked_angle, candidate_angle;
    uint64_t candidate_since, last_match_ms, last_now_ms;
    uint32_t generation;
    bool have_observation, have_time;
} vk_spatial_t;

/* All APIs are allocation-free. Caller serializes accesses and supplies monotonic time. */
vk_spatial_config_t vk_spatial_defaults(void);
void vk_spatial_init(vk_spatial_t *s);
bool vk_spatial_configure(vk_spatial_t *s, const vk_spatial_config_t *config);
uint16_t vk_angle_calibrate(uint16_t angle, int16_t offset);
unsigned vk_angle_distance(uint16_t a, uint16_t b);
bool vk_observation_fresh(const vk_observation_t *o, uint64_t now);
int vk_region_map(const vk_spatial_config_t *config, uint16_t calibrated_angle);
void vk_spatial_observe(vk_spatial_t *s, vk_observation_t observation, uint64_t now);
void vk_spatial_tick(vk_spatial_t *s, uint64_t now);
void vk_floor_revoke(vk_spatial_t *s);
bool vk_floor_grant(vk_spatial_t *s, int region, uint64_t now);
bool vk_floor_request(vk_spatial_t *s, int region, bool preempt, uint64_t now);
bool vk_floor_release(vk_spatial_t *s, int region);
int vk_spatial_region(const vk_spatial_t *s, uint64_t now);
vk_gate_reason_t vk_spatial_gate(const vk_spatial_t *s, uint64_t now);
/* Conservative attribution: full covered interval, fresh contiguous samples,
   speech throughout, one unambiguous region. Unknown is a valid result. */
int vk_event_attribute(const vk_spatial_t *s, uint64_t start, uint64_t end, uint64_t now);
typedef enum { VK_SOURCE_AUTO_PROCESSED, VK_SOURCE_FIXED_DIRECTION } vk_source_t;
bool vk_source_supported(vk_source_t source);
