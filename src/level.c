#include "level.h"
#include <stdlib.h>
#include "stb_ds.h"



bool level_create(Level* level, uint32_t tick_rate) {
    if (!level) return false;
    
    level->tick_rate = tick_rate;
    level->perf_freq = SDL_GetPerformanceFrequency();
    level->last_time = SDL_GetPerformanceCounter();
    level->fps_time_accum = 0;
    level->frame_count = 0;
    level->initialized = true;

    level->model_count = 0;
    level->models = NULL;
    level->ent_map = NULL;
    level->player_map = NULL;
    level->server = false;

    physics_init(level);
    
    return true;
}

bool level_update(Level* level, float delta_time) {
    if (!level || !level->initialized) return false;

    if (level->server) {
        physics_world_update(level, delta_time);
    } else {
        physics_step(level, delta_time);
    }

    return true;
}

bool level_add_player(Level* level, uint64_t uqid, uint32_t server_id) {
    if (!level || !level->initialized) return false;

    Player new_player = player_create();
    new_player.unqid = uqid;
    new_player.server_id = server_id;

    if (!level->server) {
        new_player.entity.model = temp_create_sphere(32, 16, 1.0f);
    }

    hmput(level->player_map, server_id, new_player);

    return true;
}

bool level_add_model(Level* level, Model* model) {
    if (!level || !model || !level->initialized) {
        return false;
    }

    if (level->server) {
        return true;
    }

    size_t new_count = (size_t)level->model_count + 1;
    Model* resized = (Model*)realloc(level->models, new_count * sizeof(*level->models));
    if (!resized) {
        return false;
    }

    level->models = resized;
    level->models[level->model_count] = *model;
    level->model_count = (uint32_t)new_count;
    return true;
}

void level_destroy(Level* level) {
    if (!level) return;

    free(level->models);
    level->models = NULL;
    level->model_count = 0;

    hmfree(level->ent_map);
    level->ent_map = NULL;

    hmfree(level->player_map);
    level->player_map = NULL;
}
