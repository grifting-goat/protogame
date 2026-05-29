#include "physics.h"
#include "client.h"
#include "server.h"
#include "level.h"
#include "stb_ds.h"
#include "states.h"

#define EPSILON 0.0001f
#define MAX_WALK_ANGLE 0.5f

#define ENTITY_GROUND_FRICTION 8.0f
#define ENTITY_SLIDE_FRICTION 0.5f
#define ENTITY_AIR_FRICTION_CAP 0.1f
#define ENTITY_AIR_FRICTION_THRESHOLD 22.0f
#define ENTITY_AIR_SPEED 500.0f
#define ENTITY_AIR_SPEED_CAP 0.65f
#define ENTITY_AIR_ACCEL 800.0f
#define ENTITY_GROUND_WALK_SPEED 6.5f
#define ENTITY_GROUND_RUN_SPEED 10.0f
#define ENTITY_GROUND_ACCEL_WALK 7.0f
#define ENTITY_GROUND_ACCEL_RUN 9.0f

void physics_p_air_movement(Player* p, float dt);
void physics_p_ground_movement(Player* p, float dt);
static void physics_e_air_movement(Entity* ent, float dt);
static void physics_e_ground_movement(Entity* ent, float dt);
static void physics_apply_friction(Entity* ent, float friction, float dt);
static void physics_air_accelerate(Entity* ent, Vec3 wishvel, float wishspeed, float accelerate,  float dt);
static void physics_clip_velocity(const Vec3* in, const Vec3* normal, Vec3* out, float overbounce);
static float physics_sample_ground_height(const Level* level, const Vec3* world_pos);
void physics_get_height_and_slope(const Level* level, const Vec3* world_pos, float* y_level, Vec3* slope);
float exp_decay(float max, float x, float rate);
float logistic_s(float max, float x, float start, float rate);

void handle_dash_client(Client* c, Player* p, float dt);

void physics_funny_bounds_check(Entity* ent);

Player* ray_check_player_collison(Level* level, Player* shooter, float max_ray_len, float step);
bool check_map_colision(Level* level, Entity* shooter, float max_ray_len);
static bool check_map_touching(Level* level, Entity* ent, float max_dist, Vec3* out_normal);


void physics_init(Level* level) {
    PhysicsWorld* world = &level->physics;
    /*
    world->tick_rate_physics = level->tick_rate;
    world->tick_freq_physics = 1.0f / (float)level->tick_rate;
    world->accumulator_physics = 0.0f;

    world->max_accumulator_physics = world->tick_freq_physics * MAX_TICK_DELAY;
    */
    world->gravity = (Vec3){0.0f, -9.81f, 0.0f};

}

/*
void physics_world_update(Level* level, float dt) {
    PhysicsWorld* world = &level->physics;
    world->accumulator_physics += dt;

    if (world->accumulator_physics > world->max_accumulator_physics) {world->accumulator_physics = world->max_accumulator_physics;}

    while (world->accumulator_physics >= world->tick_freq_physics) {
        physics_step(level, world->tick_freq_physics);
        world->accumulator_physics -= world->tick_freq_physics;
    }

}*/

