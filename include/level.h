#ifndef LEVEL_H
#define LEVEL_H

#include <stdint.h>
#include <stdbool.h>
#include <SDL3/SDL.h>

#include "input.h"
#include "entity.h"
#include "player.h"
#include "camera.h"
#include "help.h"
#include "physics.h"

#include "ground.h"
#include "event.h"


typedef struct Server Server;
typedef struct Client Client;

typedef struct {
    uint32_t key;
    Player value;
} PlayerMapEntry;

typedef struct {
    uint32_t key;
    Entity value;
} EntityMapEntry;



typedef struct Level {
    bool server;
    bool initialized;

    PlayerMapEntry* player_map;
    EntityMapEntry* ent_map;

    PhysicsWorld physics;
    Ground ground;

    Vec3 level_spawn[8];

} Level;

//shared
bool level_create(Level* level);
void level_destroy(Level* level);

//server side
bool level_server_update(Level* level, Server* server, float delta_time); 

bool level_server_add_player(Level* level, Server* server, const uint64_t uqid, const uint32_t server_id);

//client side
bool level_client_update(Level* level, Client* client, float delta_time); //dont have to explicitly pass level but it doesnt hurt

bool level_client_add_player(Level* level, Client* client, Player* new_player, const uint32_t server_id);



#endif // LEVEL_H