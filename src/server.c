#include "server.h"
#include "stb_ds.h"
#include "event.h"


const char* server_tag = "Server: ";


void server_enet_poll(Server* s);
bool server_enet_startup(Server* server);

bool server_startup(Server* server){
    if (!server) return false;

    Timing* t = &server->time;

    t->tick = 0;
    t->tick_rate = 128;
    t->tick_time = 1.0f / server->time.tick_rate;
    t->server_time = 0.0f;

    t->accumulator = 0.0f;
    t->max_accumulator = t->tick_time * MAX_TICK_DELAY;

    t->last_time = SDL_GetPerformanceCounter();
    t->perf_freq = SDL_GetPerformanceFrequency();

    t->fps_time_accum = 0;
    t->frame_count = 0;

    if (!level_create(&server->level)) return false;


    server->level.server = true;
    server->level.server_ref = server;


    if (!server_enet_startup(server)) return false;

    printf("%sserver started...\n", server_tag);
    return true;
}

bool server_run(Server* server) {
    if (!server) return false;

    //timing
    Timing* t = &server->time;
    
    uint64_t now = SDL_GetPerformanceCounter();
    uint64_t frame_ticks = now - t->last_time;
    float dt = (float)frame_ticks / (float)t->perf_freq;
    t->last_time = now;

    // if i want fps stats
    t->fps_time_accum += frame_ticks;
    t->frame_count++;
    if (t->fps_time_accum >= t->perf_freq) {
        double fps = (double)t->frame_count * (double)t->perf_freq / (double)t->fps_time_accum;
        //printf("%sPasses:%d\r", server_tag, t->frame_count);
        //fflush(stdout);
        t->fps_time_accum = 0;
        t->frame_count = 0;
    }

    t->server_time += dt;

    //dont accumulate forever if lagging
    t->accumulator += dt;
    if (t->accumulator > t->max_accumulator) {t->accumulator = t->max_accumulator;}

    //main tick loop
    while (t->accumulator >= t->tick_time) {

        level_update(&server->level, dt); // updates the positions // check collisions // advances timers

        t->accumulator -= t->tick_time;
        t->tick++;
    }
    
        
    server_enet_poll(server);


    return true;
}


bool server_enet_startup(Server* server) {
    if (enet_initialize () != 0) {
        printf("%sAn error occurred while initializing ENet.\n", server_tag);
        return false;
    }

    ENetAddress address = {0};
    server->address = address;

    address.host = ENET_HOST_ANY; /* Bind the server to the default localhost*/
    address.port = 7777; /* Bind the server to port 7777. */

    server->e_server = enet_host_create(&address, MAX_CLIENTS, 2, 0, 0);

    if (server->e_server == NULL) {
        printf("%sAn error occurred while trying to create an ENet server host.\n", server_tag);
        return false;
    }

    printf("%sStarted an enet server...\n", server_tag);
    return true;

}

