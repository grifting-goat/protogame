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

void level_handle_dash(Player* p);
void key_event(Level* level, int key, int pressed, Player* p);
void level_proccess_move(Player* p, actions action);
usercmd_t build_usercmd(Client* c);

void send_to_player(Server* server, uint32_t id, ENetPacket* pack);

static void level_process_shots(Level* level, float dt);
static void level_update_timed_entities(Level* level, float dt);

void broadcast_sound(Server* server, uint32_t server_id, bool client_side, SoundID id, Vec3 pos, float vol, float rang);

void check_for_respawn(Level* level);
void check_for_dead(Level* level);

void level_usercmd(Level* level);

bool check_dead(Entity* ent);

static void level_process_player_buses(Level* level);


bool level_create(Level* level) {
    if (!level) return false;
    
    level->initialized = true;

    level->model_count = 0;
    level->models = NULL;
    level->ent_map = NULL;
    level->player_map = NULL;
    level->server = false;
    level->server_ref = NULL;
    level->client_ref = NULL;

    level->event_bus = sys_createBus();


    srand(67);
    for (int i = 0; i < 8; i++) {
        level->level_spawn[i] = (Vec3){(float)((rand() % 150) - 75), 0.0f, (float)((rand() % 150) - 75)};
    }

    level->ground = ground_create(150, 150, 0, 1024, (1.0f/100.0f),  5.0f);

    physics_init(level);
    
    return true;
}

bool level_update(Level* level, float delta_time) {
    if (!level || !level->initialized) return false;

    level_proccess_events(level);

    level_process_shots(level, delta_time);
    level_update_timed_entities(level, delta_time);

    level->accumulator += delta_time;

    if (level->accumulator > level->max_accumulator) {level->accumulator = level->max_accumulator;}

    while (level->accumulator >= level->tick_time) {

        printf("tick: %d\r", level->tick);


        if (level->server) {
            level_process_player_buses(level);
            physics_step(level, level->tick_time);
            check_for_dead(level);
            check_for_respawn(level);
            for (int i = 0; i < hmlen(level->player_map); i++) {
                Player_dash* d = &level->player_map[i].value.dash;
                if (d->cast_wait_time > 0.0f) {
                    d->cast_wait_time -= level->tick_time;
                    if (d->cast_wait_time <= 0.0f) { d->cast_wait_time = 0.0f; }
                }
                if (d->recharge_wait_time > 0.0f && d->current_charges < d->max_charges) {
                    d->recharge_wait_time -= level->tick_time;
                }
                if (d->recharge_wait_time <= 0.0f && d->current_charges < d->max_charges) {
                    d->current_charges++;
                    d->recharge_wait_time = d->recharge_wait + d->recharge_wait_time;
                }
                if (d->current_charges > d->max_charges) { d->current_charges = d->max_charges; }
            }
        } else if (level->client_ref != NULL) {
            level_usercmd(level);
            physics_step(level, level->tick_time);
        }
        
        level->accumulator -= level->tick_time;
        level->tick++;
    }

    return true;
}

static void level_update_timed_entities(Level* level, float dt) {
    if (!level || dt <= 0.0f) {
        return;
    }

    for (int i = 0; i < hmlen(level->ent_map);) {
        Entity* ent = &level->ent_map[i].value;
        if (!ent->timed) {
            i++;
            continue;
        }

        ent->life_time -= dt;
        if (ent->life_time <= 0.0f) {
            uint32_t key = level->ent_map[i].key;
            hmdel(level->ent_map, key);
            continue;
        }

        i++;
    }


    for (int i = 0; i < hmlen(level->player_map);) {
        Entity* ent = &level->player_map[i].value.entity;
        if (!ent->timed) {
            i++;
            continue;
        }

        ent->life_time -= dt;
        i++;
    }
}

static void level_process_player_buses(Level* level) {
    for (int i = 0; i < hmlen(level->player_map); i++) {
        Player* p = &level->player_map[i].value;
        sysEvent_t event;
        while (true) {
            event = sys_popEvent(&p->event_bus);
            if (event.eventType == SE_NONE) break;
            if (event.eventType == SE_KEY) {
                key_event(level, event.value, event.value2, p);
            }
            if (event.ptr) { free(event.ptr); }
        }
    }
}