void physics_step(Level* level, float dt) {
    PhysicsWorld* world = &level->physics;

    for (int i = 0; i < hmlen(level->player_map); i++) {
        Player* p = &level->player_map[i].value;
        Entity* ent = &level->player_map[i].value.entity;

        ent->prev_position = ent->position;

        physics_update_states(level, ent);

        if (is_state(ent, GROUNDED)) {
            ent->high_y = ent->position.y;

            float horizonal_spd_mag = (ent->velocity.x * ent->velocity.x) + (ent->velocity.z * ent->velocity.z);
            if (p->movement.slide_queued && (horizonal_spd_mag > ((p->movement.walk_speed * p->movement.walk_speed)*0.8f))) {
                set_state(ent, SLIDING);
            } else {
                clear_state(ent, SLIDING);
            }

            physics_p_ground_movement(p, dt);

            float ground_y = 0.0f;
            Vec3 ground_normal = {0.0f, 1.0f, 0.0f};
            physics_get_height_and_slope(level, &ent->position, &ground_y, &ground_normal);

            if (ground_normal.y > EPSILON) {
                ent->velocity.y = -((ground_normal.x * ent->velocity.x) + (ground_normal.z * ent->velocity.z)) / ground_normal.y;
            }

            if(p->movement.jump_queued) {
                p->movement.jump_queued = false;
                float add_vel = p->movement.jump_vel - ent->velocity.y;
                if (add_vel > 0.0f) {
                    ent->velocity.y += p->movement.jump_vel;
                }
            }


        } else if (is_state(ent, IN_AIR)) {
            Vec3 steep_normal = {0.0f, 1.0f, 0.0f};
            bool touching_steep = check_map_touching(level, ent, ent->radius + 0.02f, &steep_normal);

            if (touching_steep) {
  
                float g_dot_n = vec3_dot(&world->gravity, &steep_normal);
                Vec3 g_normal = vec3_multiply(&steep_normal, g_dot_n);
                Vec3 g_tangent = vec3_subtract(&world->gravity, &g_normal);
                Vec3 slope_force = vec3_multiply(&g_tangent, (float)ent->mass);
                vec3_add_inplace(&ent->force, &slope_force);

                Vec3 clipped_vel = ent->velocity;
                physics_clip_velocity(&ent->velocity, &steep_normal, &clipped_vel, 1.0f);
                ent->velocity = clipped_vel;
                
            } else {
                Vec3 gravity_force = vec3_multiply(&world->gravity, (float)ent->mass);
                vec3_add_inplace(&ent->force, &gravity_force);
            }

            physics_p_air_movement(p, dt);

            ent->high_y = (ent->position.y > ent->high_y) ? ent->position.y : ent->high_y;
        }

        physics_funny_bounds_check(ent);

        //apply forces
        Vec3 delta_vel = vec3_multiply(&ent->force, dt / (float)ent->mass);
        vec3_add_inplace(&ent->velocity, &delta_vel);

        Vec3 delta_pos = vec3_multiply(&ent->velocity, dt);
        vec3_add_inplace(&ent->position, &delta_pos);

        //Always resolve terrain penetration, regardless of movement state.

        float y_level = physics_sample_ground_height(level, &ent->position);
        float min_y = ent->radius + y_level;
        if (ent->position.y < min_y) {
            ent->position.y = min_y;

            // Keep downhill slide velocity on steep slopes, but stop downward penetration on walkable ground.
            Vec3 touch_normal = {0.0f, 1.0f, 0.0f};
            bool touching_steep = check_map_touching(level, ent, ent->radius + 0.02f, &touch_normal);
            if (!touching_steep && ent->velocity.y < 0.0f) {
                ent->velocity.y = 0.0f;
            }
        }

        if (fabsf(ent->velocity.x) < EPSILON) ent->velocity.x = 0.0f;
        if (fabsf(ent->velocity.y) < EPSILON) ent->velocity.y = 0.0f;
        if (fabsf(ent->velocity.z) < EPSILON) ent->velocity.z = 0.0f;

        physics_update_states(level, ent);

        ent->force = (Vec3){0.0f, 0.0f, 0.0f};

        //check_dead(ent);
    }

    for (int i = 0; i < hmlen(level->ent_map); i++) {
        Entity* ent = &level->ent_map[i].value;

        physics_update_states(level, ent);

        if (is_state(ent, GROUNDED)) {
            physics_e_ground_movement(ent, dt);

            float ground_y = 0.0f;
            Vec3 ground_normal = {0.0f, 1.0f, 0.0f};
            physics_get_height_and_slope(level, &ent->position, &ground_y, &ground_normal);

            if (ground_normal.y > EPSILON) {
                ent->velocity.y = -((ground_normal.x * ent->velocity.x) + (ground_normal.z * ent->velocity.z)) / ground_normal.y;
            }

        } else if (is_state(ent, IN_AIR)) {
            Vec3 steep_normal = {0.0f, 1.0f, 0.0f};
            bool touching_steep = check_map_touching(level, ent, ent->radius + 0.02f, &steep_normal);

            if (touching_steep) {
                float g_dot_n = vec3_dot(&world->gravity, &steep_normal);
                Vec3 g_normal = vec3_multiply(&steep_normal, g_dot_n);
                Vec3 g_tangent = vec3_subtract(&world->gravity, &g_normal);
                Vec3 slope_force = vec3_multiply(&g_tangent, (float)ent->mass);
                vec3_add_inplace(&ent->force, &slope_force);

                Vec3 clipped_vel = ent->velocity;
                physics_clip_velocity(&ent->velocity, &steep_normal, &clipped_vel, 1.0f);
                ent->velocity = clipped_vel;

            } else {
                Vec3 gravity_force = vec3_multiply(&world->gravity, (float)ent->mass);
                vec3_add_inplace(&ent->force, &gravity_force);
            }

            physics_e_air_movement(ent, dt);

        }

        physics_funny_bounds_check(ent);

        Vec3 delta_vel = vec3_multiply(&ent->force, dt / (float)ent->mass);
        vec3_add_inplace(&ent->velocity, &delta_vel);

        Vec3 delta_pos = vec3_multiply(&ent->velocity, dt);
        vec3_add_inplace(&ent->position, &delta_pos);

        float y_level = physics_sample_ground_height(level, &ent->position);
        float min_y = ent->radius + y_level;
        if (ent->position.y < min_y) {
            ent->position.y = min_y;

            Vec3 touch_normal = {0.0f, 1.0f, 0.0f};
            bool touching_steep = check_map_touching(level, ent, ent->radius + 0.02f, &touch_normal);
            if (!touching_steep && ent->velocity.y < 0.0f) {
                ent->velocity.y = 0.0f;
            }
        }

        if (fabsf(ent->velocity.x) < EPSILON) ent->velocity.x = 0.0f;
        if (fabsf(ent->velocity.y) < EPSILON) ent->velocity.y = 0.0f;
        if (fabsf(ent->velocity.z) < EPSILON) ent->velocity.z = 0.0f;

        physics_update_states(level, ent);

        ent->force = (Vec3){0.0f, 0.0f, 0.0f};

        //check_dead(ent);

    }

}

