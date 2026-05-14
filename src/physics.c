#include "physics.h"
#include "level.h"
#include "stb_ds.h"
#include "states.h"

#define EPSILON 0.0001f

void physics_p_air_movement(Player* p, float dt);
void physics_p_ground_movement(Player* p, float dt);
static void physics_apply_friction(Entity* ent, float friction, float dt);
static void physics_accelerate(Entity* ent, const Vec3* wish_dir, float wish_speed, float accel, float dt);
static void physics_redirect(Entity* ent, const Vec3* wish_dir, float redirect_rate, float dt);
static void physics_gliding(Entity* ent, const Vec3* cam_dir, float redirect_rate, float dt);


void physics_init(Level* level) {
    PhysicsWorld* world = &level->physics;

    world->tick_rate_physics = level->tick_rate;
    world->tick_freq_physics = 1.0f / (float)level->tick_rate;
    world->accumulator_physics = 0.0f;

    world->max_accumulator_physics = world->tick_freq_physics * MAX_TICK_DELAY;
    
    world->gravity = (Vec3){0.0f, -9.81f, 0.0f};

}

void physics_world_update(Level* level, float dt) {
    PhysicsWorld* world = &level->physics;
    world->accumulator_physics += dt;

    if (world->accumulator_physics > world->max_accumulator_physics) {world->accumulator_physics = world->max_accumulator_physics;}

    while (world->accumulator_physics >= world->tick_freq_physics) {
        physics_step(level, world->tick_freq_physics);
        world->accumulator_physics -= world->tick_freq_physics;
    }

}

void physics_step(Level* level, float dt) {
    PhysicsWorld* world = &level->physics;

        for (int i = 0; i < hmlen(level->player_map); i++) {
            Player* p = &level->player_map[i].value;
            Entity* ent = &level->player_map[i].value.entity;

            physics_update_states(level, ent);

            if (is_state(ent, GROUNDED)) {
                if(ent->position.y < ent->radius) {
                    ent->position.y = ent->radius;
                    ent->velocity.y = 0.0f;
                }

                if (p->movement.jump_queued) {
                    p->movement.jump_queued = false;
                    ent->velocity.y += p->movement.jump_vel;
                }

                float horizonal_spd_mag = (ent->velocity.x * ent->velocity.x) + (ent->velocity.z * ent->velocity.z);
                if (p->movement.slide_queued && (horizonal_spd_mag > (p->movement.walk_speed * p->movement.walk_speed))) {
                    set_state(ent, SLIDING);
                } else {
                    clear_state(ent, SLIDING);
                }

                physics_p_ground_movement(p, dt);

            } else if (is_state(ent, IN_AIR)) {

                if (p->movement.jump_queued && ent->velocity.y < 0.0f) {
                    p->movement.jump_queued = false;
                    set_state(ent, GLIDING);

                    Vec3 gravity_force = vec3_multiply(&world->gravity, (float)ent->mass / 5.0f);
                    vec3_add_inplace(&ent->force, &gravity_force);
                } else {
                    clear_state(ent, GLIDING);
                    
                    Vec3 gravity_force = vec3_multiply(&world->gravity, (float)ent->mass);
                    vec3_add_inplace(&ent->force, &gravity_force);
                }

                physics_p_air_movement(p, dt);
            }

            //apply forces
            Vec3 delta_vel = vec3_multiply(&ent->force, dt / (float)ent->mass);
            vec3_add_inplace(&ent->velocity, &delta_vel);

            Vec3 delta_pos = vec3_multiply(&ent->velocity, dt);
            vec3_add_inplace(&ent->position, &delta_pos);

            if (ent->position.y < ent->radius) {
                ent->position.y = ent->radius;
                if (ent->velocity.y < 0.0f) {
                    ent->velocity.y = 0.0f;
                }
            }

            if (fabsf(ent->velocity.x) < EPSILON) ent->velocity.x = 0.0f;
            if (fabsf(ent->velocity.y) < EPSILON) ent->velocity.y = 0.0f;
            if (fabsf(ent->velocity.z) < EPSILON) ent->velocity.z = 0.0f;

            physics_update_states(level, ent);

            ent->force = (Vec3){0.0f, 0.0f, 0.0f};
        }

}

void physics_update_states(Level* level, Entity* ent) {
    bool on_ground = (ent->position.y <= ent->radius + EPSILON);
    if (on_ground) {
        set_state(ent, GROUNDED);
        clear_state(ent, IN_AIR);

    } else {
        clear_state(ent, GROUNDED);
        set_state(ent, IN_AIR);
    }

}

void physics_p_air_movement(Player* p, float dt) {

    Entity* ent = &p->entity;
    const Player_movement* m = &p->movement;

    float wish_speed = m->run_speed;
    if (wish_speed > m->air_speed_cap) {wish_speed = m->air_speed_cap;}

    if (is_state(ent, GLIDING)) {
        physics_gliding(ent, &m->cam_dir, m->glide_redirection, dt);
    }
    else {
        if (vec3_mag_squared(&m->wish_dir) <= EPSILON) {return;}
        physics_accelerate(ent, &m->wish_dir, wish_speed, m->air_acceleration, dt);
    }
    
}

