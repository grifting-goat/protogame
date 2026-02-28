#include "client.h"


bool client_startup(Client* client) {
    if (!client) return false;

    level_create(&client->level, 128);

    if (!window_init()) {return 0;}

    if (!window_create(&client->win, "Proto Game", 1920, 1080, true)) {window_quit(); return 0;}
    window_set_icon(&client->win, "icon.bmp");

    return 1;
}


bool client_run(Client* client) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {if (event.type == SDL_EVENT_QUIT) {return 0;}}

    //timing
    Uint64 now = SDL_GetPerformanceCounter();
    Uint64 frame_ticks = now - client->level.last_time;
    float dt = (float)frame_ticks / (float)client->level.perf_freq;
    client->level.last_time = now;



    //printing fps move to hud controler later
    client->level.fps_time_accum += frame_ticks;
    client->level.frame_count++;
        if (client->level.fps_time_accum >= client->level.perf_freq) {
            double fps = (double)client->level.frame_count * (double)client->level.perf_freq / (double)client->level.fps_time_accum;
            client->level.fps_time_accum = 0;
            client->level.frame_count = 0;
        }

    level_update(&client->level, dt);

    return 1;
}


void client_close(Client* client) {
    if (!client) return;
    window_destroy(&client->win);
    window_quit();
    level_destroy(&client->level);
}