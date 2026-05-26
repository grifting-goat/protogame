#ifndef CLIENT_H
#define CLIENT_H

#include <stdbool.h>
#include "enet.h"
#include "window.h"
#include "level.h"
#include "player.h"
#include "gun.h"
#include "render.h"
#include "camera.h"
#include "sound.h"
#include "packet.h"

#define INPUT_BUFFER_SIZE 512


typedef struct {
    Vec3 start;
    Vec3 end;

    Vec3 color;

    float lifespan;

} Tracer;




typedef struct Client {
    Window win;
    Level level;
    Player* player;
    SoundSystem sound;

    int tick;
    
    Camera player_camera;
    InputHandle player_input;

    uint64_t unique_id;
    uint32_t server_id;

    ENetHost* e_client;
    ENetPeer* server_peer;
    ENetAddress address;

    bool enet_connect_attempted;
    bool enet_connected;

    Tracer tracers[32];
    uint32_t tracer_count;


    ModelHashMap* model_cache;

    Model gun_models[3];
    Vec3 gun_view_offset;

    bool dead;
    bool aiming;
    float mouse_sensitivity;
    float hit_overlay_timer;

    usercmd_t input_buffer[INPUT_BUFFER_SIZE];
    uint32_t input_buffer_idx;

    uint32_t actions;

    Vec3 stationary_vec;
    Model dead_mdl;

} Client;

bool client_startup(Client* client, const char* host, uint32_t port);

void client_render(Client *client, float alpha);

void client_add_tracer(Client* client, Tracer tracer);

void client_input_actions(Client* client);

void client_close(Client* client);

#endif // CLIENT_H