void physics_p_ground_movement(Player* p, float dt) {
    Entity* ent = &p->entity;
    const Player_movement* m = &p->movement;

    float fric = is_state(&p->entity, SLIDING) ? m->slide_friction : m->ground_friction;
    physics_apply_friction(ent, fric, dt);

    if (is_state(ent, SLIDING)) {
        physics_redirect(ent, &m->cam_dir, m->slide_redirection, dt);
    } else {

        if (vec3_mag_squared(&m->wish_dir) <= EPSILON) {return;}
        float spd = is_state(ent, RUNNING) ? m->run_speed : m->walk_speed;
        float accel = is_state(ent, RUNNING) ? m->ground_acceleration_run : m->ground_acceleration_walk;
        physics_accelerate(ent, &m->wish_dir, spd, accel, dt);

    }
    

}


static void physics_apply_friction(Entity* ent, float friction, float dt) {
    float speed = sqrtf(ent->velocity.x * ent->velocity.x + ent->velocity.z * ent->velocity.z);

    if (speed <= EPSILON) {
        return;
    }

    float drop = speed * friction * dt;
    float new_speed = speed - drop;

    if (new_speed < 0.0f) {
        new_speed = 0.0f;
    }

    if (new_speed == speed) {
        return;
    }

    float scale = new_speed / speed;
    ent->velocity.x *= scale;
    ent->velocity.z *= scale;
}

static void physics_accelerate(Entity* ent, const Vec3* wish_dir, float wish_speed, float accel, float dt) {
    float current_speed = ent->velocity.x * wish_dir->x + ent->velocity.z * wish_dir->z;
    float add_speed = wish_speed - current_speed;

    if (add_speed <= 0.0f) {return;}

    float accel_speed = accel * dt * wish_speed;
    if (accel_speed > add_speed) {
        accel_speed = add_speed;
    }

    ent->velocity.x += wish_dir->x * accel_speed;
    ent->velocity.z += wish_dir->z * accel_speed;
}

static void physics_redirect(Entity* ent, const Vec3* cam_dir, float redirect_rate, float dt) {

    float speed = sqrtf(ent->velocity.x * ent->velocity.x + ent->velocity.z * ent->velocity.z);

    if (speed <= EPSILON) {return;}

    float dir_len_sq = (cam_dir->x * cam_dir->x) + (cam_dir->z * cam_dir->z);

    if (dir_len_sq <= EPSILON) {
        return;
    }

    Vec3 cam_flat = {cam_dir->x, 0.0f, cam_dir->z};
    cam_flat = vec3_normalize(&cam_flat);

    Vec3 current = {ent->velocity.x, 0.0f, ent->velocity.z};
    Vec3 desired = {cam_flat.x * speed, 0.0f, cam_flat.z * speed};

    float turn_t = redirect_rate * dt;
    if (turn_t < 0.0f) turn_t = 0.0f;
    if (turn_t > 1.0f) turn_t = 1.0f;

    Vec3 blended = {
        current.x + (desired.x - current.x) * turn_t,
        0.0f,
        current.z + (desired.z - current.z) * turn_t
    };

    float blended_speed = sqrtf(blended.x * blended.x + blended.z * blended.z);
    if (blended_speed <= EPSILON) {
        return;
    }

    float preserve_scale = speed / blended_speed;
    ent->velocity.x = blended.x * preserve_scale;
    ent->velocity.z = blended.z * preserve_scale;
}



static void physics_gliding(Entity* ent, const Vec3* cam_dir, float redirect_rate, float dt) {
    float speed = vec3_mag(&ent->velocity);
    if (speed <= EPSILON) {return;}

    float dir_len_sq = vec3_mag_squared(cam_dir);
    if (dir_len_sq <= EPSILON) {return;}

    Vec3 current = ent->velocity;
    Vec3 desired = vec3_multiply(cam_dir, speed);

    float turn_t = redirect_rate * dt;
    if (turn_t < 0.0f) turn_t = 0.0f;
    if (turn_t > 1.0f) turn_t = 1.0f;

    Vec3 blended = {
        current.x + (desired.x - current.x) * turn_t,
        current.y + (desired.y - current.y) * turn_t,
        current.z + (desired.z - current.z) * turn_t
    };

    float redirected_speed = vec3_mag(&blended);
    if (redirected_speed <= EPSILON) {return;}

    float preserve_scale = speed / redirected_speed;

    ent->velocity.x = blended.x * preserve_scale;
    ent->velocity.y = blended.y * preserve_scale;
    ent->velocity.z = blended.z * preserve_scale;

    physics_accelerate(ent, cam_dir, 30.0f, 0.2f, dt);
}