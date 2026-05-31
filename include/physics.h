#ifndef PHYSICS_H
#define PHYSICS_H

#include <stdint.h>
#include "engine_math.h"



typedef struct Level Level;
typedef struct Entity Entity;
typedef struct Player Player;
typedef struct Server Server;
typedef struct Client Client;

#define MAX_TICK_DELAY 10


typedef struct {

    Vec3 gravity;

} PhysicsWorld;

void physics_init(Level* level);


void physics_step(Level* level, float dt);

void physics_step_player(Level* level, Player* p, float dt);

void physics_update_states(Level* level, Entity* ent);

Player* ray_check_player_collison(Level* level, Player* shooter, float max_ray_len, float step);


#endif //PHYSICS_H