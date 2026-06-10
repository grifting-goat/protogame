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


#define INPUT_BUFFER_SIZE 512
#define STATE_BUFFER_SIZE 512


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
	uint32_t	    actions;
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

typedef struct {

    uint8_t peer_id;

    char* name[32];

    //state buffer
    userstate_t state_buffer[STATE_BUFFER_SIZE];
    uint32_t state_buffer_idx;

    //input buffer
    usercmd_t input_buffer[INPUT_BUFFER_SIZE];
    uint32_t input_buffer_idx;

    usercmd_t latest_cmd;  // most recently received cmd, applied every server tick

    uint64_t unique_id;


    Event_bus bus;

    Player* player;
    
} client_t;


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
    }
}

static inline void process_action(actionType action, Player* player) {

    switch (action) {
        case DASH:
            handle_dash(player);
            break;
        case JUMP:
            handle_jump(player);
            break;
        case SHOOT:
            break;

        default:
            printf("bad action\n");
            break;
    }

}

static inline void process_events(Event_bus* bus, Player* p) {
    Event event;

    while(true) {
        event = popEvent(bus);

        if (event.eventType == SE_NONE) {
            return;
        }
        //printf("event: %d ", event.eventType);

        switch (event.eventType) {

        case SE_NONE:
            break;
        case SE_ACTION:
            process_action((actionType)event.value, p);
            break;
		case SE_KEY:
			//key_event(level, event.value, event.value2, p);
			break;
		case SE_CHAR:
			break;
		case SE_MOUSE:
			break;
		case SE_CONSOLE:

			break;
		case SE_PACKET:
            break;
        default:
            printf("bad event\n");
        }

        if (event.ptr) {
			free(event.ptr);
		}

    }
}


static inline userstate_t get_userstate(const Player* p, int tick) {
    userstate_t state = {0};
    state.tick        = tick;
    state.position    = p->entity.position;
    state.velocity    = p->entity.velocity;
    state.health      = p->entity.health;
    state.state       = p->entity.states;
    state.cam_forward = p->cam_forward;
    state.gun_idx     = p->gun_idx;
    return state;
}

static inline void apply_userstate(Player* p, const userstate_t* state) {
    if (!p || !state) { return; }
    p->entity.position  = state->position;
    p->entity.velocity  = state->velocity;
    p->entity.health    = state->health;
    p->entity.states    = state->state;
    p->cam_forward      = state->cam_forward;
    p->gun_idx          = state->gun_idx;
}

static inline void push_usercmd_buffer(usercmd_t* cmd_buffer, uint32_t* buf_idx, uint32_t buf_size, usercmd_t* cmd) {
    if (!cmd_buffer || !buf_idx) {return;}
    if (*buf_idx >= buf_size) {return;}
    cmd_buffer[*buf_idx] = *cmd;
    *buf_idx = (*buf_idx + 1) % buf_size;
}


static inline void push_client_buffer(client_t* client_buffer, uint32_t* buf_idx, uint32_t buf_size, client_t* client) {
    if (!client_buffer || !buf_idx) {return;}
    if (*buf_idx >= buf_size) {return;}
    client_buffer[*buf_idx] = *client;
    *buf_idx = (*buf_idx + 1) % buf_size;
}
static inline void push_state_buffer(userstate_t* state_buffer, uint32_t* buf_idx, uint32_t buf_size, userstate_t* state) {
    if (!state_buffer || !buf_idx) {return;}
    if (*buf_idx >= buf_size) {return;}
    state_buffer[*buf_idx] = *state;
    *buf_idx = (*buf_idx + 1) % buf_size;
}




#endif //UNIFIED_H