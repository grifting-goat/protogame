#include "server.h"


bool server_startup(Server* server){
    if (!server) return false;
    level_create(&server->level, 128);
    printf("server started...\n");
    return 1;
}

bool server_run(Server* server) {
    if (!server) return false;

    //timing
    Uint64 now = SDL_GetPerformanceCounter();
    Uint64 frame_ticks = now - server->level.last_time;
    float dt = (float)frame_ticks / (float)server->level.perf_freq;
    server->level.last_time = now;


    server->level.fps_time_accum += frame_ticks;
    server->level.frame_count++;
        if (server->level.fps_time_accum >= server->level.perf_freq) {
            double fps = (double)server->level.frame_count * (double)server->level.perf_freq / (double)server->level.fps_time_accum;
            printf("Passes:%d\r", server->level.frame_count);
            fflush(stdout);
            server->level.fps_time_accum = 0;
            server->level.frame_count = 0;
        }

    level_update(&server->level, dt);

    return true;
}

void server_close(Server* server) {
    if (!server) return;
    level_destroy(&server->level);
    printf("\nserver closed!\n");
}