#include "window.h"
#include <string.h>
#include "stb_ds.h"

//boilerplate

bool window_init(void) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {SDL_Log("Could not initialize SDL: %s", SDL_GetError());return false;}
    return true;
}

bool window_create(Window* win, const char* title, int width, int height, bool fullscreen) {
    if (!win) return false;
    
    win->width = width;
    win->height = height;
    win->fullscreen = fullscreen;
    win->overlay_text_map = NULL;
    sh_new_strdup(win->overlay_text_map);
    
    //create window with OpenGL support
    win->window = SDL_CreateWindow(
        title,
        width, height,
        SDL_WINDOW_OPENGL
    );
    
    if (!win->window) {
        SDL_Log("Could not create window: %s", SDL_GetError());
        return false;
    }
    
    if (fullscreen) {
        SDL_SetWindowFullscreen(win->window, true);
    }
    
    //Create OpenGL context
    win->gl_context = SDL_GL_CreateContext(win->window);
    if (!win->gl_context) {
        SDL_Log("Could not create OpenGL context: %s", SDL_GetError());
        SDL_DestroyWindow(win->window);
        return false;
    }

    if (!overlay_init("UncialAntiqua-Regular.ttf", 30)) {
        SDL_Log("Could not initialize SDL_ttf overlay font: %s", SDL_GetError());
        SDL_GL_DestroyContext(win->gl_context);
        win->gl_context = NULL;
        SDL_DestroyWindow(win->window);
        win->window = NULL;
        return false;
    }
    
    
    return true;
}

void window_set_icon(Window* win, const char* icon_path) {
    if (!win || !win->window || !icon_path) return;
    
    SDL_Surface* icon = SDL_LoadBMP(icon_path);
    if (icon) {
        SDL_SetWindowIcon(win->window, icon);
        SDL_DestroySurface(icon);
    }
}

void window_toggle_fullscreen(Window* win) {
    if (!win || !win->window) return;
    
    win->fullscreen = !win->fullscreen;
    SDL_SetWindowFullscreen(win->window, win->fullscreen);
}

bool window_add_overlay(Window* win, const char* key, const char* text, int x, int y) {
    if (!win || !key || !text) return false;

    OverlayText glob;
    memset(&glob, 0, sizeof(glob));
    if (!overlay_create_text(&glob, text, x, y)) return false;

    int idx = shgeti(win->overlay_text_map, key);
    if (idx != -1) {
        overlay_destroy_text(&win->overlay_text_map[idx].value);
        win->overlay_text_map[idx].value = glob;
    } else {
        shput(win->overlay_text_map, key, glob);
    }

    return true;
}

bool window_update_overlay(Window* win, const char* key, const char* text) {
    if (!win || !key || !text) return false;
    int idx = shgeti(win->overlay_text_map, key);
    if (idx == -1) return false;

    OverlayText* ov = &win->overlay_text_map[idx].value;
    return overlay_create_text(ov, text, ov->x, ov->y);
}

void window_render_overlay(Window* win) {
    if (!win) return;

    for (int i = 0; i < shlen(win->overlay_text_map); i++) {
        overlay_render_text(&win->overlay_text_map[i].value, win->width, win->height);
    }
}

void window_swap_buffers(Window* win) {
    if (!win || !win->window) return;
    
    SDL_GL_SwapWindow(win->window);
}

void window_destroy(Window* win) {
    if (!win) return;

    for (int i = 0; i < shlen(win->overlay_text_map); i++) {
        overlay_destroy_text(&win->overlay_text_map[i].value);
    }

    shfree(win->overlay_text_map);
    win->overlay_text_map = NULL;
    
    if (win->gl_context) {
        SDL_GL_DestroyContext(win->gl_context);
        win->gl_context = NULL;
    }
    
    if (win->window) {
        SDL_DestroyWindow(win->window);
        win->window = NULL;
    }

    overlay_quit();


}

void window_quit(void) {SDL_Quit();}