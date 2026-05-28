#include "client.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "stb_ds.h"
#include "states.h"
#include "ground.h"

#include "event.h"

#include "gun.h"


Vec3 client_input_basic(InputHandle *player_input, const Camera* player_camera);
usercmd_t build_usercmd(const Client* c);

void client_add_tracer(Client* client, Tracer tracer);

void client_input_actions(Client* client);

bool client_enet_startup(Client* client);
bool client_enet_connect(Client* client, const char* host, uint32_t port);
void client_enet_poll(Client* client);

void update_gun_cooldowns(Client* c, float dt);
void reload_animation(Client* c, float dt);
void aiming_animation(Client* client, float dt);

void update_dash(Client* client, float dt);

static bool client_check_dead(const Client* c);


bool client_startup(Client *client, const char* host, uint32_t port) {
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

    client->aiming = false;
    client->dead = false;
    client->mouse_sensitivity = 0.0007f;
    client->actions = 0;

    sh_new_strdup(client->model_cache);

    Model ground = temp_create_plane();
    Model skybox = temp_create_skybox();
    Model tea = temp_create_model("teapot.obj", "sand.jpg", client->model_cache);

    Model hammer = temp_create_model("hammer.obj", "mace.png", client->model_cache);
    //Model bird = temp_create_model("figure.obj", NULL, client->model_cache);
    Model gun = temp_create_model("blunder.obj", "blunder.png", client->model_cache);
    Model othergun = temp_create_model("othergun.obj", "othergun.bmp", client->model_cache);

    //level_add_model(&client->level, &gun);

    client->stationary_vec = (Vec3){0.0f, 0.0f, 0.0f};


    client->gun_models[0] = gun;
    client->gun_models[1] = othergun;
    client->gun_models[2] = hammer;

    client->gun_models[0].offset = (Vec3){0.3f, -0.3f, -0.7f};
    client->gun_models[0].scale = (Vec3){0.7f, 0.7f, 0.7f};
    client->gun_models[0].rotation = (Vec3){0.3f, 0.3f, -0.5f};

    client->gun_models[1].offset = (Vec3){0.3f, -0.3f, -0.7f};
    client->gun_models[1].scale = (Vec3){0.12f, 0.12f, 0.12f};
    client->gun_models[1].rotation = (Vec3){0.3f, 0.1f, -0.3f};

    client->gun_models[2].offset = (Vec3){0.45f, -0.3f, -0.7f};
    client->gun_models[2].scale = (Vec3){0.3f, 0.3f, 0.3f};
    client->gun_models[2].rotation = (Vec3){0.3f, 2.9f, -0.4f};

    client->dead_mdl = temp_create_model("skull.obj", "skull.jpg", client->model_cache);

    
    tea.offset = (Vec3){5.0f, 0.0f, 0.0f};
    tea.scale = (Vec3){2.0f, 2.0f, 2.0f};


    level_add_model(&client->level, &ground);
    level_add_model(&client->level, &skybox);
    level_add_model(&client->level, &tea);

    client->gun_view_offset = (Vec3){0.0f, 0.0f, 0.0f};


    client->player = NULL;
    client->established_server_connnection = false;

    camera_init(&client->player_camera);
    client->player_camera.mode = 1;

    srand((unsigned int)time(NULL));
    client->unique_id = ((uint64_t)(uint32_t)rand() << 32) | (uint64_t)(uint32_t)rand();


    if (!client_enet_startup(client))
        return false;

    if (!client_enet_connect(client, host, port))
        return false;
    
    window_add_overlay(&client->win, "fps", "FPS: 0", 20, 10);
    window_add_overlay(&client->win, "dir", "Dir: 0,0", 20, 50);
    window_add_overlay(&client->win, "vel", "Vel: 0", 20, 90);
    window_add_overlay(&client->win, "hp", "HP: 0", 20, 950);

    window_add_overlay_image(&client->win, "crosshair", "dot.png", (client->win.width/ 2) - 10, (client->win.height / 2) - 10);

    window_add_overlay_image(&client->win, "hit", "hit.png", (client->win.width/ 2) - 15, (client->win.height / 2) - 15);
    window_toggle_overlay_image(&client->win, "hit", false);
    client->hit_overlay_timer = 0.0f;

    window_add_overlay_image(&client->win, "tail0", "dash.png", (client->win.width) - 90 - 90, (client->win.height) - 30 - 90);
    window_add_overlay_image(&client->win, "tail1", "dash.png", (client->win.width) - 180 - 90, (client->win.height) - 30 - 90);
    window_add_overlay_image(&client->win, "tail2", "dash.png", (client->win.width) - 270 - 90, (client->win.height) - 30 - 90);

    client->tracer_count = 0;

    Ground* map =  &client->level.ground;

    Model terrain = model_generate_map(map);
    terrain.offset = (Vec3) {(float)map->x_size * map->xz_scale / -2.0f, 0.0f, (float)map->z_size * map->xz_scale / -2.0f};

    level_add_model(&client->level, &terrain);


    if (!sound_init(&client->sound)) {
        return false;
    }

    sound_sync_loader(&client->sound);


    printf("Client Startup Successful\n");


    return true;
}

