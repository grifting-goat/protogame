#ifndef EERIE_WINDOW_H
#define EERIE_WINDOW_H

#include <SDL3/SDL.h>
#include <stdbool.h>
#include <SDL3/SDL_vulkan.h>

typedef struct {
    SDL_Window* window;
    int width;
    int height;
    bool fullscreen;
} Window;

bool window_init(void);

bool window_create(Window* win, const char* title, int width, int height, bool fullscreen);

void window_set_icon(Window* win, const char* icon_path);

void window_toggle_fullscreen(Window* win);

void window_destroy(Window* win);

void window_quit(void);

#endif // EERIE_WINDOW_H