#ifndef SERVER_H
#define SERVER_H

#include <stdbool.h>
#include <stdio.h>
#include "level.h"

typedef struct {
    Level level;
} Server;

bool server_startup(Server* server);

bool server_run(Server* server);

void server_close(Server* server);

#endif // SERVER_H