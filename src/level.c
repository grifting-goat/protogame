#include "level.h"
#include <stdlib.h>
#include "stb_ds.h"
#include "client.h"
#include "server.h"

#include "states.h"
#include "packet.h"

#define EPSILON 0.0001f

#define SHOOT_MAX_RAY_LEN 300.0f
#define SHOOT_RAY_STEP 0.1f
#define SHOOT_TRACER_TIME 0.3f

static void level_process_shots(Level* level, float dt);

void check_for_respawn(Level* level);

bool check_dead(Entity* ent);


bool level_create(Level* level, uint32_t tick_rate) {
    if (!level) return false;
    
    level->tick_rate = tick_rate;
    level->perf_freq = SDL_GetPerformanceFrequency();
    level->last_time = SDL_GetPerformanceCounter();
    level->fps_time_accum = 0;
    level->frame_count = 0;
    level->initialized = true;

    level->model_count = 0;
    level->models = NULL;
    level->ent_map = NULL;
    level->player_map = NULL;
    level->server = false;
    level->server_ref = NULL;
    level->client_ref = NULL;

    level->level_spawn = (Vec3){0.0f, 1.0f, 0.0f};
    level->ground = ground_create(150, 150, 0, 1024, (1.0f/100.0f),  5.0f);

    physics_init(level);
    
    return true;
}

bool level_update(Level* level, float delta_time) {
    if (!level || !level->initialized) return false;

    level_process_shots(level, delta_time);

    

    if (level->server) {
        physics_world_update(level, delta_time);
        check_for_respawn(level);
    } else {
        physics_step(level, delta_time);
    }

    return true;
}

bool level_add_player(Level* level, uint64_t uqid, uint32_t server_id) {
    if (!level || !level->initialized) return false;

    Player new_player = player_create();
    new_player.unqid = uqid;
    new_player.server_id = server_id;

    if (!level->server && level->client_ref != NULL) {
        //new_player.entity.model = temp_create_sphere(32, 16, 1.0f);
        new_player.entity.model = temp_create_model("crower.obj", "sand.jpg", level->client_ref->model_cache);

        //new_player.entity.model.scale = (Vec3){0.3f, 0.5f, 0.5f};
        new_player.entity.model.offset = (Vec3){0.0f, -1.0f, 0.0f};

        /*
        new_player.entity.model.scale = (Vec3){0.6f, 0.7f, 0.5f};
        new_player.entity.model.offset = (Vec3){0.0f, -1.1f, 0.0f};
        */
    }

    hmput(level->player_map, server_id, new_player);

    return true;
}


bool level_add_ent_death(Level* level, uint32_t server_id) {
    if (!level || !level->initialized) return false;

    int idx = hmgeti(level->ent_map, server_id);
    int idy = hmgeti(level->player_map, server_id);
    if (idx != -1 && idy != -1) {
        level->ent_map[idx].value.velocity = level->player_map[idy].value.entity.velocity;
        level->ent_map[idx].value.position = level->player_map[idy].value.entity.position;
    } else if (idx == -1) {
        Entity ent = level->player_map[idy].value.entity;
        hmput(level->ent_map, server_id, ent);
    }

    return true;
}

bool level_add_model(Level* level, Model* model) {
    if (!level || !model || !level->initialized) {
        return false;
    }

    if (level->server) {
        return true;
    }

    size_t new_count = (size_t)level->model_count + 1;
    Model* resized = (Model*)realloc(level->models, new_count * sizeof(*level->models));
    if (!resized) {
        return false;
    }

    level->models = resized;
    level->models[level->model_count] = *model;
    level->model_count = (uint32_t)new_count;
    return true;
}


