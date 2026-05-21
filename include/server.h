#ifndef SERVER_H
#define SERVER_H

#include <stdbool.h>
#include <stdio.h>
#include "enet.h"
#include "level.h"
#include "packet.h"
#include "gun.h"

#define MAX_CLIENTS 32

extern const char* server_tag;

typedef struct Server {
    Level level;

    ENetAddress address;
    ENetHost* e_server;

    Gun_stats guns[3];
} Server;

bool server_startup(Server* server);

bool server_run(Server* server);

void server_close(Server* server);

#endif // SERVER_H