bool client_run(Client *client)
{
    if (!client) {return false;}
    if (!client->established_server_connnection) {return false;}

    if (client->enet_connected && !client->established_server_connnection) {
        prinf("waiting for server ack... \n");
        while (!client->established_server_connnection) {
            //stall here
        }
    }



    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT)
            return false;
    }

    //timing
    Timing* t = &client->time;
    
    uint64_t now = SDL_GetPerformanceCounter();
    uint64_t frame_ticks = now - t->last_time;
    float dt = (float)frame_ticks / (float)t->perf_freq;
    t->last_time = now;


    t->fps_time_accum += frame_ticks;
    t->frame_count++;
    double fps = NAN;
    if (t->fps_time_accum >= t->perf_freq) {
        fps = (double)t->frame_count * (double)t->perf_freq / (double)t->fps_time_accum;
        //printf("%sPasses:%d\r", server_tag, t->frame_count);
        //fflush(stdout);
        t->fps_time_accum = 0;
        t->frame_count = 0;
    }

    t->server_time += dt; //needs to sync with server


    t->accumulator += dt;
    if (t->accumulator > t->max_accumulator) {t->accumulator = t->max_accumulator;}

    //main tick loop
    while (t->accumulator >= t->tick_time) {

        level_update(&client->level, dt); // updates the positions // check collisions // advances timers

        t->accumulator -= t->tick_time;
        t->tick++;
    }

    if (client->player) {
        client->player_camera.position = &client->player->entity.position;
        //client->dead = client_check_dead(client);
        if (client->dead) {
            //set_state(&client->player->entity, THIRDPERSON);
            //client->player_camera.position = &client->stationary_vec;
            client->aiming = false;
        }
        client->player_camera.mode = is_state(&client->player->entity, THIRDPERSON) || client->dead ? 0 : 1;
    } else {
        client->dead = false;
    }

    aiming_animation(client, dt);

    dMouse delta_mouse = input_mouse(&client->player_input);

    client->player_camera.angles.x -= delta_mouse.x * client->mouse_sensitivity;
    client->player_camera.angles.y -= delta_mouse.y * client->mouse_sensitivity;

     client->input_buffer_idx = 0;


    if (client->player_camera.angles.y > 1.5f)
        client->player_camera.angles.y = 1.5f;
    if (client->player_camera.angles.y < -1.5f)
        client->player_camera.angles.y = -1.5f;

    client->level.fps_time_accum += frame_ticks;
    client->level.frame_count++;

    fps_ui_ticks_accum += frame_ticks;
    fps_ui_frame_count++;

    if (fps_ui_ticks_accum >= (client->level.perf_freq / 6)) {
        float seconds = (float)fps_ui_ticks_accum / (float)client->level.perf_freq;
        float fps = seconds > 0.0f ? ((float)fps_ui_frame_count / seconds) : 0.0f;
        float vel = client->player ? vec3_mag(&(Vec3){client->player->entity.velocity.x, 0.0f, client->player->entity.velocity.z}) : 0.0f;
        Vec3 vel3 = client->player ? client->player->entity.velocity : (Vec3){0.0f,0.0f, 0.0f};
        Vec3 dir = client->player ? client->player_camera.angles : (Vec3){0.0f,0.0f, 0.0f};
        float health = client->player ? client->player->entity.health : 0.0f;

        char fps_text[64];
        char vel_text[64];
        char dir_text[64];
        char health_text[64];

        SDL_snprintf(fps_text, sizeof(fps_text), "FPS: %.0f", fps);
        SDL_snprintf(dir_text, sizeof(dir_text), "Dir: %.2f, %.2f", dir.x, dir.y);
        SDL_snprintf(vel_text, sizeof(vel_text), "Vel: %.2f || <%.2f, %.2f, %.2f>", vel, vel3.x, vel3.y, vel3.z);

        SDL_snprintf(health_text, sizeof(fps_text), "hp: %.0f", health);

        window_update_overlay(&client->win, "fps", fps_text);
        window_update_overlay(&client->win, "dir", dir_text);
        window_update_overlay(&client->win, "vel", vel_text);
        window_update_overlay(&client->win, "hp", health_text);

        if(client->player != NULL) {
            window_toggle_overlay_image(&client->win, "tail0", client->player->dash.current_charges >= 1);
            window_toggle_overlay_image(&client->win, "tail1", client->player->dash.current_charges >= 2);
            window_toggle_overlay_image(&client->win, "tail2", client->player->dash.current_charges >= 3);
        }

        fps_ui_ticks_accum = 0;
        fps_ui_frame_count = 0;
    }
    


    if (client->level.fps_time_accum >= client->level.perf_freq) {
        client->level.fps_time_accum = 0;
        client->level.frame_count = 0;
    }

    send_time_accum += frame_ticks;
    const Uint64 send_interval = client->level.perf_freq / client->level.tick_rate;
    while (client->enet_connected && client->player && send_time_accum >= send_interval) {
        Packet_state payload = {
            PCKT_CLIENT_STATE,
            client->server_id,
            client->player->entity.position,
            client->player->entity.velocity,
            camera_forward(&client->player_camera),
            client->player->entity.states,
            client->player->entity.health,
            client->player->eye_offset
        };
        ENetPacket* packet = enet_packet_create(
            &payload,
            sizeof(payload),
            0
        );
        enet_peer_send(client->server_peer, 1, packet);
        send_time_accum -= send_interval;
    }

    if (client->player) {
        client->player->movement.cam_forward = camera_forward(&client->player_camera);
        client->player->movement.cam_right = camera_right(&client->player_camera);
        float yaw = atan2f(-client->player->movement.cam_forward.z, client->player->movement.cam_forward.x);
        client->player->entity.model.rotation.y = yaw;
        if (client->dead) {
            client->player->movement.wish_dir = (Vec3){0.0f, 0.0f, 0.0f};
            client->player->movement.jump_queued = false;
            client->player->movement.slide_queued = false;
            client->player->movement.dash_queued = false;
            client->player->shoot_queued = false;
            client->aiming = false;
        } else {
            client_input_actions(client);
        }
    }


    client_enet_poll(client);

    if (!level_update(&client->level, dt))
        return false;

    if (client->player) {
        if (is_state(&client->player->entity, SLIDING)) {
            const float target_eye_y = 1.0f;
            const float snap_epsilon = 0.05f;

            float delta = target_eye_y - client->player->eye_offset.y;
            if (fabsf(delta) <= snap_epsilon) {
                client->player->eye_offset.y = target_eye_y;
            } else {
                client->player->eye_offset.y += 5.0f * delta * dt;
            }
            

        }
        else if (is_state(&client->player->entity, GLIDING)) {
            //client->player->eye_offset.y = 0.0f;
        } else {
            const float target_eye_y = 1.0f;
            const float snap_epsilon = 0.001f;

            float delta = target_eye_y - client->player->eye_offset.y;
            if (fabsf(delta) <= snap_epsilon) {
                client->player->eye_offset.y = target_eye_y;
            } else {
                client->player->eye_offset.y += 5.0f * delta * dt;
            }
        }
    }

    if (client->player) {
        Vec3 third_person_offset = {0.0f, 0.7f, 4.5f};
        camera_mode_control(&client->player_camera, &client->player->eye_offset, &third_person_offset, dt);
    }

    for (uint32_t i = 0; i < client->tracer_count;) {
        client->tracers[i].lifespan -= dt;
        if (client->tracers[i].lifespan <= 0.0f) {
            client->tracers[i] = client->tracers[client->tracer_count - 1];
            client->tracer_count--;
        } else {
            i++;
        }
    }

    if (client->player) {
        reload_animation(client, dt);
        update_gun_cooldowns(client, dt);
    }

    
    float a = t->accumulator / t->tick_time;
    client_render(client, a);

    for (int i = 0; i < hmlen(client->level.player_map); i++) {
        client->level.player_map[i].value.entity.prev_position = client->level.player_map[i].value.entity.position;
    }
    for (int i = 0; i < hmlen(client->level.ent_map); i++) {
        client->level.ent_map[i].value.prev_position = client->level.ent_map[i].value.position;
    }

    return true;
}


