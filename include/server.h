#ifndef SERVER_H
#define SERVER_H

#include <stdbool.h>
#include <stdio.h>
#include "enet.h"
#include "level.h"
#include "packet.h"
#include "gun.h"
#include "sound.h"
#include "timing.h"
#include "stb_ds.h"
#include "event.h"

#define MAX_CLIENTS 32 //arbitrary
#define MAX_TICK_DELAY 10



typedef struct Server {
    bool initialized;

    Level level;

    ENetAddress address;
    ENetHost* e_server;
    uint16_t port;

    Timing time;
    
} Server;

bool server_startup(Server* server);

bool server_run(Server* server);

void server_close(Server* server);

#endif // SERVER_H