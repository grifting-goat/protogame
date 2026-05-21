#ifndef GROUND_H
#define GROUND_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


typedef struct {

    uint32_t** height_map;

    uint32_t x_size;
    uint32_t z_size;

    uint32_t max_y;
    uint32_t min_y;

    float y_scale;
    float xz_scale;


} Ground;


//void ground_init(Ground* ground, uint32_t x, uint32_t y, uint32_t min, uint32_t max);


Ground ground_create(uint32_t x, uint32_t z, uint32_t min, uint32_t max, float y_scale, float xz_scale);

void ground_destroy(Ground* ground);

#endif //GROUND_H