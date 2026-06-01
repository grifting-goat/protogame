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



void send_to_player(Server* server, uint32_t id, ENetPacket* pack);
static void level_update_timed_entities(Level* level, float dt);
void broadcast_sound(Server* server, uint32_t server_id, bool client_side, SoundID id, Vec3 pos, float vol, float rang);



bool level_create(Level* level) {
    if (!level) return false;
    
    level->initialized = true;
    level->ent_map = NULL;
    level->player_map = NULL;
    level->server = false;


    //generate spawn points
    srand(67);
    for (int i = 0; i < 8; i++) {
        level->level_spawn[i] = (Vec3){(float)((rand() % 150) - 75), 0.0f, (float)((rand() % 150) - 75)};
    }

    level->ground = ground_create(150, 150, 0, 1024, (1.0f/100.0f),  5.0f);

    physics_init(level);
    
    return true;
}

bool level_server_update(Level* level, Server* server, float delta_time) {
    if (!level || !level->initialized) return false;
    if (!server || !server->initialized) return false;


    level_update_timed_entities(level, delta_time);

    physics_step(level, delta_time);


    for (int i = 0; i < hmlen(level->player_map); i++) { //eventually client_t map?
        Player_dash* d = &level->player_map[i].value.dash;
        if (d->cast_wait_time > 0.0f) {
            d->cast_wait_time -= delta_time;
            if (d->cast_wait_time <= 0.0f) { d->cast_wait_time = 0.0f; }
        }
        if (d->recharge_wait_time > 0.0f && d->current_charges < d->max_charges) {
            d->recharge_wait_time -= delta_time;
        }
        if (d->recharge_wait_time <= 0.0f && d->current_charges < d->max_charges) {
            d->current_charges++;
            d->recharge_wait_time = d->recharge_wait + d->recharge_wait_time;
        }
        if (d->current_charges > d->max_charges) { d->current_charges = d->max_charges; }
    }

    return true;
}

bool level_client_update(Level* level, Client* client, float delta_time) {
    if (!level || !level->initialized) return false;
    if (!client) return false;

    level_update_timed_entities(level, delta_time);

    physics_step(level, delta_time);
 
    Player_dash* d = &client->player->dash;
    if (d->cast_wait_time > 0.0f) {
        d->cast_wait_time -= delta_time;
        if (d->cast_wait_time <= 0.0f) { d->cast_wait_time = 0.0f; }
    }
    if (d->recharge_wait_time > 0.0f && d->current_charges < d->max_charges) {
        d->recharge_wait_time -= delta_time;
    }
    if (d->recharge_wait_time <= 0.0f && d->current_charges < d->max_charges) {
        d->current_charges++;
        d->recharge_wait_time = d->recharge_wait + d->recharge_wait_time;
    }
    if (d->current_charges > d->max_charges) { d->current_charges = d->max_charges; }


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


bool level_client_add_player(Level* level, Client* client, Player* new_player, const uint32_t server_id) {
    if (!level || !level->initialized || !client || !new_player) return false;


    hmput(level->player_map, server_id, *new_player);

    int idx = hmgeti(level->player_map, server_id);
    if (idx != -1) {
        client->player = &level->player_map[idx].value;
        return true;
    }
    return false;
    
}



// move this player creation out of there, just pass a created player like client function does

bool level_server_add_player(Level* level, Server* server, const uint64_t uqid, const uint32_t server_id) {
    if (!level || !level->initialized || server) return false;

    Player new_player = player_create();
    new_player.unqid = uqid;
    new_player.server_id = server_id;

    hmput(level->player_map, server_id, new_player);

    return true;
}





void level_server_process_shots(Level* level, Server* server, float dt) {
    for (int i = 0; i < hmlen(level->player_map); i++) {
        Player* p = &level->player_map[i].value;
        Entity* ent = &level->player_map[i].value.entity;

        //some event
        /*

            //send sound

            SoundID sound_id = SOUND_BLUNDER;
            if (p->gun_idx == 1) { sound_id = SOUND_MUSKET; }
            if (p->gun_idx == 2) { sound_id = SOUND_MACE; }

            broadcast_sound(server, p->server_id, true, sound_id, p->entity.position, vol, rang);


            //tracers

            Vec3 dir = p->cam_forward;
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

                p->cam_forward = shot_dir;
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

            p->cam_forward = original_dir;
        }*/

            


    }
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

        Vec3 dir = p->cam_forward;
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

            p->cam_forward = shot_dir;
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

            p->cam_forward = original_dir;
        }
    }
    */



/*
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
*/

void level_destroy(Level* level) {
    if (!level) return;

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







