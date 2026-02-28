#include "input.h"

void input_init(InputHandle* input, Window* window) {
    //hard code config for now
    input->relative_mouse = true;
    input->kb_state = SDL_GetKeyboardState(NULL);

    SDL_SetWindowRelativeMouseMode(window->window, input->relative_mouse);
}

dMouse input_mouse(InputHandle* input) {
    float mx = 0, my = 0;
    if (input->relative_mouse) {SDL_GetRelativeMouseState(&mx, &my);}
    else {printf("[input] not in relative mode... nothing else is implemented yet\n");}
    return (dMouse){mx,my,(Uint32)(SDL_GetMouseState(NULL, NULL))};
}