void client_close(Client *client)
{
    if (!client)
        return;

    if (client->server_peer && client->e_client) {

        //try to disconnect nicely
        enet_peer_disconnect(client->server_peer, 0); //add reasons later?

        ENetEvent event;
        int attempts = 0;
        while (attempts < 200 && enet_host_service(client->e_client, &event, 10) > 0) {
            if (event.type == ENET_EVENT_TYPE_RECEIVE)
                enet_packet_destroy(event.packet);
            else if (event.type == ENET_EVENT_TYPE_DISCONNECT)
                break;
            attempts++;
        }
        enet_peer_reset(client->server_peer); //disconnect meanly
        client->server_peer = NULL;
    }

    if (client->e_client) {
        enet_host_destroy(client->e_client);
        client->e_client = NULL;
    }

    enet_deinitialize();

    shfree(client->model_cache);
    client->model_cache = NULL;

    window_destroy(&client->win);
    window_quit(); //why would i do this

    level_destroy(&client->level);

    printf("\nClient closed!\n");
}


void client_render(Client *client, float alpha) {

    if (!client->player) {return;}

    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    Shader* basic_shader = &client->player_camera.shader;

    shader_use(basic_shader);    

    Vec3 light_dir = { 0.86f, -0.8f, 0.52f };
    shader_set_vec3(basic_shader, "lightDir", &light_dir);

    Vec3 thing = vec3_lerp(&client->player->entity.prev_position, &client->player->entity.position, alpha);
    client->player_camera.position = &thing;


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
        Vec3 thing = {0.0f, 0.0f, 0.0f};
        render_model(&client->level.models[i], thing, basic_shader, temp_color[6]);
    }

    for (int i = 0; i < hmlen(client->level.player_map); i++) {
        if (client->level.player_map[i].key != client->server_id) {
            Entity* pe = &client->level.player_map[i].value.entity;
            Vec3 lerp_pos = vec3_lerp(&pe->prev_position, &pe->position, alpha);

            if (is_state(pe, DEAD)) {
                client->dead_mdl.rotation.y = atan2f(-client->level.player_map[i].value.movement.cam_forward.z, client->level.player_map[i].value.movement.cam_forward.x);
                render_model(&client->dead_mdl, lerp_pos, basic_shader, temp_color[6]);
            }
            else {
                render_entity_at(pe, lerp_pos, basic_shader, temp_color[((client->level.player_map[i].value.unqid)) % 7]);
            }
            
        }
    }

    if (client->player && client->player_camera.mode == 0) {
        Vec3 lerp_pos = vec3_lerp(&client->player->entity.prev_position, &client->player->entity.position, alpha);
        if (is_state(&client->player->entity, DEAD)) {
                client->dead_mdl.rotation.y  = atan2f(-client->player->movement.cam_forward.z, client->player->movement.cam_forward.x);
                render_model(&client->dead_mdl, lerp_pos, basic_shader, temp_color[6]);
        } else {
            render_entity_at(&client->player->entity, lerp_pos, basic_shader, temp_color[(client->player->unqid) % 7]);
        }
        
    }

    for (int i = 0; i < hmlen(client->level.ent_map); i++) {
        Entity* e = &client->level.ent_map[i].value;
        Vec3 lerp_pos = vec3_lerp(&e->prev_position, &e->position, alpha);
        render_entity_at(e, lerp_pos, basic_shader, temp_color[6]);
    }

    for (uint32_t i = 0; i < client->tracer_count; i++) {
        render_line(client->tracers[i].start, client->tracers[i].end, client->tracers[i].color);
    }

    if (client->player_camera.mode == 1) {
        render_model_static(&client->gun_models[client->player->gun_idx], &client->player_camera, client->gun_view_offset, basic_shader, temp_color[6]);
    }

    client->player_camera.position = &client->player->entity.position;


    window_render_overlay(&client->win);
    window_swap_buffers(&client->win);
}


