#ifndef UNIFIED_H
#define UNIFIED_H

#include <stdint.h>
#include "gun.h"
#include "player.h"
#include "states.h"

typedef struct Level Level;
typedef struct Entity Entity;
typedef struct Server Server;
typedef struct Client Client;


/*

timing struct

*/

typedef struct {

    uint32_t tick_rate;
    uint32_t tick;
    float tick_time;
    double server_time;

    float accumulator;
    float max_accumulator;

    uint64_t perf_freq;
    uint64_t last_time;
    uint64_t fps_time_accum;
    uint32_t frame_count;

} Timing;


typedef struct {

	int				tick;
	Vec3		    angles;
	uint32_t	    buttons;
	uint32_t		gun_idx;

    Vec3            wishdir;

} usercmd_t; 

typedef struct {
    int tick;
    Vec3 cam_forward;
    Vec3 position;
    Vec3 velocity;
    float health;

    uint32_t state;
    uint32_t gun_idx;

} userstate_t;


/* 

timing operations

*/

static inline void update_gun_cooldowns(Player* p, const float dt) {
    Gun_stats* gun = &p->guns.guns[p->gun_idx];
    if (gun->wait_time) {
        gun->wait_time -= dt;
        if (gun->wait_time < 0.0f) {
            gun->wait_time = 0.0f;
        }
    }
}


// this should be replaced with ECS 

static inline void update_dash(Player_dash* d , const float dt) {

    if (d->cast_wait_time > 0.0f) {
        d->cast_wait_time -= dt;
        if (d->cast_wait_time <= 0.0f) {d->cast_wait_time = 0.0f;}
    }

    if (d->recharge_wait_time > 0.0f && d->current_charges < d->max_charges) {
        d->recharge_wait_time -= dt;
        
    }

    if (d->recharge_wait_time <= 0.0f && d->current_charges < d->max_charges) {
        d->current_charges++;
        d->recharge_wait_time = d->recharge_wait + d->recharge_wait_time;
    }

}

static inline void handle_dash(Player* p) {

    //dash really should be part of an ECS but i dont have that yet

    if (p->dash.cast_wait_time > 0.0f) {p->movement.dash_queued = false; return;}
    if (p->dash.current_charges <= 0) {p->movement.dash_queued = false; return;}

    p->dash.current_charges--;
    p->dash.cast_wait_time = p->dash.cast_wait;

    //sound_play_name(&c->sound, "dash", 0.2f);

    p->entity.position.y += 0.01f;
    Vec3 wishdir = p->cam_forward;
    wishdir = vec3_normalize(&wishdir);
    Vec3 wishflat = wishdir;
    wishflat.y = 0.0f;

    float wishspeed = p->dash.dash_vel;
    float currentspeed = vec3_dot(&p->entity.velocity, &wishdir);
    float addspeed = (currentspeed < 0.0f) ? (wishspeed - currentspeed) : wishspeed;


    Vec3 accel = vec3_multiply(&wishdir, addspeed);
    vec3_add_inplace(&p->entity.velocity, &accel);

}


static inline void handle_jump(Player* p) {
    if (!p->movement.can_jump) {return;}
    float add_vel = p->movement.jump_vel - p->entity.velocity.y;
    if (add_vel > 0.0f) {
        p->entity.velocity.y += p->movement.jump_vel;
        set_state(&p->entity, IN_AIR);
        clear_state(&p->entity, GROUNDED);
        printf("jumped!\n");
    }
}





static inline void proccess_action(actionType action, Player* player) {

    switch (action) {
        case DASH:
            handle_dash(player);
            break;
        case JUMP:
            handle_jump(player);
            break;

        default:
            printf("bad action\n");
            break;



    }


}





#endif //UNIFIED_H