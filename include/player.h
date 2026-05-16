#ifndef PLAYER_H
#define PLAYER_H

#include "entity.h"
#include <stdint.h>

typedef struct {
    Vec3 wish_dir;
    Vec3 cam_forward;
    Vec3 cam_right;
    float ground_acceleration_run;
    float ground_acceleration_walk;
    float ground_friction;
    float air_acceleration;
    float air_speed_cap;

    float run_speed;
    float walk_speed;
    float air_speed;

    bool jump_queued;
    float jump_vel;

    bool slide_queued;
    float slide_friction;

} Player_movement;



typedef struct {
    Entity entity;
    Vec3 eye_offset; //how high the camera is placed over the centroid

    uint64_t unqid;
    uint32_t server_id;

    const char* player_name;

    Player_movement movement;

    bool shoot_queued; //temp

} Player;

Player player_create();

#endif // PLAYER_H