#include "window.h"

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

void window_swap_buffers(Window* win) {
    if (!win || !win->window) return;
    
    SDL_GL_SwapWindow(win->window);
}

void window_destroy(Window* win) {
    if (!win) return;
    
    if (win->gl_context) {
        SDL_GL_DestroyContext(win->gl_context);
        win->gl_context = NULL;
    }
    
    if (win->window) {
        SDL_DestroyWindow(win->window);
        win->window = NULL;
    }
}

void window_quit(void) {SDL_Quit();}