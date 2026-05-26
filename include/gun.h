#ifndef GUN_H
#define GUN_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "engine_math.h"

typedef struct {
    float damage;
    float knockback;
    float knockback_y;

    float range;

    uint32_t seed;
    float aim_fov;

    float reload_time;
    float wait_time;

    bool tracers;
} Gun_stats;

// bottom 8 bits: ray count
// next 8 bits: spread/accuracy bucket
// top 16 bits: can be used as per-shot random seed


static inline Gun_stats gun_stats_blunder() {
    Gun_stats gun;
    gun.damage = 10.0f;
    gun.knockback = 2.0f;
    gun.knockback_y = 1.3f;
    gun.range = 120.0f,
    gun.seed = 9U | (30U << 8);
    gun.reload_time = 1.0f;
    gun.wait_time = 0.0f;
    gun.aim_fov = 70.0f;
    gun.tracers = true;
    return gun;
}

static inline Gun_stats gun_stats_sniper() {
    Gun_stats gun; 
    gun.damage = 70.0f;
    gun.knockback = 0.0f;
    gun.knockback_y = 0.0f;
    gun.range = 400.0f,
    gun.seed = 1U | (0U << 8);
    gun.reload_time = 1.8f;
    gun.wait_time = 0.0f;
    gun.aim_fov = 45.0f;
    gun.tracers = true;
    return gun;
}

static inline Gun_stats gun_stats_teapot() {
    Gun_stats gun;
    gun.damage = -20.0f;
    gun.knockback = 0.0f;
    gun.knockback_y = 0.0f;
    gun.range = 300.0f,
    gun.seed = 1U | (0U << 8);
    gun.reload_time = 0.5f;
    gun.wait_time = 0.0f;
    gun.aim_fov = 103.0f;
    gun.tracers = true;
    return gun;
}

static inline Gun_stats gun_stats_mace() {
    Gun_stats gun;
    gun.damage = 26.0f;
    gun.knockback = 6.0f;
    gun.knockback_y = 2.0f;
    gun.range = 4.2f,
    gun.seed = 1U | (0U << 8);
    gun.reload_time = 0.45f;
    gun.wait_time = 0.0f;
    gun.aim_fov = 103.0f;
    gun.tracers = false;
    return gun;
}

static inline uint32_t gun_seed_gen(Gun_stats gun) {
    uint32_t config = gun.seed & 0x0000FFFFU;
    uint32_t random_hi = ((uint32_t)rand() & 0x0000FFFFU) << 16;
    return random_hi | config;
}

static inline void gun_seed_read(uint32_t seed, uint8_t* shots, uint8_t* spread) {

    if (!shots || !spread ) {return;}

    uint8_t shot_count = seed & 0xFF;
    uint8_t spread_mul = (seed >> 8) & 0xFF;

    *shots = shot_count;
    *spread = spread_mul;

}

static inline uint32_t gun_hash_u32(uint32_t x) {
    x ^= x >> 16;
    x *= 0x7feb352dU;
    x ^= x >> 15;
    x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

static inline float gun_rand_signed(uint32_t* state) {
    *state = gun_hash_u32(*state);
    return ((float)(*state & 0x00FFFFFFU) / 16777215.0f) * 2.0f - 1.0f;
}

static inline Vec3 gun_spread(uint32_t seed, uint8_t shot_idx, Vec3 original_dir) {
    uint8_t shots = 0;
    uint8_t spread = 0;
    gun_seed_read(seed, &shots, &spread);

    Vec3 base = vec3_normalize(&original_dir);
    if (vec3_mag_squared(&base) <= 0.0f) {
        base = (Vec3){0.0f, 0.0f, -1.0f};
    }

    Vec3 ref = fabsf(base.y) > 0.99f ? (Vec3){1.0f, 0.0f, 0.0f} : (Vec3){0.0f, 1.0f, 0.0f};
    Vec3 tangent = vec3_cross(&ref, &base);
    tangent = vec3_normalize(&tangent);
    Vec3 bitangent = vec3_cross(&base, &tangent);
    bitangent = vec3_normalize(&bitangent);

    uint32_t state = seed ^ ((uint32_t)shot_idx * 0x9E3779B9U + 0xA341316CU);
    float spread_strength = (float)spread * 0.0025f;
    float rx = gun_rand_signed(&state) * spread_strength;
    float ry = gun_rand_signed(&state) * spread_strength;

    Vec3 spread_right = vec3_multiply(&tangent, rx);
    Vec3 spread_up = vec3_multiply(&bitangent, ry);
    Vec3 spread_dir = vec3_add(&base, &spread_right);
    spread_dir = vec3_add(&spread_dir, &spread_up);
    return vec3_normalize(&spread_dir);
}


#endif //GUN_H