void physics_step_player(Level* level, Player* p, float dt) {
    PhysicsWorld* world = &level->physics;
    Entity* ent = &p->entity;

    ent->prev_position = ent->position;
    ent->force = (Vec3){0.0f, 0.0f, 0.0f};

    physics_update_states(level, ent);

    if (is_state(ent, GROUNDED)) {
        ent->high_y = ent->position.y;

        float horizonal_spd_mag = (ent->velocity.x * ent->velocity.x) + (ent->velocity.z * ent->velocity.z);
        if (p->movement.slide_queued && (horizonal_spd_mag > ((p->movement.walk_speed * p->movement.walk_speed)*0.8f))) {
            set_state(ent, SLIDING);
        } else {
            clear_state(ent, SLIDING);
        }

        physics_p_ground_movement(p, dt);

        float ground_y = 0.0f;
        Vec3 ground_normal = {0.0f, 1.0f, 0.0f};
        physics_get_height_and_slope(level, &ent->position, &ground_y, &ground_normal);

        if (ground_normal.y > EPSILON) {
            ent->velocity.y = -((ground_normal.x * ent->velocity.x) + (ground_normal.z * ent->velocity.z)) / ground_normal.y;
        }

        if (p->movement.jump_queued) {
            p->movement.jump_queued = false;
            float add_vel = p->movement.jump_vel - ent->velocity.y;
            if (add_vel > 0.0f) {
                ent->velocity.y += p->movement.jump_vel;
            }
        }

    } else if (is_state(ent, IN_AIR)) {
        Vec3 steep_normal = {0.0f, 1.0f, 0.0f};
        bool touching_steep = check_map_touching(level, ent, ent->radius + 0.02f, &steep_normal);

        if (touching_steep) {
            float g_dot_n = vec3_dot(&world->gravity, &steep_normal);
            Vec3 g_normal = vec3_multiply(&steep_normal, g_dot_n);
            Vec3 g_tangent = vec3_subtract(&world->gravity, &g_normal);
            Vec3 slope_force = vec3_multiply(&g_tangent, (float)ent->mass);
            vec3_add_inplace(&ent->force, &slope_force);

            Vec3 clipped_vel = ent->velocity;
            physics_clip_velocity(&ent->velocity, &steep_normal, &clipped_vel, 1.0f);
            ent->velocity = clipped_vel;
        } else {
            Vec3 gravity_force = vec3_multiply(&world->gravity, (float)ent->mass);
            vec3_add_inplace(&ent->force, &gravity_force);
        }

        physics_p_air_movement(p, dt);

        ent->high_y = (ent->position.y > ent->high_y) ? ent->position.y : ent->high_y;
    }

    physics_funny_bounds_check(ent);

    Vec3 delta_vel = vec3_multiply(&ent->force, dt / (float)ent->mass);
    vec3_add_inplace(&ent->velocity, &delta_vel);

    Vec3 delta_pos = vec3_multiply(&ent->velocity, dt);
    vec3_add_inplace(&ent->position, &delta_pos);

    float y_level = physics_sample_ground_height(level, &ent->position);
    float min_y = ent->radius + y_level;
    if (ent->position.y < min_y) {
        ent->position.y = min_y;

        Vec3 touch_normal = {0.0f, 1.0f, 0.0f};
        bool touching_steep2 = check_map_touching(level, ent, ent->radius + 0.02f, &touch_normal);
        if (!touching_steep2 && ent->velocity.y < 0.0f) {
            ent->velocity.y = 0.0f;
        }
    }

    if (fabsf(ent->velocity.x) < EPSILON) ent->velocity.x = 0.0f;
    if (fabsf(ent->velocity.y) < EPSILON) ent->velocity.y = 0.0f;
    if (fabsf(ent->velocity.z) < EPSILON) ent->velocity.z = 0.0f;

    physics_update_states(level, ent);
    ent->force = (Vec3){0.0f, 0.0f, 0.0f};
}

