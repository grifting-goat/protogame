#include "server.h"
#include "stb_ds.h"

const char* server_tag = "Server: ";


void server_enet_poll(Server* s);
bool server_enet_startup(Server* server);

bool server_startup(Server* server){
    if (!server) return false;
    if (!level_create(&server->level, 128)) return false;
    server->level.server = true;
    server->level.server_ref = server;

    if (!server_enet_startup(server)) return false;


    printf("%sserver started...\n", server_tag);
    return true;
}

bool server_run(Server* server) {
    if (!server) return false;

    static Uint64 send_time_accum = 0;

    //timing
    Uint64 now = SDL_GetPerformanceCounter();
    Uint64 frame_ticks = now - server->level.last_time;
    float dt = (float)frame_ticks / (float)server->level.perf_freq;
    server->level.last_time = now;

    
    server->level.fps_time_accum += frame_ticks;
    server->level.frame_count++;
        if (server->level.fps_time_accum >= server->level.perf_freq) {
            double fps = (double)server->level.frame_count * (double)server->level.perf_freq / (double)server->level.fps_time_accum;
            printf("%sPasses:%d\r", server_tag, server->level.frame_count);
            fflush(stdout);
            server->level.fps_time_accum = 0;
            server->level.frame_count = 0;
        }
    
    server_enet_poll(server);
    level_update(&server->level, dt);
    

    send_time_accum += frame_ticks;
    const Uint64 send_interval = server->level.perf_freq / server->level.tick_rate;
    while (send_time_accum >= send_interval) {

        for (int i = 0; i < hmlen(server->level.player_map); i++) {
            Packet_state payload = {
                PCKT_SERVER_STATE,
                server->level.player_map[i].key,
                server->level.player_map[i].value.entity.position,
                server->level.player_map[i].value.entity.velocity,
                server->level.player_map[i].value.movement.cam_dir,
                server->level.player_map[i].value.entity.states,
                server->level.player_map[i].value.entity.health
            };
            ENetPacket* packet = enet_packet_create(
                &payload,
                sizeof(payload),
                0
            );
            enet_host_broadcast(server->e_server, 1, packet);
        }

        send_time_accum -= send_interval;
    }

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
                    Packet_server_id pack = {PCKT_SERVER_ID, assigned_id};
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
                        if (idx != -1) {
                            s->level.player_map[idx].value.entity.position = pos->pos;
                            s->level.player_map[idx].value.entity.velocity= pos->vel;
                            s->level.player_map[idx].value.movement.cam_dir = pos->cam_dir;
                            s->level.player_map[idx].value.entity.states = pos->state;
                        }
                    }

                    if (packet_type == PCKT_SHOOT && event.packet->dataLength == sizeof(Packet_shoot)) {
                        const Packet_shoot* pos = (const Packet_shoot*)event.packet->data;
                        int idx = hmgeti(s->level.player_map, pos->server_id);
                        if (idx != -1) {
                            s->level.player_map[idx].value.shoot_queued = true;
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
    level_destroy(&server->level);

    enet_host_destroy(server->e_server);
    enet_deinitialize();

    printf("\n%sserver closed!\n", server_tag);
}

