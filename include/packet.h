#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>
#include "engine_math.h"
#include "gun.h"
#include "sound.h"

//fix this with enum

typedef enum {
    ON_CONNECT = 0,
    ADD_PLAYER,
    REMOVE_PLAYER,
    SHOOT_PACKET,
    ADD_TRACER,
    RESPAWN,
    DEAD_PACKET,
    HIT_VERIFY,
    ADD_SOUND,
    AUTH_STATE,
    AUTH_FULL_SYNC,
    USERCMD_PACKET
} packet_t;

typedef struct {

	int				tick;
	Vec3		    angles;
	uint32_t	    buttons;
	uint32_t		gun_idx;

    Vec3            wishdir;

} usercmd_t; //move to elsewhere

typedef struct {
    int tick;
    Vec3 cam_forward;
    Vec3 position;
    Vec3 velocity;

    uint32_t state;
    uint32_t gun_idx;

} userstate_t; //move to elsewhere

typedef enum {
    DASH = 0,
    SHOOT,
    JUMP,
    SLIDE

} actions; //move to events


typedef struct {
    uint8_t pckt_id;
    uint32_t server_id;

    uint32_t tick;
    double server_time
}
Packet_on_connect; //sent from server to client


typedef struct {
    uint8_t pckt_id;
    uint32_t server_id;

    uint64_t unique_id
}
Packet_add_player; // client->server and server->client

typedef struct {
    uint8_t pckt_id;
    uint32_t server_id;
}
Packet_remove_player;


typedef struct {
    uint8_t pckt_id;

    Vec3 exact_angle;

    uint32_t gun_idx;
    uint32_t seed;

} Packet_shoot;


typedef struct {
    uint8_t pckt_id;
    uint32_t server_id;

    Vec3 source;
    Vec3 dest;
    float time;

} Packet_add_tracer;


typedef struct {
    uint8_t pckt_id;
    uint32_t server_id;

    Vec3 pos;
    Vec3 vel;
    float health;

} Packet_respawn;

typedef struct {
    uint8_t pckt_id;

} Packet_dead;

typedef struct {
    uint8_t pckt_id;

    uint32_t hit_server_id;

    uint32_t gun_idx;
    float damage_amount;

    bool kill;

} Packet_hit_verify;

typedef struct {
    uint8_t pckt_id;

    uint32_t server_id;
    bool client_side;

    Vec3 location;
    SoundID sound_id;

    float volume;
    float range;

} Packet_add_sound;

typedef struct {
    uint8_t pckt_id;

    Vec3 position;
    Vec3 velocity;

    float health;

    uint32_t state

} Packet_auth_state;


typedef struct {
    uint8_t pckt_id;

    Vec3 position;
    Vec3 velocity;

    float health;
    uint32_t state;

    //other stuff



} Packet_auth_full_sync;



typedef struct {
    uint8_t pckt_id;
    usercmd_t cmd;

} Packet_usercmd;













#endif //PACKET_H