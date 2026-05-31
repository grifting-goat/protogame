#include "input.h"

void input_init(InputHandle *input, Window *window) {
    input->relative_mouse = true;
    input->kb_state = SDL_GetKeyboardState(NULL);
    memset(input->kb_prev, 0, sizeof(input->kb_prev));
    input->mb      = 0;
    input->mb_prev = 0;

    SDL_SetWindowRelativeMouseMode(window->window, input->relative_mouse);
}

void input_update(InputHandle *input) {
    memcpy(input->kb_prev, input->kb_state, SDL_SCANCODE_COUNT);
    input->mb_prev = input->mb;

    input->mb = SDL_GetMouseState(NULL, NULL);
}

dMouse input_mouse(InputHandle *input) {
    float mx = 0, my = 0;
    if (input->relative_mouse) {
        SDL_GetRelativeMouseState(&mx, &my);
    } else {
        printf("[input] not in relative mode — nothing else implemented yet\n");
    }
    return (dMouse){ mx, my, input->mb };
}

bool input_key_held(InputHandle *input, SDL_Scancode sc) {
    return input->kb_state[sc];
}

bool input_key_pressed(InputHandle *input, SDL_Scancode sc) {
    return input->kb_state[sc] && !input->kb_prev[sc];
}

bool input_key_released(InputHandle *input, SDL_Scancode sc) {
    return !input->kb_state[sc] && input->kb_prev[sc];
}

//buttons fit in 32 bits so we get to use masking -- yippie

bool input_mb_held(InputHandle *input, Uint32 mask) {
    return (input->mb & mask) != 0;
}

bool input_mb_pressed(InputHandle *input, Uint32 mask) {
    return (input->mb & mask) && !(input->mb_prev & mask);
}

bool input_mb_released(InputHandle *input, Uint32 mask) {
    return !(input->mb & mask) && (input->mb_prev & mask);
}