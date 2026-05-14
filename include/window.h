#ifndef WINDOW_H
#define WINDOW_H

#include <stdbool.h>
#include <SDL3/SDL.h>
#include "overlay.h"

typedef struct {
    const char* key;
    OverlayText value;
} OverlayTextMapEntry;

typedef struct {
    SDL_Window* window;
    SDL_GLContext gl_context;
    int width;
    int height;
    bool fullscreen;

    OverlayTextMapEntry* overlay_text_map;

} Window;

bool window_init(void);

bool window_create(Window* win, const char* title, int width, int height, bool fullscreen);

void window_set_icon(Window* win, const char* icon_path);

void window_toggle_fullscreen(Window* win);

bool window_add_overlay(Window* win, const char* key, const char* text, int x, int y);

bool window_update_overlay(Window* win, const char* key, const char* text);

void window_render_overlay(Window* win);

void window_swap_buffers(Window* win);

void window_destroy(Window* win);

void window_quit(void);

#endif // WINDOW_H