void server_enet_poll(Server* s) {
    if (!s || !s->e_server) return;

    ENetEvent event;
    while (enet_host_service(s->e_server, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                {
                    char ip[64] = {0};
                    if (enet_address_get_host_ip(&event.peer->address, ip, sizeof(ip)) == 0) {
                        printf("%sClient connected: %s:%u\n", server_tag, ip, event.peer->address.port);
                    } else {
                        printf("%sClient connected: <unknown>:%u\n", server_tag, event.peer->address.port);
                    }

                    uint32_t assigned_id = (uint32_t)event.peer->incomingPeerID;
                    Packet_on_connect pack = {
                        .pckt_id = PCKT_SERVER_ID, 
                        .server_id = assigned_id,
                        .tick = server.
                    };

                    ENetPacket* packet = enet_packet_create(
                        &pack,
                        sizeof(pack),
                        ENET_PACKET_FLAG_RELIABLE
                    );
                    enet_peer_send(event.peer, 0, packet);

                    
                }
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                if (event.packet && event.packet->dataLength >= 1) {
                    const uint8_t* data = (const uint8_t*)event.packet->data;
                    const uint8_t packet_type = data[0];

                    if (packet_type == PCKT_CLIENT_ACK && event.packet->dataLength == sizeof(Packet_client_ack)) {
                        const Packet_client_ack* pos = (const Packet_client_ack*)event.packet->data;

                        for (int i = 0; i < hmlen(s->level.player_map); i++) {
                            Packet_player existing = {
                                PCKT_ADD_PLAYER,
                                s->level.player_map[i].key,
                                s->level.player_map[i].value.unqid
                            };
                            ENetPacket* existing_packet = enet_packet_create(
                                &existing,
                                sizeof(existing),
                                ENET_PACKET_FLAG_RELIABLE
                            );
                            if (existing_packet) {
                                enet_peer_send(event.peer, 0, existing_packet);
                            }
                        }

                        level_add_player(&s->level, pos->uqid, pos->server_id);

                        //send to all connected clients
                        Packet_player payload = {PCKT_ADD_PLAYER, pos->server_id, pos->uqid};
                        ENetPacket* packet = enet_packet_create(
                            &payload,
                            sizeof(payload),
                            ENET_PACKET_FLAG_RELIABLE
                        );
                        if (packet) {
                            enet_host_broadcast(s->e_server, 0, packet);
                        }
                    }

                    if (packet_type == PCKT_CLIENT_STATE && event.packet->dataLength == sizeof(Packet_state)) {
                        const Packet_state* pos = (const Packet_state*)event.packet->data;
                        int idx = hmgeti(s->level.player_map, pos->server_id);
                        /*if (idx != -1) {
                            s->level.player_map[idx].value.entity.position = pos->pos;
                            s->level.player_map[idx].value.entity.velocity= pos->vel;
                            s->level.player_map[idx].value.movement.cam_forward = pos->cam_dir;
                            Vec3 up = {0.0f, 1.0f, 0.0f};
                            Vec3 right = vec3_cross(&up, &pos->cam_dir);
                            if (vec3_mag_squared(&right) > 0.0f) {
                                vec3_normalize_inplace(&right);
                            } else {
                                right = (Vec3){1.0f, 0.0f, 0.0f};
                            }
                            s->level.player_map[idx].value.movement.cam_right = right;
                            //s->level.player_map[idx].value.entity.states = pos->state;
                            //s->level.player_map[idx].value.eye_offset = pos->cam_offset;
                        }*/
                    }

                    if (packet_type == PCKT_USERCMD && event.packet->dataLength == sizeof(Packet_usercmd)) {
                        const Packet_usercmd* ucmd = (const Packet_usercmd*)event.packet->data;
                        uint32_t player_id = (uint32_t)event.peer->incomingPeerID;
                        int idx = hmgeti(s->level.player_map, player_id);
                        if (idx != -1) {
                            Player* p = &s->level.player_map[idx].value;
                            p->movement.cam_forward = ucmd->cmd.angles;
                            p->movement.wish_dir = ucmd->cmd.wishdir;
                            Vec3 up = {0.0f, 1.0f, 0.0f};
                            Vec3 right = vec3_cross(&up, &ucmd->cmd.angles);
                            if (vec3_mag_squared(&right) > 0.0f) {
                                vec3_normalize_inplace(&right);
                            } else {
                                right = (Vec3){1.0f, 0.0f, 0.0f};
                            }
                            p->movement.cam_right = right;
                            p->gun_idx = ucmd->cmd.gun_idx;
                            p->movement.jump_queued = (ucmd->cmd.buttons & (1U << JUMP)) != 0;
                            if (ucmd->cmd.buttons & (1U << DASH)) {
                                sys_queueEvent(&p->event_bus, ucmd->cmd.tick, SE_KEY, DASH, 1, 0, NULL);
                            }
                            if (ucmd->cmd.buttons & (1U << SHOOT)) {
                                sys_queueEvent(&p->event_bus, ucmd->cmd.tick, SE_KEY, SHOOT, 1, 0, NULL);
                            }
                        }
                    }

                    if (packet_type == PCKT_SHOOT && event.packet->dataLength == sizeof(Packet_shoot)) {
                        const Packet_shoot* pos = (const Packet_shoot*)event.packet->data;
                        int idx = hmgeti(s->level.player_map, pos->server_id);
                        if (idx != -1) {
                            s->level.player_map[idx].value.shoot_queued = true;
                            s->level.player_map[idx].value.gun_idx = pos->gun_idx;
                            s->level.player_map[idx].value.guns.guns[pos->gun_idx].seed = pos->seed;
                        }   

                    }

                }
                enet_packet_destroy(event.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                {
                    uint32_t disconnected_id = (uint32_t)event.peer->incomingPeerID;
                    printf("%sClient disconnected (id=%u).\n", server_tag, disconnected_id);

                    int idx = hmgeti(s->level.player_map, disconnected_id);
                    uint64_t uqid = (idx != -1) ? s->level.player_map[idx].value.unqid : 0;

                    hmdel(s->level.player_map, disconnected_id);

                    Packet_player payload = {PCKT_REMOVE_PLAYER, disconnected_id, uqid};
                    ENetPacket* packet = enet_packet_create(
                        &payload,
                        sizeof(payload),
                        ENET_PACKET_FLAG_RELIABLE
                    );
                    if (packet) {
                        enet_host_broadcast(s->e_server, 0, packet);
                    }
                }
                break;

            default:
                break;
        }
    }
}

void server_close(Server* server) {
    if (!server) return;
    sound_shutdown(&server->sound);
    level_destroy(&server->level);

    enet_host_destroy(server->e_server);
    enet_deinitialize();

    printf("\n%sserver closed!\n", server_tag);
}

