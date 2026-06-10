#include "server.h"
#include "stb_ds.h"

//defined here so no one else can call them

const char* server_log_tag = "Server Log: ";
const char* server_err_tag = "Server Error: ";

#define SERVER_LOG(fmt, ...) printf("%s" fmt, server_log_tag, ##__VA_ARGS__)
#define SERVER_ERR(fmt, ...) fprintf(stderr, "%s" fmt, server_err_tag, ##__VA_ARGS__)


void server_add_client(Server* server, uint32_t server_id, uint32_t uqid, char* name);
static void server_apply_client_inputs(Server* s);


void server_enet_poll(Server* s);
bool server_enet_startup(Server* server, uint16_t port);
void server_disconnect_peers(Server* server);
void server_send_auth_states(Server* server);

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

    server->port = 7777;
    if (!server_enet_startup(server, server->port)) return false;

    server->initialized = true;
    server->clients_connected = 0;

    SERVER_LOG("Server started successfully!");
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
    server_enet_poll(server);

    while (t->accumulator >= t->tick_time) {

        server_apply_client_inputs(server);
        level_server_update(&server->level, server, t->tick_time); // updates the positions // check collisions // advances timers


        server_send_auth_states(server);
        t->accumulator -= t->tick_time;
        t->tick++;

    }
    
        
    return true;
}


