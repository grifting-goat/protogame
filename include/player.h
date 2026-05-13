#ifndef PLAYER_H
#define PLAYER_H

#include "entity.h"

typedef struct {
    Entity entity;
    Vec3 eye_offset; //how high the camera is placed over the centroid

    uint64_t unqid;
    uint8_t server_id;

} Player;

Player player_create();

#endif // PLAYER_H