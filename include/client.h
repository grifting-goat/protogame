#ifndef CLIENT_H
#define CLIENT_H

#include <stdbool.h>
#include "enet.h"
#include "window.h"
#include "level.h"
#include "player.h"
#include "render.h"
#include "camera.h"


typedef struct Client {
    Window win;
    Level level;
    Player* player;
    
    Camera player_camera;
    InputHandle player_input;

    uint64_t unique_id;
    uint32_t server_id;

    ENetHost* e_client;
    ENetPeer* server_peer;
    ENetAddress address;

    bool enet_connect_attempted;
    bool enet_connected;

} Client;

bool client_startup(Client* client, const char* host);

bool client_run(Client* client);

void client_close(Client* client);

#endif // CLIENT_H