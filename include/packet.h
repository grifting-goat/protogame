#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>
#include "engine_math.h"
#include "gun.h"

#define PCKT_SERVER_ID 0x00
#define PCKT_CLIENT_ACK 0x01
#define PCKT_ADD_PLAYER 0x02
#define PCKT_CLIENT_STATE 0x03
#define PCKT_SERVER_STATE 0x04
#define PCKT_REMOVE_PLAYER 0x05

#define PCKT_SHOOT 0x06
#define PCKT_SERVER_AUTH_KNOCK 0x07
#define PCKT_TRACER 0x08
#define PCKT_SERVER_AUTH_RESPAWN 0x09

typedef struct {
    uint8_t pckt_id;
    uint32_t server_id;
}
Packet_server_id;

typedef struct {
    uint8_t pckt_id;
    uint32_t server_id;

    Vec3 pos;
    Vec3 vel;
    Vec3 cam_dir;

    uint32_t state;
    float health;

    Vec3 cam_offset;
    
} Packet_state;


typedef struct {
    uint8_t pckt_id;
    uint32_t server_id;
    uint64_t uqid;

} Packet_client_ack;


typedef struct {
    uint8_t pckt_id;
    uint32_t server_id;
    uint64_t uqid;

} Packet_player;


typedef struct {
    uint8_t pckt_id;
    uint32_t server_id;

    uint32_t gun_idx;
    uint32_t seed;

} Packet_shoot;


typedef struct {
    uint8_t pckt_id;
    uint32_t server_id;

    Vec3 source;
    Vec3 dest;
    float time;

} Packet_tracer;

typedef struct {
    uint8_t pckt_id;
    uint32_t server_id;

    Vec3 vel_knock;

} Packet_server_auth_knockback;


typedef struct {
    uint8_t pckt_id;
    uint32_t server_id;

    Vec3 pos;
    Vec3 vel;
    float heatlh;

} Packet_server_auth_respawn;







#endif //PACKET_H