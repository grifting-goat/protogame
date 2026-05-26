#ifndef LEVEL_H
#define LEVEL_H

#include <stdint.h>
#include <stdbool.h>
#include <SDL3/SDL.h>

#include "input.h"
#include "entity.h"
#include "player.h"
#include "camera.h"
#include "help.h"
#include "physics.h"

#include "ground.h"
#include "event.h"


typedef struct Server Server;
typedef struct Client Client;

typedef struct {
    uint32_t key;
    Player value;
} PlayerMapEntry;

typedef struct {
    uint32_t key;
    Entity value;
} EntityMapEntry;

#define MAX_TICK_DELAY 10

typedef struct Level {
    bool server;

    int tick;
    uint32_t tick_rate;
    float tick_time;
    uint32_t server_time;

    float accumulator;
    float max_accumulator;

    Uint64 perf_freq;
    Uint64 last_time;
    Uint64 fps_time_accum;
    int frame_count;

    bool initialized;

    PlayerMapEntry* player_map;

    EntityMapEntry* ent_map;

    Model* models;
    uint32_t model_count;

    PhysicsWorld physics;
    Ground ground;


    Server* server_ref;
    Client* client_ref;

    Vec3 level_spawn[8];

    sysEventBus event_bus;

} Level;

bool level_create(Level* level, uint32_t tick_rate);
bool level_update(Level* level, float delta_time);
bool level_add_model(Level* level, Model* model);
bool level_add_player(Level* level, uint64_t uqid, uint32_t server_id);
bool level_add_ent_death(Level* level, uint32_t server_id);
void level_destroy(Level* level);

void level_proccess_events(Level* level);

#endif // LEVEL_H