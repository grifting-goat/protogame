#ifndef LEVEL_H
#define LEVEL_H

#include <stdint.h>
#include <stdbool.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

typedef struct {
    uint32_t tick_rate;

    Uint64 perf_freq;
    Uint64 last_time;
    Uint64 fps_time_accum;
    int frame_count;

    bool initialized;
} Level;

bool level_create(Level* level, uint32_t tick_rate);
bool level_update(Level* level, float delta_time);
void level_destroy(Level* level);

#endif // LEVEL_H