bool server_enet_startup(Server* server, uint16_t port) {
    if (enet_initialize () != 0) {
        SERVER_ERR("An error occurred while initializing ENet.\n");
        return false;
    }

    ENetAddress address = {0};
    server->address = address;

    address.host = ENET_HOST_ANY; // Bind the server to the default localhost
    address.port = port; // Bind the server to port -> default: 7777.

    server->e_server = enet_host_create(&address, MAX_CLIENTS, 2, 0, 0);

    if (server->e_server == NULL) {
        SERVER_ERR("An error occurred while trying to create an ENet server host.\n");
        return false;
    }

    SERVER_LOG("Started an enet server...\n");
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
                        SERVER_LOG("Client connected: %s:%u\n", ip, event.peer->address.port);
                    } else {
                        SERVER_LOG("Client connected: <unknown>:%u\n", event.peer->address.port);
                    }

                    uint32_t assigned_id = (uint32_t)event.peer->incomingPeerID;

                    
                    Packet_on_connect pack = {
                        .pckt_id = (uint8_t)ON_CONNECT,
                        .server_id = assigned_id,
                        .tick = s->time.tick,
                        .server_time = s->time.server_time
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

                    if (packet_type == (uint8_t)ADD_PLAYER) {
                        const Packet_add_player* pack = (const Packet_add_player*)event.packet->data;
                        server_add_client(s, pack->server_id, pack->unique_id, NULL);
                    }

                    if (packet_type == (uint8_t)REMOVE_PLAYER) {
                        // Packet_remove_player handler.
                    }

                    if (packet_type == (uint8_t)SHOOT_PACKET) {
                        // Packet_shoot handler.
                        /*
                        Relevant previous implementation:
                        const Packet_shoot* pos = (const Packet_shoot*)event.packet->data;
                        int idx = hmgeti(s->level.player_map, pos->server_id);
                        if (idx != -1) {
                            s->level.player_map[idx].value.shoot_queued = true;
                            s->level.player_map[idx].value.gun_idx = pos->gun_idx;
                            s->level.player_map[idx].value.guns.guns[pos->gun_idx].seed = pos->seed;
                        }
                        */
                    }

                    if (packet_type == (uint8_t)ADD_TRACER) {
                        // Packet_add_tracer handler.
                    }

                    if (packet_type == (uint8_t)RESPAWN) {
                        // Packet_respawn handler.
                    }

                    if (packet_type == (uint8_t)DEAD_PACKET) {
                        // Packet_dead handler.
                    }

                    if (packet_type == (uint8_t)HIT_VERIFY) {
                        // Packet_hit_verify handler.
                    }

                    if (packet_type == (uint8_t)ADD_SOUND) {
                        // Packet_add_sound handler.
                    }

                    if (packet_type == (uint8_t)AUTH_STATE) {
                        // Packet_auth_state handler.
                    }

                    if (packet_type == (uint8_t)AUTH_FULL_SYNC) {
                        // Packet_auth_full_sync handler.
                    }

                    if (packet_type == (uint8_t)USERCMD && event.packet->dataLength == sizeof(Packet_usercmd)) {
                        const Packet_usercmd* ucmd = (const Packet_usercmd*)event.packet->data;
                        uint32_t peer_id = (uint32_t)event.peer->incomingPeerID;

                        for (uint32_t i = 0; i < s->clients_connected; i++) {
                            if (s->clients[i].peer_id == peer_id) {
                                s->clients[i].latest_cmd = ucmd->cmd;
                                break;
                            }
                        }
                    }

                }
                enet_packet_destroy(event.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT: {
                    uint32_t disconnected_id = (uint32_t)event.peer->incomingPeerID;
                    SERVER_LOG("Client disconnected (id=%u).\n", disconnected_id);

                    int idx = hmgeti(s->level.player_map, disconnected_id);
                    uint64_t uqid = (idx != -1) ? s->level.player_map[idx].value.unqid : 0;

                    hmdel(s->level.player_map, disconnected_id);

                    (void)uqid;
                    Packet_remove_player payload = {(uint8_t)REMOVE_PLAYER, disconnected_id};
                    ENetPacket* packet = enet_packet_create(
                        &payload,
                        sizeof(payload),
                        ENET_PACKET_FLAG_RELIABLE
                    );

                    if (packet) {enet_host_broadcast(s->e_server, 0, packet);}
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

    server_disconnect_peers(server);
    enet_host_destroy(server->e_server);
    enet_deinitialize();

    SERVER_LOG("server closed\n");
}


//only the server can do these

void server_send_auth_states(Server* server) {
    if (!server) { return; }

    for (uint32_t i = 0; i < server->clients_connected; i++) {
        client_t* ct = &server->clients[i];

        uint32_t peer_id = (uint32_t)ct->peer_id;
        int idx = hmgeti(server->level.player_map, peer_id);
        if (idx == -1) { continue; }

        const Player* p = &server->level.player_map[idx].value;

        userstate_t state = get_userstate(p, server->time.tick);
        Packet_auth_state payload = {
            .pckt_id = (uint8_t)AUTH_STATE,
            .state   = state,
        };

        ENetPeer* peer = &server->e_server->peers[peer_id];
        if (peer->state != ENET_PEER_STATE_CONNECTED) { continue; }

        ENetPacket* packet = enet_packet_create(
            &payload,
            sizeof(payload),
            ENET_PACKET_FLAG_UNSEQUENCED
        );
        if (packet) { enet_peer_send(peer, 1, packet); }
    }
}


void server_broadcast_sound(Server* server, uint32_t server_id, bool client_side, SoundID id, Vec3 pos, float vol, float rang) {

    //more elagant solution would be -> server only sends to specific clients based on sound handled clientside, distance and states (flashbangs coming soon?)
    Packet_add_sound sound_payload = {
        .pckt_id = (packet_t)ADD_SOUND,
        .client_side = client_side, // was this sound handled clientside?
        .server_id = server_id,   //used so client can filter -> so wasteful
        .location = pos,
        .sound_id = id,
        .volume = vol,
        .range = rang
    };

    ENetPacket* sound_packet = enet_packet_create(
        &sound_payload,
        sizeof(sound_payload),
        ENET_PACKET_FLAG_RELIABLE //ensure audio q's happen even if late
    );
    enet_host_broadcast(server->e_server, 1, sound_packet);
}


void server_disconnect_peers(Server* server) {
    if (!server || !server->e_server) return;

    // Disconnect all connected peers
    for (size_t i = 0; i < server->e_server->peerCount; ++i) {
        ENetPeer* peer = &server->e_server->peers[i];
        if (peer->state == ENET_PEER_STATE_CONNECTED) {
            enet_peer_disconnect(peer, 0);
        }
    }


    uint32_t breakout = 0;
    ENetEvent event;
    while (enet_host_service(server->e_server, &event, 1000) > 0 && breakout < server->e_server->peerCount) {
        switch (event.type) {
            case ENET_EVENT_TYPE_RECEIVE:
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                breakout++;
                break;
            default:
                break;
        }
    }
}


void server_add_client(Server* server, uint32_t server_id, uint32_t uqid, char* name) {
    if (!server) {return;}
    if (!name) {
        char str[10] = {0};
        name = "Player_" + snprintf(str, sizeof(str), "%u", server_id);
    }

    level_server_add_player(&server->level, server, uqid, server_id);

    client_t client = {
        .name = name,
        .state_buffer_idx = 0,
        .input_buffer_idx = 0,
        .peer_id = server_id,
        .bus = createBus()
    };

    push_client_buffer(server->clients, &server->clients_connected, MAX_CLIENTS, &client);

    SERVER_LOG("client added.");

    //this works till people start unjoining
}

void server_handle_events(Server* server) {
    for (int i = 0; i < server->clients_connected; i++) {
        client_t client = server->clients[i];

        process_events(&client.bus, client.player);
    }
}

static void server_apply_client_inputs(Server* s) {
    for (uint32_t i = 0; i < s->clients_connected; i++) {
        client_t* ct = &s->clients[i];
        usercmd_t* cmd = &ct->latest_cmd;

        uint32_t peer_id = (uint32_t)ct->peer_id;
        int idx = hmgeti(s->level.player_map, peer_id);
        if (idx == -1) continue;

        Player* p = &s->level.player_map[idx].value;

        p->cam_forward = cmd->angles;
        p->movement.wish_dir = cmd->wishdir;
        p->gun_idx = cmd->gun_idx;

        if (cmd->actions & (1U << JUMP)) {
            handle_jump(p);
        }
        if (cmd->actions & (1U << DASH)) {
            handle_dash(p);
        }
    }
}








