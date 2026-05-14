#include "client.h"
#include <glad/glad.h>
#include <string.h>
#include <stdio.h>
#include "packet.h"
#include "stb_ds.h"
#include "states.h"

void client_render(Client *client);

Vec3 client_input_basic(InputHandle *player_input, const Camera* player_camera);
void client_input_test(Client* client, InputHandle *player_input, const Camera* player_camera);

bool client_enet_startup(Client* client);
bool client_enet_connect(Client* client, const char* host);
void client_enet_poll(Client* client);


bool client_startup(Client *client, const char* host)
{
    if (!client || !host)
        return false;

    if (!level_create(&client->level, 128))
        return false;

    if (!window_init())
        return false;

    if (!window_create(&client->win, "Proto", 1920, 1080, true)) {
        window_quit();
        return false;
    }

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        window_destroy(&client->win);
        window_quit();
        return false;
    }

    window_set_icon(&client->win, "icon.bmp");

    input_init(&client->player_input, &client->win);

    Model ground = temp_create_plane();
    level_add_model(&client->level, &ground);

    client->player = NULL;

    camera_init(&client->player_camera);
    client->player_camera.mode = 0;

    client->unique_id = ((uint64_t)(uint32_t)rand() << 32) | (uint64_t)(uint32_t)rand();

    if (!client_enet_startup(client))
        return false;

    if (!client_enet_connect(client, host))
        return false;
    
    window_add_overlay(&client->win, "fps", "FPS: 0", 20, 20);

    return true;
}

bool client_run(Client *client)
{
    if (!client)
        return false;

    static Uint64 send_time_accum = 0;
    static Uint64 fps_ui_ticks_accum = 0;
    static int fps_ui_frame_count = 0;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT)
            return false;
    }

    Uint64 now = SDL_GetPerformanceCounter();
    Uint64 frame_ticks = now - client->level.last_time;
    float dt = (float)frame_ticks / (float)client->level.perf_freq;
    client->level.last_time = now;

    dMouse delta_mouse = input_mouse(&client->player_input);
    float mouse_sensitivity = 0.0007f;

    client->player_camera.angles.x -= delta_mouse.x * mouse_sensitivity;
    client->player_camera.angles.y -= delta_mouse.y * mouse_sensitivity;

    if (client->player_camera.angles.y > 1.5f)
        client->player_camera.angles.y = 1.5f;
    if (client->player_camera.angles.y < -1.5f)
        client->player_camera.angles.y = -1.5f;

    client->level.fps_time_accum += frame_ticks;
    client->level.frame_count++;

    fps_ui_ticks_accum += frame_ticks;
    fps_ui_frame_count++;

    if (fps_ui_ticks_accum >= (client->level.perf_freq / 4)) {
        float seconds = (float)fps_ui_ticks_accum / (float)client->level.perf_freq;
        float fps = seconds > 0.0f ? ((float)fps_ui_frame_count / seconds) : 0.0f;
        char fps_text[64];
        SDL_snprintf(fps_text, sizeof(fps_text), "FPS: %.0f", fps);
        window_update_overlay(&client->win, "fps", fps_text);

        fps_ui_ticks_accum = 0;
        fps_ui_frame_count = 0;
    }
    


    if (client->level.fps_time_accum >= client->level.perf_freq) {
        client->level.fps_time_accum = 0;
        client->level.frame_count = 0;
    }

    send_time_accum += frame_ticks;
    const Uint64 send_interval = client->level.perf_freq / 64;
    while (client->enet_connected && client->player && send_time_accum >= send_interval) {
        Packet_pos payload = {PCKT_CLIENT_POS, client->server_id, client->player->entity.position, client->player->entity.velocity};
        ENetPacket* packet = enet_packet_create(
            &payload,
            sizeof(payload),
            0
        );
        enet_peer_send(client->server_peer, 1, packet);
        send_time_accum -= send_interval;
    }
    if (client->player) {
        client->player->movement.cam_dir = camera_forward(&client->player_camera);
        client_input_test(client, &client->player_input, &client->player_camera);
    }


    client_enet_poll(client);

    if (!level_update(&client->level, dt))
        return false;

    client_render(client);
    
    return true;
}