void client_input_actions(Client* client) {
    if (!client->player) {return;}
    uint32_t* actions = &client->actions;
    InputHandle *player_input = &client->player_input;
    Camera* player_camera = &client->player_camera;

    Vec3 dir = {0.0f, 0.0f, 0.0f};
    dMouse mb = input_mouse(player_input);

    if (player_input->kb_state[SDL_SCANCODE_W]) {dir.z += 1.0f; *actions |= (1U << FORWARD);}
    if (player_input->kb_state[SDL_SCANCODE_S]) {dir.z -= 1.0f; *actions |= (1U << BACKWARD);}
    if (player_input->kb_state[SDL_SCANCODE_A]) {dir.x += 1.0f; *actions |= (1U << LEFT);}
    if (player_input->kb_state[SDL_SCANCODE_D]) {dir.x -= 1.0f; *actions |= (1U << RIGHT);}

    if(player_input->kb_state[SDL_SCANCODE_SPACE]) {*actions |= (1U << JUMP);}

    //player_input->kb_state[SDL_SCANCODE_LSHIFT] ? set_state(&client->player->entity, RUNNING) : clear_state(&client->player->entity, RUNNING);
    clear_state(&client->player->entity, RUNNING);
    client->player->movement.jump_queued = player_input->kb_state[SDL_SCANCODE_SPACE];

    client->player->movement.slide_queued = player_input->kb_state[SDL_SCANCODE_LCTRL];

    if (player_input->kb_state[SDL_SCANCODE_1]) {
        client->player->gun_idx = 0;
    }

    if (player_input->kb_state[SDL_SCANCODE_2]) {
        client->player->gun_idx = 1;
    }

    if (player_input->kb_state[SDL_SCANCODE_3] || player_input->kb_state[SDL_SCANCODE_Q]  ) {
        client->player->gun_idx = 2;
    }

    bool tab = input_key_pressed(player_input, SDL_SCANCODE_TAB);
    if (tab) {
        is_state(&client->player->entity, THIRDPERSON) ? 
        clear_state(&client->player->entity, THIRDPERSON) : set_state(&client->player->entity, THIRDPERSON);
    }

    //client->player->movement.dash_queued |= input_key_pressed(player_input, SDL_SCANCODE_R);

    if (input_key_pressed(player_input, SDL_SCANCODE_R)) {
        *actions |= (1U << DASH);

        if (client->player->dash.current_charges > 0) {
            sound_play_id(&client->sound, SOUND_DASH, 0.2f);
            level_handle_dash(client->player);
        } else { sound_play_id(&client->sound, SOUND_CANT, 0.5f); }
    }
    

    client->aiming = input_mb_held(player_input, SDL_BUTTON_RMASK);
    //client->player->shoot_queued |= input_mb_pressed(player_input, SDL_BUTTON_LMASK);

    if (input_mb_pressed(player_input, SDL_BUTTON_LMASK)) {
        sys_queueEvent(&client->level.event_bus, client->level.tick, KEY_EVENT, SHOOT, 1, 0, NULL);
        *actions |= (1U << SHOOT);
    }

    client->player->guns.guns[client->player->gun_idx].seed = gun_seed_gen(client->player->guns.guns[client->player->gun_idx]);
    if (client->player->shoot_queued && client->player->guns.guns[client->player->gun_idx].wait_time > 0.0f) {
        sound_play_id(&client->sound, SOUND_CANT, 0.8f);
    }

    if (input_mb_pressed(player_input, SDL_BUTTON_LMASK) && client->player->guns.guns[client->player->gun_idx].wait_time <= 0.0f) {

        if (client->player->gun_idx == 0) {sound_play_id(&client->sound, SOUND_BLUNDER, 0.8f);}
        if (client->player->gun_idx == 1) {sound_play_id(&client->sound, SOUND_MUSKET, 0.5f);}
        if (client->player->gun_idx == 2) {sound_play_id(&client->sound, SOUND_MACE, 0.6f);}

        if (client->player->guns.guns[client->player->gun_idx].tracers) {
            uint8_t rays = 1;
            uint8_t spread = 0;
            gun_seed_read(client->player->guns.guns[client->player->gun_idx].seed, &rays, &spread);
            if (rays == 0) {
                rays = 1;
            }

            Vec3 base_forward = camera_forward(player_camera);
            base_forward = vec3_normalize(&base_forward);

            Vec3 start = vec3_add(&client->player->entity.position, &client->player->eye_offset);


            for (int i = 0; i < rays; i++) {
                Vec3 spread_dir = gun_spread(client->player->guns.guns[client->player->gun_idx].seed, (uint8_t)i, base_forward);

                Vec3 ray = vec3_multiply(&spread_dir, 3000.0f);
                Vec3 end = vec3_add(&start, &ray);

                Tracer tracer = {
                    start,
                    end,
                    (Vec3){1.0f, 1.0f, 1.0f},
                    0.3f
                };
                client_add_tracer(client, tracer);
            }
        }

    }

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

void client_add_tracer(Client* client, Tracer tracer) {
    if (!client) return;

    if (client->tracer_count < 32) {
        client->tracers[client->tracer_count++] = tracer;
    } else {
        for (uint32_t i = 1; i < 32; i++) {
            client->tracers[i - 1] = client->tracers[i];
        }
        client->tracers[31] = tracer;
    }
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

bool client_enet_connect(Client* client, const char* host, uint32_t port) {
    if (!host) host = "127.0.0.1";
    if (port == 0) {
        port = 7777;
    }
    if (port > 65535) {
        fprintf(stderr, "Invalid port %u.\n", (unsigned)port);
        return false;
    }

    enet_uint16 enet_port = (enet_uint16)port;

    if (enet_address_set_host(&client->address, host) != 0) {
        fprintf(stderr, "Failed to resolve host '%s'.\n", host);
        return false;
    }
    client->address.port = enet_port;

    client->server_peer = enet_host_connect(client->e_client, &client->address, 2, 0);
    if (client->server_peer == NULL) {
        fprintf(stderr, "No available peers for initiating an ENet connection.\n");
        return false;
    }

    client->enet_connect_attempted = true;
    printf("Connecting to %s:%u...\n", host, (unsigned)enet_port);

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
                        sound_play_id(&client->sound, SOUND_JOIN, 0.8f);
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

                    if (packet_type == PCKT_SERVER_STATE && event.packet->dataLength >= sizeof(Packet_state)) {
                        const Packet_state* pos_pack = (const Packet_state*)event.packet->data;
                        client->level.server_time = pos_pack->server_time;

                        int idx = hmgeti(client->level.player_map, pos_pack->server_id);
                        if (idx != -1) {

                            if (pos_pack->server_id == client->server_id) {
                                Player* p = &client->level.player_map[idx].value;
                                p->entity.health = pos_pack->health;
                                p->dash.current_charges = pos_pack->current_charges;

                                // Always accept server-authoritative position/velocity
                                p->entity.position = pos_pack->pos;
                                p->entity.velocity = pos_pack->vel;

                                // Replay unacknowledged inputs from input buffer
                                int unacked = 0;
                                for (int bi = 1; bi <= (int)client->input_buffer_idx && bi <= INPUT_BUFFER_SIZE; bi++) {
                                    int buf_idx = (client->input_buffer_idx - bi) % INPUT_BUFFER_SIZE;
                                    if (client->input_buffer[buf_idx].tick > pos_pack->server_tick) {
                                        unacked++;
                                    } else {
                                        break;
                                    }
                                }

                                for (int bi = unacked; bi >= 1; bi--) {
                                    int buf_idx = (client->input_buffer_idx - bi) % INPUT_BUFFER_SIZE;
                                    usercmd_t* cmd = &client->input_buffer[buf_idx];
                                    p->movement.wish_dir = cmd->wishdir;
                                    vec3_normalize_inplace(&p->movement.wish_dir);
                                    p->movement.cam_forward = cmd->angles;
                                    Vec3 up = {0.0f, 1.0f, 0.0f};
                                    Vec3 right = vec3_cross(&up, &cmd->angles);
                                    if (vec3_mag_squared(&right) > 0.0f) {
                                        vec3_normalize_inplace(&right);
                                    } else {
                                        right = (Vec3){1.0f, 0.0f, 0.0f};
                                    }
                                    p->movement.cam_right = right;
                                    p->movement.jump_queued = (cmd->buttons & (1U << JUMP)) != 0;
                                    physics_step_player(&client->level, p, client->level.tick_time);
                                }
                            } else {
                                client->level.player_map[idx].value.entity.position = pos_pack->pos;
                                client->level.player_map[idx].value.entity.velocity = pos_pack->vel;
                                client->level.player_map[idx].value.movement.cam_forward = pos_pack->cam_dir;
                                float yaw = atan2f(-pos_pack->cam_dir.z, pos_pack->cam_dir.x);
                                client->level.player_map[idx].value.entity.model.rotation = (Vec3){0.0f, yaw, 0.0f};
                                client->level.player_map[idx].value.entity.states = pos_pack->state;

                                client->level.player_map[idx].value.entity.health = pos_pack->health;
                                client->level.player_map[idx].value.eye_offset = pos_pack->cam_offset;

                            }
                        }
                    }

                    if (packet_type == PCKT_ADD_PLAYER && event.packet->dataLength >= sizeof(Packet_player)) {
                        const Packet_player* player_pack = (const Packet_player*)event.packet->data;
                        if (hmgeti(client->level.player_map, player_pack->server_id) == -1) {
                            sound_play_id(&client->sound, SOUND_JOIN, 0.8f);
                            level_add_player(&client->level, player_pack->uqid, player_pack->server_id);
                        }
                    }

                    if (packet_type == PCKT_TRACER && event.packet->dataLength >= sizeof(Packet_add_tracer)) {
                        const Packet_add_tracer* player_knock = (const Packet_add_tracer*)event.packet->data;
                        int idx = hmgeti(client->level.player_map, player_knock->server_id);
                        if (idx != -1) {
                            Tracer t = (Tracer){player_knock->source, player_knock->dest, (Vec3){1.0f, 1.0f, 0.0f}, player_knock->time};
                            client_add_tracer(client, t);
                        }
                    }

                    if (packet_type == PCKT_REMOVE_PLAYER && event.packet->dataLength >= sizeof(Packet_player)) {
                        const Packet_player* player_pack = (const Packet_player*)event.packet->data;
                        hmdel(client->level.player_map, player_pack->server_id);
                    }

                    if (packet_type == PCKT_SERVER_AUTH_DEAD && event.packet->dataLength >= sizeof(Packet_server_auth_dead)) {
                        const Packet_server_auth_dead* dead = (const Packet_server_auth_dead*)event.packet->data;
                        int idx = hmgeti(client->level.player_map, dead->server_id);
                        if (idx != -1) {
                            set_state(&client->level.player_map[idx].value.entity, DEAD);
                            if (dead->server_id == client->server_id) {
                                client->dead = true;
                                client->stationary_vec = client->player->entity.position;
                                //set_state(&client->level.player_map[idx].value.entity, THIRDPERSON);
                            }
                        }
                    }

                    if (packet_type == PCKT_SERVER_AUTH_RESPAWN && event.packet->dataLength >= sizeof(Packet_server_auth_respawn)) {
                        const Packet_server_auth_respawn* respawn = (const Packet_server_auth_respawn*)event.packet->data;
                        int idx = hmgeti(client->level.player_map, respawn->server_id);
                        if (idx != -1) {

                            //level_add_ent_death(&client->level, respawn->server_id);

                            client->level.player_map[idx].value.entity.position = respawn->pos;
                            client->level.player_map[idx].value.entity.velocity = respawn->vel;
                            client->level.player_map[idx].value.entity.health = respawn->health;
                            clear_state(&client->level.player_map[idx].value.entity, DEAD);

                            if (respawn->server_id == client->server_id) {
                                client->dead = false;
                            }

                            client->level.player_map[idx].value.dash.current_charges = client->level.player_map[idx].value.dash.max_charges;

                        }
                    }
                    if (packet_type == PCKT_HIT_VERIFY && event.packet->dataLength >= sizeof(Packet_hit_verify)) {
                        const Packet_hit_verify* hit_v = (const Packet_hit_verify*)event.packet->data;
                        int idx = hmgeti(client->level.player_map, hit_v->server_id);
                        if (idx != -1) {
                            client->hit_overlay_timer = 0.3f;
                            window_toggle_overlay_image(&client->win, "hit", true);

                            if (hit_v->gun_idx == 2) {
                                client->player->dash.current_charges++;
                                //(hit_v->damage_amount > 70.0f) ? sound_play_name(&client->sound, "mace_smash", 1.1f) : sound_play_name(&client->sound, "mace_hit", 1.3f);
                            }

                            if (hit_v->kill) {sound_play_id(&client->sound, SOUND_KILL, 0.3f); client->player->dash.current_charges++;}
                        }
                    }

                    if (packet_type == PCKT_ADD_SOUND && event.packet->dataLength >= sizeof(Packet_add_sound)) {
                        const Packet_add_sound* sound = (const Packet_add_sound*)event.packet->data;
                        if (client->player) {
                            bool ignore_local = sound->client_side && (client->player->server_id == sound->server_id);
                            if (!ignore_local) {
                                float range = sound->range * sound->range;
                                float volume = sound->volume;
                                Vec3 dist = vec3_subtract(&client->player->entity.position, &sound->location);
                                float distf = vec3_mag_squared(&dist);
                                if (distf < range) {
                                    volume = volume - ((distf / range) * volume);
                                    sound_play_id(&client->sound, (SoundID)sound->sound_id, (volume > 0.05f) ? volume : 0.05f);
                                }
                            }
                        }
                        

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


void weapon_sway(Model* mdl, Player* p, float dt) {
    if (mdl == NULL || p == NULL) {return;}

    //lerp slightly towards camera and velocity changes

    //move / rotate model slightly left when moving left, or move / rotate


}

/*

straight to the unified header

*/

void update_gun_cooldowns(Player* p, const float dt) {
    Gun_stats* gun = &p->guns.guns[p->gun_idx];
    if (gun->wait_time) {
        gun->wait_time -= dt;
        if (gun->wait_time < 0.0f) {
            gun->wait_time = 0.0f;
        }
    }
}

/*

Half-baked animations

*/

//works for now
void aiming_animation(Client* client, const float dt) {
    if (!client->player) {return;}
    const float hip_fov = 103.0f;
    const float aim_fov = client->player->guns.guns[client->player->gun_idx].aim_fov;
    const float hip_sens = 0.0007f;
    const float aim_sens = hip_sens * (aim_fov / hip_fov);

    const float duration = client->aiming ? 0.05f : 0.01f;
    float t = (duration > 0.0f) ? (dt / duration) : 1.0f;
    if (t > 1.0f) t = 1.0f;
    if (t < 0.0f) t = 0.0f;

    const float target_fov = client->aiming ? aim_fov : hip_fov;
    const float target_sens = client->aiming ? aim_sens : hip_sens;

    client->player_camera.fov += (target_fov - client->player_camera.fov) * t;
    client->mouse_sensitivity += (target_sens - client->mouse_sensitivity) * t;

}


void reload_animation(Client* c, const float dt) {
    Player* p = c->player;
    Gun_stats* gun = &p->guns.guns[p->gun_idx];
    Vec3 target = {0.0f, 0.0f, 0.0f};

    if (gun->reload_time > 0.0f) {
        float ratio = gun->wait_time / gun->reload_time;
        if (ratio < 0.0f) ratio = 0.0f;
        if (ratio > 1.0f) ratio = 1.0f;
        target.y = -ratio;
    }

    float t = dt * 14.0f;
    if (t > 1.0f) t = 1.0f;
    c->gun_view_offset = vec3_lerp(&c->gun_view_offset, &target, t);

}


void hit_overlay_animation(Client* client, const float dt) {
    if (client->hit_overlay_timer > 0.0f) {
        client->hit_overlay_timer -= dt;
        if (client->hit_overlay_timer <= 0.0f) {
            client->hit_overlay_timer = 0.0f;
            window_toggle_overlay_image(&client->win, "hit", false); //currently you have to call 'window_toggle_overlay_image(&client->win, "hit", true);' when you hit
        }
    }
}



/*
//Legacy code--> might be good for spec mode?
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
    */





usercmd_t build_usercmd(const Client* c) {
    usercmd_t cmd;

    cmd.tick = c->tick;
    cmd.angles = camera_forward(&c->player_camera);
    cmd.buttons = c->actions;
    cmd.gun_idx = c->player->gun_idx;
    cmd.wishdir = c->player->movement.wish_dir;

    return cmd;

}