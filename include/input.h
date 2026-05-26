#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <SDL3/SDL.h>

#include "window.h"

typedef struct {
    bool     relative_mouse;
    const Uint8 *kb_state;
    Uint8     kb_prev[SDL_SCANCODE_COUNT];
    Uint32    mb;
    Uint32    mb_prev;
} InputHandle;

typedef struct {
    float  x;
    float  y;
    Uint32 mb;
} dMouse;


void input_init(InputHandle *input, Window *window);
void input_update(InputHandle *input);   // call once per frame

dMouse input_mouse(InputHandle *input);

bool input_key_held    (InputHandle *input, SDL_Scancode sc);
bool input_key_pressed (InputHandle *input, SDL_Scancode sc); //oneshotting
bool input_key_released(InputHandle *input, SDL_Scancode sc); 

bool input_mb_held    (InputHandle *input, Uint32 mask);
bool input_mb_pressed (InputHandle *input, Uint32 mask);
bool input_mb_released(InputHandle *input, Uint32 mask);

#endif // INPUT_H