void client_render(Client *client) {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    Shader* basic_shader = &client->player_camera.shader;

    shader_use(basic_shader);    
    Mat4 view = camera_view_matrix(&client->player_camera);
    Mat4 projection = camera_projection_matrix(&client->player_camera);
    shader_set_mat4(basic_shader, "view", &view);
    shader_set_mat4(basic_shader, "projection", &projection);

    Vec3 temp_color[8] = {
        (Vec3){1.0f, 0.0f, 0.0f}, // red
        (Vec3){1.0f, 0.5f, 0.0f}, // orange
        (Vec3){1.0f, 1.0f, 0.0f}, // yellow
        (Vec3){0.0f, 1.0f, 0.0f}, // green
        (Vec3){0.0f, 0.0f, 1.0f}, // blue
        (Vec3){0.5f, 0.0f, 1.0f}, // purple
        (Vec3){1.0f, 1.0f, 1.0f}, // white
        (Vec3){0.0f, 0.0f, 0.0f}  // black
    };


    for (int i = 0; i < client->level.model_count; i++) {
        Vec3 thing = {0.0f, -2.0f, 0.0f};
        render_model(&client->level.models[i], thing, basic_shader, temp_color[6]);
    }

    for (int i = 0; i < hmlen(client->level.player_map); i++) {
        if (client->level.player_map[i].key != client->server_id) {
            render_entity(&client->level.player_map[i].value.entity, basic_shader, temp_color[((client->level.player_map[i].key) + 3) % 8]);
        }
    }

    for (int i = 0; i < hmlen(client->level.ent_map); i++) {
        if (client->level.ent_map[i].key != client->server_id) {
            render_entity(&client->level.ent_map[i].value, basic_shader, temp_color[7]);
        }
        
    }

    window_render_overlay(&client->win);
    window_swap_buffers(&client->win);
}

Vec3 client_input_basic(InputHandle *player_input, const Camera* player_camera) {
    Vec3 dir = {0.0f, 0.0f, 0.0f};
    if (player_input->kb_state[SDL_SCANCODE_W]) dir.z += 1.0f;
    if (player_input->kb_state[SDL_SCANCODE_S]) dir.z -= 1.0f;
    if (player_input->kb_state[SDL_SCANCODE_A]) dir.x += 1.0f;
    if (player_input->kb_state[SDL_SCANCODE_D]) dir.x -= 1.0f;
    
    if (player_input->kb_state[SDL_SCANCODE_SPACE]) dir.y += 1.0f;
    if (player_input->kb_state[SDL_SCANCODE_LCTRL]) dir.y -= 1.0f;

    Vec3 forward = camera_forward(player_camera);
    Vec3 right = camera_right(player_camera);

    forward.y = 0.0f; right.y = 0.0f;
    forward = vec3_normalize(&forward);
    right = vec3_normalize(&right);

    Vec3 move_dir = {
        forward.x * dir.z + right.x * dir.x,
        dir.y,
        forward.z * dir.z + right.z * dir.x
    };

    move_dir = vec3_normalize(&move_dir);

    return move_dir;
}

void client_input_test(Client* client, InputHandle *player_input, const Camera* player_camera) {
    Vec3 dir = {0.0f, 0.0f, 0.0f};

    if (player_input->kb_state[SDL_SCANCODE_W]) dir.z += 1.0f;
    if (player_input->kb_state[SDL_SCANCODE_S]) dir.z -= 1.0f;
    if (player_input->kb_state[SDL_SCANCODE_A]) dir.x += 1.0f;
    if (player_input->kb_state[SDL_SCANCODE_D]) dir.x -= 1.0f;

    player_input->kb_state[SDL_SCANCODE_LSHIFT] ? set_state(&client->player->entity, RUNNING) : clear_state(&client->player->entity, RUNNING);
    
    client->player->movement.jump_queued = player_input->kb_state[SDL_SCANCODE_SPACE];
    client->player->movement.slide_queued = player_input->kb_state[SDL_SCANCODE_LCTRL];


    Vec3 forward = camera_forward(player_camera);
    Vec3 right = camera_right(player_camera);

    forward.y = 0.0f; right.y = 0.0f;
    forward = vec3_normalize(&forward);
    right = vec3_normalize(&right);

    Vec3 move_dir = {
        forward.x * dir.z + right.x * dir.x,
        0,
        forward.z * dir.z + right.z * dir.x
    };

    move_dir = vec3_normalize(&move_dir);
    client->player->movement.wish_dir = move_dir;
}


bool client_enet_startup(Client* client) {
    if (enet_initialize() != 0) {
        fprintf(stderr, "An error occurred while initializing ENet.\n");
        return false;
    }

    ENetHost* e_client = { 0 };
    e_client = enet_host_create(NULL /* create a client host */,
        1 /* only allow 1 outgoing connection */,
        2 /* allow up 2 channels to be used, 0 and 1 */,
        0 /* assume any amount of incoming bandwidth */,
        0 /* assume any amount of outgoing bandwidth */);
    if (e_client == NULL) {
        fprintf(stderr,
        "An error occurred while trying to create an ENet client host.\n");
        return false;
    }
    
    client->e_client = e_client;
    client->server_peer = NULL;
    client->enet_connect_attempted = false;
    client->enet_connected = false;
    client->server_id = 0;

    ENetAddress address = { 0 };
    client->address = address;

    return true;
}

