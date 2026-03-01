#include "level.h"



bool level_create(Level* level, uint32_t tick_rate) {
    if (!level) return false;
    
    level->tick_rate = tick_rate;
    level->perf_freq = SDL_GetPerformanceFrequency();
    level->last_time = SDL_GetPerformanceCounter();
    level->fps_time_accum = 0;
    level->frame_count = 0;
    level->initialized = true;
    
    
    return true;
}

bool level_update(Level* level, float delta_time) {
    if (!level || !level->initialized) return false;

    return true;
}

void level_destroy(Level* level) {
    if (!level) return;
    
}
