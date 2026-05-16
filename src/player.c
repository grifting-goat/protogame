#include "player.h"

#include "states.h"

Player player_create() {
    Player p;
    entity_create_default(&p.entity);
    p.player_name = "Grift";
    p.eye_offset = (Vec3){0.0f,1.0f,0.0f}; //how high the camera is placed over the centroid
    p.entity.mass = 70.0f;   
    p.entity.radius = 1.0f;
    p.movement.wish_dir = (Vec3) {0.0f, 0.0f, 0.0f};
    p.movement.cam_forward = (Vec3) {0.0f, 0.0f, -1.0f};
    p.movement.cam_right = (Vec3) {1.0f, 0.0f, 0.0f};
    p.movement.ground_acceleration_run = 9.0f;
    p.movement.ground_acceleration_walk = 6.0f;
    p.movement.air_acceleration = 12.0f;
    p.movement.ground_friction = 8.0f;
    p.movement.air_speed_cap = 17.0f;
    p.movement.run_speed = 10.0f;
    p.movement.walk_speed = 7.0f;
    p.movement.air_speed = 1.5f;
    p.movement.jump_queued = false;
    p.movement.jump_vel = 5.0f;
    p.movement.slide_queued = false;
    p.movement.slide_friction = 0.5f;

    clear_state(&p.entity, DEAD);
    

    p.shoot_queued = false;
    return p;
}