#include "physics.h"
#include "client.h"
#include "server.h"
#include "level.h"
#include "stb_ds.h"
#include "states.h"

#define EPSILON 0.0001f
#define SHOOT_MAX_RAY_LEN 100.0f
#define SHOOT_RAY_STEP 0.1f
#define SHOOT_TRACER_TIME 0.08f

void physics_p_air_movement(Player* p, float dt);
void physics_p_ground_movement(Player* p, float dt);
static void physics_apply_friction(Entity* ent, float friction, float dt);
static void physics_accelerate(Entity* ent, const Vec3* wish_dir, float wish_speed, float accel, float dt);
static void physics_redirect(Entity* ent, const Vec3* wish_dir, float redirect_rate, float dt);
static void physics_gliding(Entity* ent, const Vec3* cam_dir, float redirect_rate, float dt);
static void physics_air_accelerate(Entity* ent, Vec3 wishvel, float wishspeed, float accelerate,  float dt);
bool check_dead(Entity* ent);

void physics_funny_bounds_check(Entity* ent);

Player* ray_check_player_collison(Level* level, Player* shooter, float max_ray_len, float step);


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

        check_dead(ent);

        if (p->shoot_queued) {
            p->shoot_queued = false;

            if (level->client_ref != NULL) {
                Client* client = level->client_ref;
                
                Packet_shoot payload = {
                    PCKT_SHOOT,
                    p->server_id
                };

                ENetPacket* packet = enet_packet_create(
                    &payload,
                    sizeof(payload),
                    0
                );

                enet_peer_send(client->server_peer, 1, packet);

            }

            if (level->server_ref != NULL) {
                Server* server = level->server_ref;
                Vec3 dir = p->movement.cam_dir;
                Vec3 source = p->entity.position;
                vec3_add_inplace(&source, &p->eye_offset);

                if (vec3_mag_squared(&dir) > EPSILON) {
                    vec3_normalize_inplace(&dir);
                } else {
                    dir = (Vec3){0.0f, 0.0f, -1.0f};
                }

                Vec3 ray = vec3_multiply(&dir, SHOOT_MAX_RAY_LEN);
                Vec3 dest = vec3_add(&source, &ray);

                Packet_tracer tracer_payload = {
                    PCKT_TRACER,
                    p->server_id,
                    source,
                    dest,
                    SHOOT_TRACER_TIME
                };

                ENetPacket* tracer_packet = enet_packet_create(
                    &tracer_payload,
                    sizeof(tracer_payload),
                    0
                );
                enet_host_broadcast(server->e_server, 1, tracer_packet);
            }

            Player* hit = ray_check_player_collison(level, p, SHOOT_MAX_RAY_LEN, SHOOT_RAY_STEP);

            if (hit && (level->server_ref != NULL)) {

                hit->entity.health -= 10.0f;

                Server* server = level->server_ref;

                Vec3 dir = vec3_subtract(&hit->entity.position, &ent->position);
                vec3_normalize_inplace(&dir);
                Vec3 knock = vec3_multiply(&dir, 6.0f);
                knock.y += 4.5f;

                Packet_server_auth_knockback payload = {
                    PCKT_SERVER_AUTH_KNOCK,
                    hit->server_id,
                    knock
                };
                ENetPacket* packet = enet_packet_create(
                    &payload,
                    sizeof(payload),
                    0
                );
                enet_host_broadcast(server->e_server, 1, packet);
            }


        }

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

            if (0 /*p->movement.jump_queued && ent->velocity.y < 6.0f*/) {
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

        physics_funny_bounds_check(ent);

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

    Vec3 wishdir = (Vec3){m->wish_dir.x, 0.0f, m->wish_dir.z};

    if (vec3_mag_squared(&wishdir) > EPSILON) {vec3_normalize_inplace(&wishdir);}

    float wishspeed = m->run_speed;
    if (wishspeed > m->air_speed_cap) {wishspeed = m->air_speed_cap;}
    Vec3 wishvel = vec3_multiply(&wishdir, wishspeed);

    if (is_state(ent, GLIDING)) {
        physics_gliding(ent, &m->cam_dir, m->glide_redirection, dt);
    }
    else {
        if (vec3_mag_squared(&wishvel) <= EPSILON) {return;}
        physics_air_accelerate(ent, wishvel, wishspeed, m->air_acceleration, dt);
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

        Vec3 wishdir = (Vec3){m->wish_dir.x, 0.0f, m->wish_dir.z};
        if (vec3_mag_squared(&wishdir) <= EPSILON) {return;}
        vec3_normalize_inplace(&wishdir);
        Vec3 wishvel = vec3_multiply(&wishdir, spd);

        physics_air_accelerate(ent, wishvel, spd, accel, dt);

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



static void physics_air_accelerate(Entity* ent, Vec3 wishvel, float wishspeed, float accelerate,  float dt) {
    float wishspd = vec3_mag(&wishvel);
    vec3_normalize_inplace(&wishvel);
    float currentspeed = vec3_dot(&ent->velocity, &wishvel);
    float addspeed = wishspd - currentspeed;
    if(addspeed <=0) {return;}
    float accelspeed = accelerate  * wishspeed * dt;

    if (accelspeed > addspeed) {accelspeed = addspeed;}

    Vec3 accel_vel = vec3_multiply(&wishvel, accelspeed);
    vec3_add_inplace(&ent->velocity, &accel_vel);
    
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

    physics_accelerate(ent, cam_dir, 20.0f, 0.1f, dt);
}




Player* ray_check_player_collison(Level* level, Player* shooter, float max_ray_len, float step) {
    if (!level || step <= EPSILON || max_ray_len <= 0.0f) {
        return NULL;
    }

    Vec3 dir = shooter->movement.cam_dir;
    if (vec3_mag_squared(&dir) <= EPSILON) {
        return NULL;
    }
    vec3_normalize_inplace(&dir);
    Vec3 source = shooter->entity.position;
    vec3_add_inplace(&source, &shooter->eye_offset);

    float ray_len = 0.0f;
    Vec3 ray = {0.0f, 0.0f, 0.0f};
    Vec3 ray_step = vec3_multiply(&dir, step);

    while (ray_len <= max_ray_len) {
        ray_len += step;
        vec3_add_inplace(&ray, &ray_step);

        Vec3 test_coords = vec3_add(&ray, &source);
        
        for (int i = 0; i < hmlen(level->player_map); i++) {
            Player* p = &level->player_map[i].value;

            if (shooter && (p == shooter || p->server_id == shooter->server_id)) {
                continue;
            }

            float hit_radius = p->entity.radius;
            Vec3 to_player = vec3_subtract(&test_coords, &p->entity.position);
            float dist_sq = vec3_mag_squared(&to_player);

            if (dist_sq <= (hit_radius * hit_radius)) {
                return p;
            }

        }

    }

    return NULL;
}



void physics_funny_bounds_check(Entity* ent) {
    if (vec3_mag(&ent->position) > 300.0f) {
        Vec3 to_center = vec3_subtract(&(Vec3){0.0f, 0.0f, 0.0f}, &ent->position);
        if (vec3_mag_squared(&to_center) <= EPSILON) {return;}
        vec3_normalize_inplace(&to_center);

        float speed = vec3_mag(&ent->velocity);
        Vec3 backvel = vec3_multiply(&to_center, speed);

        ent->velocity = backvel;
    }
}


bool check_dead(Entity* ent) {
    if (ent->health <= 0.0f) {
        set_state(ent, DEAD);
        return 1;
    }
    return 0;
}