bool client_enet_connect(Client* client, const char* host) {
    if (!host) host = "127.0.0.1";
    const enet_uint16 port = 7777;

    if (enet_address_set_host(&client->address, host) != 0) {
        fprintf(stderr, "Failed to resolve host '%s'.\n", host);
        return false;
    }
    client->address.port = port;

    client->server_peer = enet_host_connect(client->e_client, &client->address, 2, 0);
    if (client->server_peer == NULL) {
        fprintf(stderr, "No available peers for initiating an ENet connection.\n");
        return false;
    }

    client->enet_connect_attempted = true;
    printf("Connecting to %s:%u...\n", host, (unsigned)port);

    return true;
}

void client_enet_poll(Client* client) {
    if (!client || !client->e_client) return;

    ENetEvent event;
    while (enet_host_service(client->e_client, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                client->enet_connected = true;
                printf("Client ENet connected.\n");
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                if (event.packet && event.packet->dataLength >= 1) {
                    const uint8_t* data = (const uint8_t*)event.packet->data;
                    const uint8_t packet_type = data[0];

                    if (packet_type == PCKT_SERVER_ID && event.packet->dataLength >= (1 + sizeof(uint32_t))) {
                        const Packet_server_id* id_pack = (const Packet_server_id *)event.packet->data;
                        client->server_id = id_pack->server_id;
                        printf("Assigned server id: %u\n", (unsigned)client->server_id);

                        level_add_player(&client->level, client->unique_id, client->server_id);
                        int p = hmgeti(client->level.player_map, client->server_id);
                        if (p != -1) {
                            client->player = &client->level.player_map[p].value;
                            camera_attach(
                                &client->player_camera,
                                &client->player->entity.position,
                                &client->player->eye_offset);
                        }

                        Packet_client_ack ack = {
                            PCKT_CLIENT_ACK,
                            client->server_id,
                            client->unique_id
                        };
                        ENetPacket* packet = enet_packet_create(
                            &ack,
                            sizeof(ack),
                            ENET_PACKET_FLAG_RELIABLE
                        );
                        if (packet) {
                            enet_peer_send(client->server_peer, 0, packet);
                        }
                    }

                    if (packet_type == PCKT_SERVER_POS && event.packet->dataLength >= sizeof(Packet_pos)) {
                        const Packet_pos* pos_pack = (const Packet_pos*)event.packet->data;
                        if (pos_pack->server_id == client->server_id) {
                            continue;
                        }
                        int idx = hmgeti(client->level.player_map, pos_pack->server_id);
                        if (idx != -1) {
                            client->level.player_map[idx].value.entity.position = pos_pack->pos;
                            client->level.player_map[idx].value.entity.velocity = pos_pack->vel;
                        }
                    }

                    if (packet_type == PCKT_ADD_PLAYER && event.packet->dataLength >= sizeof(Packet_player)) {
                        const Packet_player* player_pack = (const Packet_player*)event.packet->data;
                        if (hmgeti(client->level.player_map, player_pack->server_id) == -1) {
                            level_add_player(&client->level, player_pack->uqid, player_pack->server_id);
                        }
                    }

                    if (packet_type == PCKT_REMOVE_PLAYER && event.packet->dataLength >= sizeof(Packet_player)) {
                        const Packet_player* player_pack = (const Packet_player*)event.packet->data;
                        hmdel(client->level.player_map, player_pack->server_id);
                    }
                }

                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                client->enet_connected = false;
                client->server_peer = NULL;
                printf("Client ENet disconnected.\n");
                break;
            default:
                break;
        }
    }
}


void client_close(Client *client)
{
    if (!client)
        return;

    if (client->server_peer && client->e_client) {
        enet_peer_disconnect(client->server_peer, 0);
        ENetEvent event;
        int attempts = 0;
        while (attempts < 200 && enet_host_service(client->e_client, &event, 10) > 0) {
            if (event.type == ENET_EVENT_TYPE_RECEIVE)
                enet_packet_destroy(event.packet);
            else if (event.type == ENET_EVENT_TYPE_DISCONNECT)
                break;
            attempts++;
        }
        enet_peer_reset(client->server_peer);
        client->server_peer = NULL;
    }

    if (client->e_client) {
        enet_host_destroy(client->e_client);
        client->e_client = NULL;
    }
    enet_deinitialize();

    window_destroy(&client->win);
    window_quit();
    level_destroy(&client->level);

    printf("\nClient closed!\n");
}