void physics_update_states(Level* level, Entity* ent) {

    bool on_ground = check_map_colision(level, ent, ent->radius);

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

    float cur_spd = vec3_mag(&ent->velocity);
    if(cur_spd > m->air_fric_threshold) {physics_apply_friction(ent,  logistic_s(0.1f, cur_spd, m->air_fric_threshold, 0.5), dt);}

    Vec3 wishdir = (Vec3){m->wish_dir.x, 0.0f, m->wish_dir.z};

    if (vec3_mag_squared(&wishdir) > EPSILON) {vec3_normalize_inplace(&wishdir);}

    float wishspeed = m->air_speed;
    if (wishspeed > m->air_speed_cap) {wishspeed = m->air_speed_cap;}
    Vec3 wishvel = vec3_multiply(&wishdir, wishspeed);

    if (vec3_mag_squared(&wishvel) <= EPSILON) {return;}
    physics_air_accelerate(ent, wishvel, wishspeed, m->air_acceleration, dt);
}

void physics_p_ground_movement(Player* p, float dt) {
    Entity* ent = &p->entity;
    const Player_movement* m = &p->movement;      

    float fric = is_state(&p->entity, SLIDING)? m->slide_friction : m->ground_friction;


    if (!p->movement.jump_queued) {
        physics_apply_friction(ent, fric, dt);
    }

    Vec3 wishdir;
    float spd = 0.0f;
    if (is_state(ent, SLIDING)) { 
        wishdir = (Vec3){m->wish_dir.x + p->movement.cam_forward.x, 0.0f, m->wish_dir.z + p->movement.cam_forward.z};
        spd = m->air_speed;
        if (spd > m->air_speed_cap) {spd = m->air_speed_cap;}
    } else {
        wishdir = (Vec3){m->wish_dir.x, 0.0f, m->wish_dir.z};
        spd = is_state(ent, RUNNING) ? m->run_speed : m->walk_speed;
    }

    if (vec3_mag_squared(&wishdir) <= EPSILON) {return;}
    float accel = is_state(ent, RUNNING) ? m->ground_acceleration_run : m->ground_acceleration_walk;

    
    if (vec3_mag_squared(&wishdir) <= EPSILON) {return;}
    vec3_normalize_inplace(&wishdir);
    Vec3 wishvel = vec3_multiply(&wishdir, spd);


    physics_air_accelerate(ent, wishvel, spd, accel, dt);



}

static void physics_e_air_movement(Entity* ent, float dt) {
    float cur_spd = vec3_mag(&ent->velocity);
    if (cur_spd > ENTITY_AIR_FRICTION_THRESHOLD) {
        physics_apply_friction(ent, logistic_s(ENTITY_AIR_FRICTION_CAP, cur_spd, ENTITY_AIR_FRICTION_THRESHOLD, 0.5f), dt);
    }

    Vec3 wishdir = (Vec3){ent->acceleration.x, 0.0f, ent->acceleration.z};
    if (vec3_mag_squared(&wishdir) > EPSILON) {
        vec3_normalize_inplace(&wishdir);
    }

    float wishspeed = ENTITY_AIR_SPEED;
    if (wishspeed > ENTITY_AIR_SPEED_CAP) {
        wishspeed = ENTITY_AIR_SPEED_CAP;
    }

    Vec3 wishvel = vec3_multiply(&wishdir, wishspeed);
    if (vec3_mag_squared(&wishvel) <= EPSILON) {
        return;
    }

    physics_air_accelerate(ent, wishvel, wishspeed, ENTITY_AIR_ACCEL, dt);
}

