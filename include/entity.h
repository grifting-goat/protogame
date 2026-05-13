#ifndef ENTITY_H
#define ENTITY_H

#include "model.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdlib.h>

typedef struct Entity Entity; // Forward declaration

typedef struct Entity {
    Model model;
    Vec3 force; 
    Vec3 position;
    Vec3 velocity;
    Vec3 acceleration;
    float mass;

    float health;
} Entity;

void entity_create_default(Entity *ent);

void entity_create(Entity *ent, Model *mdl, Vec3 pos);

void entity_destroy(Entity *ent);


#endif // ENTITY_H