void level_usercmd(Level* level) {
    Client* client = level->client_ref;

    if (client->player) {
        usercmd_t cmd = build_usercmd(client);

        client->input_buffer[client->input_buffer_idx % INPUT_BUFFER_SIZE] = cmd;
        client->input_buffer_idx++;

        Packet_usercmd payload = {
            PCKT_USERCMD,
            cmd
        };
        ENetPacket* packet = enet_packet_create(
            &payload,
            sizeof(payload),
            0
        );
        enet_peer_send(client->server_peer, 1, packet);

        client->actions = 0;
    }
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

        /*
            if (level->client_ref != NULL) {
                Client* client = level->client_ref;
                Gun_stats* gun = &p->guns.guns[p->gun_idx];
                


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

            }*/

            if (level->server_ref != NULL) {
                Server* server = level->server_ref;

                //send sound

                SoundID sound_id = SOUND_BLUNDER;
                if (p->gun_idx == 1) { sound_id = SOUND_MUSKET; }
                if (p->gun_idx == 2) { sound_id = SOUND_MACE; }

                broadcast_sound(server, p->server_id, true, sound_id, p->entity.position, vol, rang);


                //tracers

                Vec3 dir = p->movement.cam_forward;
                Vec3 source = p->entity.position;
                vec3_add_inplace(&source, &p->eye_offset);

                Gun_stats gun = p->guns.guns[p->gun_idx];
            uint32_t seed = p->guns.guns[p->gun_idx].seed;
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
                bool any_hit = false;
                bool any_kill = false;
                bool any_mace_smash = false;
                Vec3 impact_sound_pos = p->entity.position;
                for (uint8_t i = 0; i < rays; i++) {
                    Vec3 shot_dir = gun_spread(seed, i, original_dir);
                    Vec3 ray = vec3_multiply(&shot_dir, gun.range);
                    Vec3 dest = vec3_add(&source, &ray);

                    if (gun.tracers) {
                        Packet_add_tracer tracer_payload = {
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

                    //hit logic

                    p->movement.cam_forward = shot_dir;
                    Player* hit = ray_check_player_collison(level, p, gun.range, SHOOT_RAY_STEP);

                    if (hit) {
                        float hit_start_hp = hit->entity.health;
                        float fall_dist = ent->high_y - ent->position.y;
                        if (fall_dist < 0.0f) {fall_dist = 0.0f;}

                        float low_mul = (fall_dist >= 6.0f) ? (6.0f * 0.4f) : fall_dist * 0.4f;
                        float high_mul = (fall_dist >= 6.0f) ? (fall_dist - 6.0f) * 0.15f : 0;

                        hit->entity.health -= gun.damage;

                        if (!any_hit) {
                            impact_sound_pos = hit->entity.position;
                        }
                        any_hit = true;
                        if (hit_start_hp > 0.0f && hit->entity.health <= 0.0f) {
                            any_kill = true;
                        }

                        if (p->gun_idx == 2) {
                            hit->entity.health -= gun.damage * low_mul;
                            hit->entity.health -= gun.damage * high_mul;
                            if (high_mul > 0.0f) {
                                any_mace_smash = true;
                            }

                        }

                        float hit_dam = hit_start_hp - hit->entity.health;

                        Vec3 hit_dir = vec3_subtract(&hit->entity.position, &ent->position);
                        vec3_normalize_inplace(&hit_dir);
                        Vec3 knock = vec3_multiply(&hit_dir, gun.knockback);
                        knock.y = gun.knockback_y + ((knock.y > 0.0f) * knock.y);

                        if (p->gun_idx == 2) {
                            knock = vec3_multiply(&knock, 1.0f + fall_dist * 0.3f);
                        }

                        vec3_add_inplace(&hit->entity.velocity, &knock);
                        hit->entity.position.y += 0.01f;

                        if (p->gun_idx == 2 && hit_dam > 40.0f) {
                            p->entity.velocity.y = hit_dam / 7.5f;
                        }

                        Packet_hit_verify payload_v = {
                            PCKT_HIT_VERIFY,
                            p->server_id,
                            hit->server_id,
                            p->gun_idx,
                            hit_dam,
                            (hit->entity.health <= 0.0f)
                        };

                        ENetPacket* packet_v = enet_packet_create(
                            &payload_v,
                            sizeof(payload_v),
                            0
                        );

                        send_to_player(server, p->server_id, packet_v);
                    }
                }

                if (any_hit) {
                    SoundID s_id;
                    float s_vol = 0.6f;
                    float s_range = 20.0f;

                    if (p->gun_idx == 2) {
                        s_id = any_mace_smash ? SOUND_MACE_SMASH : SOUND_MACE_HIT;
                        s_vol = 1.1f;
                        s_range = 100.0f;
                    } else if (any_kill) {
                        s_vol = 0.25f;
                        s_range = 50.0f;
                        s_id = (rand() % 4) ? SOUND_DEATH : SOUND_OOF;
                    } else {
                        s_vol = 0.7f;
                        s_range = 12.0f;
                        s_id = SOUND_HURT;
                    }

                    broadcast_sound(server, p->server_id, false, s_id, impact_sound_pos, s_vol, s_range);

                    // For mace kill shots, emit both impact and death vocal sounds.
                    if (p->gun_idx == 2 && any_kill) {
                        SoundID death_id = (rand() % 3) ? SOUND_DEATH : SOUND_OOF;
                        broadcast_sound(server, p->server_id, false, death_id, impact_sound_pos, 0.3f, 50.0f);
                    }
                }

                p->movement.cam_forward = original_dir;
            }

            


        }

    }
}