void level_process_shots(Level* level, float dt) {
    for (int i = 0; i < hmlen(level->player_map); i++) {
        Player* p = &level->player_map[i].value;
        Entity* ent = &level->player_map[i].value.entity;

    
        if (p->shoot_queued) {
            p->shoot_queued = false;

            if (level->client_ref != NULL) {
                Client* client = level->client_ref;
                Gun_stats* gun = &client->guns[p->gun_idx];
                


                if (gun->wait_time > 0.0f) {
                    continue;
                }


                gun->wait_time = gun->reload_time;
                
                Packet_shoot payload = {
                    PCKT_SHOOT,
                    p->server_id,
                    p->gun_idx,
                    gun->seed
                };

                ENetPacket* packet = enet_packet_create(
                    &payload,
                    sizeof(payload),
                    0
                );

                enet_peer_send(client->server_peer, 1, packet);

            }

            if (level->server_ref != NULL) {
                Server* server = level->server_ref;
                Vec3 dir = p->movement.cam_forward;
                Vec3 source = p->entity.position;
                vec3_add_inplace(&source, &p->eye_offset);

                Gun_stats gun = server->guns[p->gun_idx];
                uint32_t seed = server->guns[p->gun_idx].seed;
                uint8_t rays = 1;
                uint8_t spread = 0;
                gun_seed_read(seed, &rays, &spread);
                if (rays == 0) {
                    rays = 1;
                }
                if (vec3_mag_squared(&dir) > EPSILON) {
                    vec3_normalize_inplace(&dir);
                } else {
                    dir = (Vec3){0.0f, 0.0f, -1.0f};
                }

                Vec3 original_dir = dir;
                for (uint8_t i = 0; i < rays; i++) {
                    Vec3 shot_dir = gun_spread(seed, i, original_dir);
                    Vec3 ray = vec3_multiply(&shot_dir, gun.range);
                    Vec3 dest = vec3_add(&source, &ray);

                    if (gun.tracers) {
                        Packet_tracer tracer_payload = {
                            PCKT_TRACER,
                            p->server_id,
                            source,
                            dest,
                            SHOOT_TRACER_TIME
                        };

                        ENetPacket* tracer_packet = enet_packet_create(
                            &tracer_payload,
                            sizeof(tracer_payload),
                            0
                        );
                        enet_host_broadcast(server->e_server, 1, tracer_packet);
                    }

                    p->movement.cam_forward = shot_dir;
                    Player* hit = ray_check_player_collison(level, p, gun.range, SHOOT_RAY_STEP);

                    if (hit) {

                        hit->entity.health -= gun.damage;

                        if(p->gun_idx == 2) {hit->entity.health -= gun.damage * vec3_mag(&p->entity.velocity);}


                        Vec3 hit_dir = vec3_subtract(&hit->entity.position, &ent->position);
                        vec3_normalize_inplace(&hit_dir);
                        Vec3 knock = vec3_multiply(&hit_dir, gun.knockback);
                        knock.y = gun.knockback_y + ((knock.y > 0.0f) * knock.y);

                        if(p->gun_idx == 2) {knock = vec3_multiply(&knock, 1.0f + (vec3_mag(&p->entity.velocity) * 0.1f));}
              

                        Packet_server_auth_knockback payload = {
                            PCKT_SERVER_AUTH_KNOCK,
                            hit->server_id,
                            knock
                        };
                        ENetPacket* packet = enet_packet_create(
                            &payload,
                            sizeof(payload),
                            0
                        );
                        enet_host_broadcast(server->e_server, 1, packet);
                    }
                }

                p->movement.cam_forward = original_dir;
            }

            


        }

    }
}


bool check_dead(Entity* ent) {
    if (ent->health <= 0.0f) {
        set_state(ent, DEAD);
        return 1;
    }
    return 0;
}

void check_for_respawn(Level* level) {
    Server* server = level->server_ref;

    for (int i = 0; i < hmlen(level->player_map); i++) {
        Player* p = &level->player_map[i].value;
        Entity* ent = &level->player_map[i].value.entity;

        if (ent->health <= 0.0f || is_state(ent, DEAD)) {
            ent->position = level->level_spawn;
            ent->velocity = (Vec3){0.0f, 0.0f, 0.0f};
            ent->health = 100.0f;
            clear_state(ent, DEAD);

            Packet_server_auth_respawn payload = {
                PCKT_SERVER_AUTH_RESPAWN,
                p->server_id,
                ent->position,
                ent->velocity,
                ent->health
            };

            ENetPacket* packet = enet_packet_create(
                &payload,
                sizeof(payload),
                0
            );
            enet_host_broadcast(server->e_server, 1, packet);

        }

    }
}




void level_destroy(Level* level) {
    if (!level) return;

    free(level->models);
    level->models = NULL;
    level->model_count = 0;

    hmfree(level->ent_map);
    level->ent_map = NULL;

    hmfree(level->player_map);
    level->player_map = NULL;
}


