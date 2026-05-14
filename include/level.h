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

typedef struct {
    uint32_t key;
    Player value;
} PlayerMapEntry;

typedef struct {
    uint32_t key;
    Entity value;
} EntityMapEntry;


typedef struct Level {
    bool server;
    uint32_t tick_rate;

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

} Level;

bool level_create(Level* level, uint32_t tick_rate);
bool level_update(Level* level, float delta_time);
bool level_add_model(Level* level, Model* model);
bool level_add_player(Level* level, uint64_t uqid, uint32_t server_id);
void level_destroy(Level* level);

#endif // LEVEL_H