#ifndef CLIENT_H
#define CLIENT_H

#include <stdbool.h>
#include "window.h"
#include "level.h"

typedef struct {
    Window win;
    Level level;
} Client;

bool client_startup(Client* client);

bool client_run(Client* client);

void client_close(Client* client);

#endif // CLIENT_H