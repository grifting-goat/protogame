#include "player.h"

Player player_create() {
    Player p;

    entity_create_default(&p.entity);
    p.eye_offset = (Vec3){0.0f,1.0f,0.0f}; //how high the camera is placed over the centroid
    p.entity.mass = 70.0f;    

    return p;
}