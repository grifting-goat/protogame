#include "player.h"

Player player_create() {
    Player p;
    entity_create_default(&p.entity);

    p.health = 100.0f;

    //sudo_physics
    p.sprint_speed = 8.0f;
    p.walk_speed = 5.0f;
    p.air_accel = 2.2f;
    p.ground_accel = 10.0f;
    p.ground_friction = 8.0f;

    p.jump_force = 1000.0f;
    p.force_control = (Vec3){0.0f,0.0f,0.0f};
    p.eye_offset = (Vec3){0.0f,1.0f,0.0f}; //how high the camera is placed over the centroid

    //less sudo physics
    p.entity.mass = 70.0f;    

    //states
    p.collision = true;
    p.fly = false;
    p.sprinting = false;
    p.crouched = false;

    return p;
}