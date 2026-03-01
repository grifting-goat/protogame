#include "entity.h"


void entity_create_default(Entity *ent) {
    ent->position = (Vec3){0.0f,1.0f,0.0f};
    ent->velocity = (Vec3){0.0f,0.0f,0.0f};
    ent->acceleration = (Vec3){0.0f,0.0f,0.0f};
    ent->force = (Vec3){0.0f,0.0f,0.0f};
    ent->mass = 20.0f;
    ent->on_ground = false;
    ent->locked = false;
    ent->relative_ent = NULL;

}

void entity_create(Entity *ent, Model *mdl, Vec3 pos) {
    ent->model = *mdl;
    ent->position = pos;
    ent->velocity = (Vec3){0.0f,0.0f,0.0f};
    ent->acceleration = (Vec3){0.0f,0.0f,0.0f};
    ent->force = (Vec3){0.0f,0.0f,0.0f};
    ent->mass = 20.0f;
    ent->relative_ent = NULL;
    ent->on_ground = false;
    ent->locked = false;
}

void entity_destroy(Entity *ent) {
    return;
}