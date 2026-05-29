#ifndef PLAYER_H
#define PLAYER_H

#include "entity.h"
#include <stdint.h>
#include "gun.h"
#include "event.h"


typedef struct {
    Vec3 wish_dir;
    Vec3 cam_forward;
    Vec3 cam_right;
    float ground_acceleration_run;
    float ground_acceleration_walk;
    float ground_friction;
    float air_acceleration;
    float air_speed_cap;

    float air_fric_threshold;

    float run_speed;
    float walk_speed;
    float air_speed;

    bool jump_queued;
    float jump_vel;

    bool slide_queued;
    float slide_friction;

    bool dash_queued;

} Player_movement;

typedef struct {
    uint32_t current_charges;
    uint32_t max_charges;

    uint32_t dash_vel;

    float cast_wait;
    float cast_wait_time;

    float recharge_wait;
    float recharge_wait_time;
} Player_dash;


typedef struct {
    uint32_t gun_count;
    Gun_stats* guns;
} Player_guns;

void player_guns_destroy(Player_guns* pg);

typedef struct Player {
    Entity entity;
    Vec3 eye_offset; //how high the camera is placed over the centroid

    uint64_t unqid;
    uint32_t server_id;

    const char* player_name;

    Player_movement movement;
    Player_dash dash;

    uint32_t gun_idx;
    Player_guns guns;

    bool shoot_queued;

    Event_bus event_bus;

} Player;

Player player_create();

#endif // PLAYER_H