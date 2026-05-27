#ifndef SERVER_H
#define SERVER_H

#include <stdbool.h>
#include <stdio.h>
#include "enet.h"
#include "level.h"
#include "packet.h"
#include "gun.h"
#include "sound.h"

#define MAX_CLIENTS 32
#define MAX_TICK_DELAY 10

extern const char* server_tag;


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


typedef struct Server {
    Level level;

    ENetAddress address;
    ENetHost* e_server;

    Timing time;
    
} Server;

bool server_startup(Server* server);

bool server_run(Server* server);

void server_close(Server* server);

#endif // SERVER_H