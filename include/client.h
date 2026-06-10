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

#include "unified.h"




typedef struct {
    Vec3 start;
    Vec3 end;

    Vec3 color;

    float lifespan;

} Tracer;




typedef struct Client {
    Window win;
    Level level;
    Player* player; //pointer to the object in level, makes it easy but dangerous
    SoundSystem sound;

    Timing time;
    
    Camera player_camera;
    InputHandle player_input;

    uint64_t unique_id;
    uint32_t server_id;

    ENetHost* e_client;
    ENetPeer* server_peer;
    ENetAddress address;

    bool enet_connect_attempt_active;
    bool enet_connected;
    bool established_server_connnection; //happens after id exchange

    Tracer tracers[32];
    uint32_t tracer_count;


    ModelHashMap* model_cache; //premature optimisation?

    Model gun_models[3]; //disgusting but works
    Vec3 gun_view_offset;

    bool dead;
    bool aiming;
    float mouse_sensitivity;
    float hit_overlay_timer;

    usercmd_t input_buffer[INPUT_BUFFER_SIZE];
    uint32_t input_buffer_idx;

    userstate_t state_buffer[STATE_BUFFER_SIZE];
    uint32_t state_buffer_idx;

    uint32_t actions;

    Vec3 stationary_vec;
    Model dead_mdl;

    Event_bus bus;

    Model* models;
    uint32_t model_count;

} Client;

bool client_startup(Client* client, const char* host, uint32_t port);

bool client_run(Client *client);

void client_render(Client *client, const float alpha);

void client_close(Client* client);

#endif // CLIENT_H