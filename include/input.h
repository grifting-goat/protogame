#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <SDL3/SDL.h>

#include "window.h"


//super simple rn but hopefully a useful abstraction later

typedef struct {
    bool relative_mouse;
    const Uint8* kb_state;
} InputHandle;

typedef struct {
    float x;
    float y;
    Uint32 mb;
} dMouse;

void input_init(InputHandle* input, Window* window); //
dMouse input_mouse(InputHandle* input); //grab the any relative mouse movement

#endif // INPUT_H