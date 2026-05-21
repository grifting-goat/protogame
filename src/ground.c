#include "ground.h"

/*
void ground_init(Ground* ground, uint32_t x, uint32_t y, uint32_t min, uint32_t max) {
    return ground_create(x, y, min, max);
}*/

#define SEED 0x67


void ground_generate_noise(Ground* ground, int seed);
uint32_t** create_empty_map(uint32_t r_size, uint32_t c_size);
uint32_t** convolve(uint32_t** map, uint32_t r_size, uint32_t c_size);
void dealloc_map(uint32_t** map, uint32_t r_size);


Ground ground_create(uint32_t x, uint32_t z, uint32_t min, uint32_t max, float y_scale, float xz_scale) {
    Ground ground = {NULL, x, z, max, min, y_scale, xz_scale};

    if (min >= max) {printf("ground creation failed, conditon (min >= max) \n"); return ground;}

    ground.height_map =  create_empty_map(ground.x_size, ground.z_size);

    ground_generate_noise(&ground, SEED);

    if (ground.height_map == NULL) {printf("ground creation failed\n");}

    return ground;
}


void ground_generate_noise(Ground* ground, int seed) {

    if (ground->height_map == NULL) {printf("cannot generate noise on null height map\n"); return;}
    srand(seed);

    for (int x = 0; x < ground->x_size; x++) {
        for (int z = 0; z < ground->z_size; z++)  {
            ground->height_map[x][z] = ((uint32_t)rand() % (ground->max_y - ground->min_y)) +  ground->min_y;
        }
    }

    




    ground->height_map = convolve(ground->height_map, ground->x_size ,ground->z_size);
    ground->height_map = convolve(ground->height_map, ground->x_size ,ground->z_size);

    for (int x = 70; x < 90; x++) {
            ground->height_map[x][59] = ground->max_y * 1.0f;
            ground->height_map[x][60] = ground->max_y * 1.6f;
            ground->height_map[x][61] = ground->max_y * 1.6f;
            ground->height_map[x][62] = ground->max_y * 1.0f;
    }
    ground->height_map[69][62] = ground->max_y * 0.5f;
    ground->height_map[69][61] = ground->max_y * 0.5f;
    ground->height_map[69][60] = ground->max_y * 0.5f;
    ground->height_map[69][59] = ground->max_y * 0.5f;


    ground->height_map[90][62] = ground->max_y * 0.5f;
    ground->height_map[90][61] = ground->max_y * 0.5f;
    ground->height_map[90][60] = ground->max_y * 0.5f;
    ground->height_map[90][59] = ground->max_y * 0.5f;



}


uint32_t** convolve(uint32_t** map, uint32_t r_size, uint32_t c_size) {
    if(map == NULL) {printf("cannot convolve null map\n"); return NULL;}
    if(r_size < 3 || c_size < 3) {printf("bounds to small to convolve\n"); return map;}
    uint32_t** new_map = create_empty_map(r_size, c_size);
    if(new_map == NULL) {printf("cannot create new map for convolution\n"); return map;}
    //fix padding at some point

    for (int r = 1; r < r_size - 1; r++) {
        for (int c = 1; c < c_size - 1; c++) {
            //3x3 kernal
            uint32_t sum = 0;
            sum += map[r][c] * 1.2f; //more weight on current
            sum += map[r+1][c];
            sum += map[r+-1][c];
            sum += map[r][c-1];
            sum += map[r+1][c-1];
            sum += map[r+-1][c-1];
            sum += map[r][c+1];
            sum += map[r+1][c+1];
            sum += map[r+-1][c+1];

            sum /= 9;

            new_map[r][c] = sum;
        }
    }

    dealloc_map(map, r_size);

    return new_map;
}



uint32_t** create_empty_map(uint32_t r_size, uint32_t c_size) {

    uint32_t** new_map;
    new_map = calloc((r_size), sizeof(uint32_t*));
    if(new_map == NULL) {printf("failed to allocate new map\n"); new_map = NULL; return new_map;}

    for(int i = 0; i < r_size; i++) {
        new_map[i] = calloc((c_size), sizeof(uint32_t));
        if(new_map[i] == NULL) {
            printf("failed to allocate hieght map\n"); 
            for(int k = 0; k < i; k++) {free(new_map[k]); new_map[k] = NULL;}
            free(new_map); 
            new_map = NULL; 
            return new_map;
        }
    }

    return new_map;
}


void ground_destroy(Ground* ground) {

    dealloc_map(ground->height_map, ground->x_size);
    ground->height_map = NULL;

}

void dealloc_map(uint32_t** map, uint32_t r_size) {

    if(map == NULL) {return;}

    for(int r = 0; r < r_size; r++) {
        free(map[r]);
        map[r] = NULL;
    }

    free(map);

}