static void physics_e_ground_movement(Entity* ent, float dt) {
    float fric = is_state(ent, SLIDING) ? ENTITY_SLIDE_FRICTION : ENTITY_GROUND_FRICTION;
    physics_apply_friction(ent, fric, dt);

    Vec3 wishdir = (Vec3){ent->acceleration.x, 0.0f, ent->acceleration.z};
    float spd = is_state(ent, RUNNING) ? ENTITY_GROUND_RUN_SPEED : ENTITY_GROUND_WALK_SPEED;

    if (vec3_mag_squared(&wishdir) <= EPSILON) {
        return;
    }

    float accel = is_state(ent, RUNNING) ? ENTITY_GROUND_ACCEL_RUN : ENTITY_GROUND_ACCEL_WALK;
    vec3_normalize_inplace(&wishdir);
    Vec3 wishvel = vec3_multiply(&wishdir, spd);

    physics_air_accelerate(ent, wishvel, spd, accel, dt);
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



static void physics_air_accelerate(Entity* ent, Vec3 wishvel, float wishspeed, float accelerate,  float dt) {
    float wishspd = wishspeed;

    vec3_normalize_inplace(&wishvel);
    float currentspeed = ent->velocity.x * wishvel.x + ent->velocity.z * wishvel.z;

    float addspeed = wishspd - currentspeed;
    if(addspeed <=0) {return;}
    float accelspeed = accelerate  * wishspeed * dt;

    if (accelspeed > addspeed) {accelspeed = addspeed;}

    Vec3 accel_vel = vec3_multiply(&wishvel, accelspeed);
    vec3_add_inplace(&ent->velocity, &accel_vel);
    
}

static void physics_clip_velocity(const Vec3* in, const Vec3* normal, Vec3* out, float overbounce) {
    if (!in || !normal || !out) {
        return;
    }

    float backoff = vec3_dot(in, normal) * overbounce;

    out->x = in->x - (normal->x * backoff);
    out->y = in->y - (normal->y * backoff);
    out->z = in->z - (normal->z * backoff);

    float adjust = vec3_dot(out, normal);
    if (adjust < 0.0f) {
        out->x -= normal->x * adjust;
        out->y -= normal->y * adjust;
        out->z -= normal->z * adjust;
    }
}



Player* ray_check_player_collison(Level* level, Player* shooter, float max_ray_len, float step) {
    if (!level || step <= EPSILON || max_ray_len <= 0.0f) {
        return NULL;
    }

    Vec3 dir = shooter->movement.cam_forward;
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

//check if grounded and walkable (angle < 1 radian)
bool check_map_colision(Level* level, Entity* shooter, float max_ray_len) {
    if (!level || !shooter || max_ray_len <= 0.0f) {
        return false;
    }

    const float max_walkable_angle_rad = MAX_WALK_ANGLE;
    const float min_walkable_normal_y = cosf(max_walkable_angle_rad);
    float y = 0.0f;
    Vec3 slope_normal = {0.0f, 1.0f, 0.0f};

    physics_get_height_and_slope(level, &shooter->position, &y, &slope_normal);

    if (shooter->position.y - y > max_ray_len + EPSILON) {
        return false;
    }

    return slope_normal.y >= min_walkable_normal_y;
}

static bool check_map_touching(Level* level, Entity* ent, float max_dist, Vec3* out_normal) {
    if (!level || !ent || max_dist <= 0.0f) {
        return false;
    }

    float y = 0.0f;
    Vec3 slope_normal = {0.0f, 1.0f, 0.0f};
    physics_get_height_and_slope(level, &ent->position, &y, &slope_normal);

    float gap = ent->position.y - y;
    if (gap > max_dist + EPSILON) {
        return false;
    }

    const float min_walkable_normal_y = cosf(MAX_WALK_ANGLE);
    if (slope_normal.y >= min_walkable_normal_y) {
        return false;
    }

    if (out_normal) {
        *out_normal = slope_normal;
    }
    return true;
}



void physics_funny_bounds_check(Entity* ent) {
    if (vec3_mag(&ent->position) > 350.0f) {
        Vec3 to_center = vec3_subtract(&(Vec3){0.0f, 0.0f, 0.0f}, &ent->position);
        if (vec3_mag_squared(&to_center) <= EPSILON) {return;}
        vec3_normalize_inplace(&to_center);

        float speed = vec3_mag(&ent->velocity);
        Vec3 backvel = vec3_multiply(&to_center, speed);

        ent->velocity = backvel;
    }
}

static float physics_sample_ground_height(const Level* level, const Vec3* world_pos) {
    float y_level = 0.0f;
    physics_get_height_and_slope(level, world_pos, &y_level, NULL);
    return y_level;
}

void physics_get_height_and_slope(const Level* level, const Vec3* world_pos, float* y_level, Vec3* slope) {
    if (y_level) {
        *y_level = 0.0f;
    }
    if (slope) {
        *slope = (Vec3){0.0f, 1.0f, 0.0f};
    }

    if (!level || !world_pos || !level->ground.height_map) {
        return;
    }

    if (level->ground.xz_scale <= EPSILON) {
        return;
    }

    float world_origin_x = -((float)level->ground.x_size * level->ground.xz_scale * 0.5f);
    float world_origin_z = -((float)level->ground.z_size * level->ground.xz_scale * 0.5f);

    float local_x = (world_pos->x - world_origin_x) / level->ground.xz_scale;
    float local_z = (world_pos->z - world_origin_z) / level->ground.xz_scale;

    int map_x0 = (int)floorf(local_x);
    int map_z0 = (int)floorf(local_z);
    int map_x1 = map_x0 + 1;
    int map_z1 = map_z0 + 1;

    if (map_x0 < 0 || map_z0 < 0 || (uint32_t)map_x0 >= level->ground.x_size || (uint32_t)map_z0 >= level->ground.z_size) {
        return;
    }

    if ((uint32_t)map_x1 >= level->ground.x_size) {
        map_x1 = map_x0;
    }
    if ((uint32_t)map_z1 >= level->ground.z_size) {
        map_z1 = map_z0;
    }

    float tx = local_x - (float)map_x0;
    float tz = local_z - (float)map_z0;

    float h00 = (float)level->ground.height_map[map_x0][map_z0];
    float h10 = (float)level->ground.height_map[map_x1][map_z0];
    float h01 = (float)level->ground.height_map[map_x0][map_z1];
    float h11 = (float)level->ground.height_map[map_x1][map_z1];

    float hx0 = h00 + tx * (h10 - h00);
    float hx1 = h01 + tx * (h11 - h01);
    float y = hx0 + tz * (hx1 - hx0);

    if (y_level) {
        *y_level = y * level->ground.y_scale;
    }

    if (slope) {
        float dh_dmx = ((1.0f - tz) * (h10 - h00)) + (tz * (h11 - h01));
        float dh_dmz = ((1.0f - tx) * (h01 - h00)) + (tx * (h11 - h10));

        float dh_dx = (dh_dmx * level->ground.y_scale) / level->ground.xz_scale;
        float dh_dz = (dh_dmz * level->ground.y_scale) / level->ground.xz_scale;

        Vec3 normal = {-dh_dx, 1.0f, -dh_dz};
        *slope = vec3_normalize(&normal);
    }
}



float exp_decay(float max, float x, float rate) {
    return max * (1 - powf(2.71828182846f, (-x / rate)));
}


//variation of logistic function where you choose the the place to start the curve
float logistic_s(float max, float x, float start, float rate) {
    return max / (1 + powf(2.71828182846f, (-rate * (x - (start + 5.0f / rate)))));
}


void handle_dash_client(Client* c, Player* p, float dt) {

    if(!p->movement.dash_queued) {return;}
    if (p->dash.cast_wait_time > 0.0f) {p->movement.dash_queued = false; return;}
    if (p->dash.current_charges <= 0) {p->movement.dash_queued = false; return;}

    p->movement.dash_queued = false;

    p->dash.current_charges--;
    p->dash.cast_wait_time = p->dash.cast_wait;

    sound_play_id(&c->sound, SOUND_DASH, 0.2f);

    p->entity.position.y += 0.01f;
    p->movement.jump_queued = true;
    Vec3 wishdir = p->movement.cam_forward;
    wishdir = vec3_normalize(&wishdir);
    Vec3 wishflat = wishdir;
    wishflat.y = 0.0f;

    float wishspeed = p->dash.dash_vel;
    float currentspeed = vec3_dot(&p->entity.velocity, &wishdir);
    float addspeed = (currentspeed < 0.0f) ? (wishspeed - currentspeed) : wishspeed;


    Vec3 accel = vec3_multiply(&wishdir, addspeed);
    vec3_add_inplace(&p->entity.velocity, &accel);

}