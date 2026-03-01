#ifndef PLAYER_H
#define PLAYER_H

#include "entity.h"

typedef struct {
    Entity entity;

    float health;

    float sprint_speed;
    float walk_speed;
    float jump_force;
    float air_accel;
    float ground_accel;
    float ground_friction;
    
    Vec3 force_control;
    Vec3 eye_offset; //how high the camera is placed over the centroid


    bool fly; //if currently flying
    bool collision; //does player collide
    bool crouched;
    bool sprinting;

} Player;

Player player_create();

#endif // PLAYER_H