void level_process_shot(Level* level, Player* p) {
    Entity* ent = &p->entity;

    if (level->client_ref != NULL) {
        Client* client = level->client_ref;
        Gun_stats* gun = &p->guns.guns[p->gun_idx];
        
        if (gun->wait_time > 0.0f) {
            return;
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

    /*

    if (level->server_ref != NULL) {
        Server* server = level->server_ref;

        //send sound

        int sound_idx = 0;
        float vol = 0.9f;
        float rang = 100.0f;
        if (p->gun_idx == 0) {sound_idx = sound_find(&server->sound, "blunder");}
        if (p->gun_idx == 1) {sound_idx = sound_find(&server->sound, "musket");}
        if (p->gun_idx == 2) {sound_idx = sound_find(&server->sound, "mace");}

        if (sound_idx >= 0) {
            broadcast_sound(server, p->server_id, true, sound_idx, p->entity.position, vol, rang);
        }


        //tracers

        Vec3 dir = p->movement.cam_forward;
        Vec3 source = p->entity.position;
        vec3_add_inplace(&source, &p->eye_offset);

        Gun_stats gun = p->guns.guns[p->gun_idx];
        uint32_t seed = p->guns.guns[p->gun_idx].seed;
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
        bool any_hit = false;
        bool any_kill = false;
        bool any_mace_smash = false;
        Vec3 impact_sound_pos = p->entity.position;
        for (uint8_t i = 0; i < rays; i++) {
            Vec3 shot_dir = gun_spread(seed, i, original_dir);
            Vec3 ray = vec3_multiply(&shot_dir, gun.range);
            Vec3 dest = vec3_add(&source, &ray);

            if (gun.tracers) {
                Packet_add_tracer tracer_payload = {
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

            //hit logic

            p->movement.cam_forward = shot_dir;
            Player* hit = ray_check_player_collison(level, p, gun.range, SHOOT_RAY_STEP);

            if (hit) {
                float hit_start_hp = hit->entity.health;
                float fall_dist = ent->high_y - ent->position.y;
                if (fall_dist < 0.0f) {fall_dist = 0.0f;}

                float low_mul = (fall_dist >= 6.0f) ? (6.0f * 0.4f) : fall_dist * 0.4f;
                float high_mul = (fall_dist >= 6.0f) ? (fall_dist - 6.0f) * 0.15f : 0;

                hit->entity.health -= gun.damage;

                if (!any_hit) {
                    impact_sound_pos = hit->entity.position;
                }
                any_hit = true;
                if (hit_start_hp > 0.0f && hit->entity.health <= 0.0f) {
                    any_kill = true;
                }

                if (p->gun_idx == 2) {
                    hit->entity.health -= gun.damage * low_mul;
                    hit->entity.health -= gun.damage * high_mul;
                    if (high_mul > 0.0f) {
                        any_mace_smash = true;
                    }

                }

                float hit_dam = hit_start_hp - hit->entity.health;

                Vec3 hit_dir = vec3_subtract(&hit->entity.position, &ent->position);
                vec3_normalize_inplace(&hit_dir);
                Vec3 knock = vec3_multiply(&hit_dir, gun.knockback);
                knock.y = gun.knockback_y + ((knock.y > 0.0f) * knock.y);

                if (p->gun_idx == 2) {
                    knock = vec3_multiply(&knock, 1.0f + fall_dist * 0.3f);
                }

                vec3_add_inplace(&hit->entity.velocity, &knock);
                hit->entity.position.y += 0.01f;

                if (p->gun_idx == 2 && hit_dam > 40.0f) {
                    p->entity.velocity.y = hit_dam / 7.5f;
                }

                Packet_hit_verify payload_v = {
                    PCKT_HIT_VERIFY,
                    p->server_id,
                    hit->server_id,
                    p->gun_idx,
                    hit_dam,
                    (hit->entity.health <= 0.0f)
                };

                ENetPacket* packet_v = enet_packet_create(
                    &payload_v,
                    sizeof(payload_v),
                    0
                );

                send_to_player(server, p->server_id, packet_v);
            }
        }

            if (any_hit) {
                SoundID s_id;
                float s_vol = 0.6f;
                float s_range = 20.0f;

                if (p->gun_idx == 2) {
                    s_id = any_mace_smash ? SOUND_MACE_SMASH : SOUND_MACE_HIT;
                    s_vol = 1.1f;
                    s_range = 100.0f;
                } else if (any_kill) {
                    s_vol = 0.25f;
                    s_range = 50.0f;
                    s_id = (rand() % 4) ? SOUND_DEATH : SOUND_OOF;
                } else {
                    s_vol = 0.7f;
                    s_range = 12.0f;
                    s_id = SOUND_HURT;
                }

                broadcast_sound(server, p->server_id, false, s_id, impact_sound_pos, s_vol, s_range);

                // For mace kill shots, emit both impact and death vocal sounds.
                if (p->gun_idx == 2 && any_kill) {
                    SoundID death_id = (rand() % 3) ? SOUND_DEATH : SOUND_OOF;
                    broadcast_sound(server, p->server_id, false, death_id, impact_sound_pos, 0.3f, 50.0f);
                }
            }

            p->movement.cam_forward = original_dir;
        }
    }
    */
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

        // Only respawn if entity is timing and timer expired
        if (ent->timed && ent->life_time <= 0.0f) {
            ent->position = level->level_spawn[rand() % 8];
            ent->velocity = (Vec3){0.0f, 0.0f, 0.0f};
            ent->health = 100.0f;
            clear_state(ent, DEAD);
            ent->life_time = 0.0f;
            ent->timed = false;

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


void check_for_dead(Level* level) {
    Server* server = level->server_ref;

    for (int i = 0; i < hmlen(level->player_map); i++) {
        Player* p = &level->player_map[i].value;
        Entity* ent = &level->player_map[i].value.entity;

        // If player is dead and not already timing, send death packet and start timer
        if ((ent->health <= 0.0f || is_state(ent, DEAD)) && !ent->timed) {
            set_state(ent, DEAD);
            ent->timed = true;
            ent->life_time = 3.0f;

            Packet_server_auth_dead payload = {
                PCKT_SERVER_AUTH_DEAD,
                p->server_id
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


void send_to_player(Server* server, uint32_t id, ENetPacket* pack) {
    if (pack) {
    const size_t peer_idx = (size_t)id;
        if (server->e_server && peer_idx < (size_t)server->e_server->peerCount) {
            ENetPeer* target_peer = &server->e_server->peers[peer_idx];
            if (target_peer->state == ENET_PEER_STATE_CONNECTED) {
                enet_peer_send(target_peer, 1, pack);
            } else {
                enet_packet_destroy(pack);
            }
        } else {
            enet_packet_destroy(pack);
        }
    }
}

void broadcast_sound(Server* server, uint32_t server_id, bool client_side, SoundID id, Vec3 pos, float vol, float rang) {
    Packet_add_sound sound_payload = {
        PCKT_ADD_SOUND,
        server_id,
        client_side,
        pos,
        id,
        vol,
        rang
    };

    ENetPacket* sound_packet = enet_packet_create(
        &sound_payload,
        sizeof(sound_payload),
        0
    );
    enet_host_broadcast(server->e_server, 1, sound_packet);
}


void level_proccess_events(Level* level) {
    sysEventBus* bus = &level->event_bus;

    sysEvent_t	event;
    while(true) {
        event = sys_popEvent(bus);

        

        if (event.eventType == SE_NONE) {
            return;
        }
        printf("event: %d ", event.eventType);

        Player* p = NULL;
        if (level->server_ref != NULL) {
            int idx = hmgeti(level->player_map, (uint32_t)event.ptrLength);
            if (idx != -1) p = &level->player_map[idx].value;
        } else if (level->client_ref != NULL) {
            p = level->client_ref->player;
        }

        switch (event.eventType) {

        case SE_NONE:
            break;
		case SE_KEY:
			key_event(level, event.value, event.value2, p);
			break;
		case SE_CHAR:
			break;
		case SE_MOUSE:
			break;
		case SE_CONSOLE:

			break;
		case SE_PACKET:
            break;
        default:
            printf("bad event\n");
        }

        if (event.ptr) {
			free(event.ptr);
		}

    }
}

void key_event(Level* level, int key, int pressed, Player* p) {

    if (!p || !pressed) { return; }

    switch ((actions)key) {
        case DASH:
            level_handle_dash(p);
            break;
        case SHOOT:
            level_process_shot(level, p);
            break;
        case FORWARD:
            level_proccess_move(p, FORWARD);
            break;
        case BACKWARD:
            level_proccess_move(p, BACKWARD);
            break;
        case RIGHT:
            level_proccess_move(p, RIGHT);
            break;
        case LEFT:
            level_proccess_move(p, LEFT);
            break;
        default:
            break;
    }

}


void level_handle_dash(Player* p) {

    if (p->dash.cast_wait_time > 0.0f) {p->movement.dash_queued = false; return;}
    if (p->dash.current_charges <= 0) {p->movement.dash_queued = false; return;}

    p->dash.current_charges--;
    p->dash.cast_wait_time = p->dash.cast_wait;

    //sound_play_name(&c->sound, "dash", 0.2f);

    p->entity.position.y += 0.01f;
    p->movement.jump_queued = true;
    Vec3 wishdir = p->movement.cam_forward;
    wishdir = vec3_normalize(&wishdir);
    Vec3 wishflat = wishdir;
    wishflat.y = 0.0f;

    float wishspeed = p->dash.dash_vel;
    float currentspeed = vec3_dot(&p->entity.velocity, &wishdir);
    float addspeed = (currentspeed < 0.0f) ? (wishspeed - currentspeed) : wishspeed;


    Vec3 accel = vec3_multiply(&wishdir, addspeed);
    vec3_add_inplace(&p->entity.velocity, &accel);

}

usercmd_t build_usercmd(Client* c) {
    usercmd_t cmd;

    cmd.tick = c->level.tick;
    cmd.angles = camera_forward(&c->player_camera);
    cmd.buttons = c->actions;
    cmd.gun_idx = c->player->gun_idx;
    cmd.wishdir = c->player->movement.wish_dir;

    return cmd;

}



void level_proccess_move(Player* p, actions action) {

    switch (action) {
        case FORWARD:
            p->movement.wish_dir.z += 1.0f;
            break;
        case BACKWARD:
            p->movement.wish_dir.z -= 1.0f;
            break;
        case RIGHT:
            p->movement.wish_dir.x -= 1.0f;
            break;
        case LEFT:
            p->movement.wish_dir.x += 1.0f;
            break;
        default:
            break;
    }
    
}