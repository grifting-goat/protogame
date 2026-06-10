#include "entity.h"


void entity_create_default(Entity *ent) {
    ent->position = (Vec3){0.0f,1.0f,0.0f};
    ent->velocity = (Vec3){0.0f,0.0f,0.0f};
    ent->acceleration = (Vec3){0.0f,0.0f,0.0f};
    ent->force = (Vec3){0.0f,0.0f,0.0f};
    ent->mass = 20.0f;
    ent->health = 100.0f;
    ent->radius = 1.0f;
    ent->timed = false;
    ent->life_time = 0.0f;
    ent->high_y = 0;
    ent->states = 0;


}

void entity_create(Entity *ent, Model *mdl, Vec3 pos) {
    ent->model = *mdl;
    ent->position = pos;
    ent->velocity = (Vec3){0.0f,0.0f,0.0f};
    ent->acceleration = (Vec3){0.0f,0.0f,0.0f};
    ent->force = (Vec3){0.0f,0.0f,0.0f};
    ent->mass = 20.0f;
    ent->timed = false;
    ent->life_time = 0.0f;

    ent->high_y = 0;
}

void entity_destroy(Entity *ent) {
    return;
}