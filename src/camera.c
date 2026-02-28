#include "camera.h"


void camera_init(Camera* cam) {
    static Vec3 default_position = {0.0f, 0.0f, 0.0f};
    if (!cam) return;
    cam->static_position = default_position;
    cam->position = &cam->static_position;
    cam->angles = (Vec3){0.0f, 0.0f, 0.0f}; // yaw, pitch, roll //rads
    cam->fov = 103.0f;
    cam->aspect = 16.0f / 9.0f;
    cam->near_plane = 0.05f;
    cam->far_plane = 250.0f;
    cam->mode = 1;
    cam->offset_vector = (Vec3){0.0f, 0.0f, 0.0f};
}

void camera_attach(Camera* cam, Vec3* position_ptr, Vec3* offset) {
    if (cam == NULL || position_ptr == NULL) {return;}
    cam->position = position_ptr;
    cam->offset_vector = *offset;
}

void camera_deattach(Camera* cam) {
    if (cam == NULL) {return;}
    //have the camera stay where it was when detached.
    cam->static_position = *cam->position;
    cam->position = &cam->static_position;
}


void camera_mode_control(Camera* cam, Vec3* offset_1, Vec3* offset_3) {
    switch (cam->mode) {
        case 1: //1st
            cam->offset_vector = *offset_1;
            break;
        case 0: //3rd
            Vec3 forward = camera_forward(cam);
            Vec3 up = {0.0f, 1.0f, 0.0f};
            Vec3 right = camera_right(cam);
            cam->offset_vector.x = 0
                - forward.x * offset_3->z
                + up.x * offset_3->y
                + right.x * offset_3->x;
            cam->offset_vector.y = 0 + offset_1->y
                - forward.y * offset_3->z
                + up.y * offset_3->y
                + right.y * offset_3->x;
            cam->offset_vector.z = 0
                - forward.z * offset_3->z
                + up.z * offset_3->y
                + right.z * offset_3->x;
            break;
    }

}

Vec3 camera_forward(const Camera* cam) {
    if (!cam) return (Vec3){0.0f, 0.0f, -1.0f};
    
    // Calculate forward vector from yaw and pitch
    // yaw = angles.x, pitch = angles.y
    float yaw = cam->angles.x;
    float pitch = cam->angles.y;
    
    Vec3 forward = {
        cosf(pitch) * cosf(yaw),    // x
        sinf(pitch),                // y  
        -cosf(pitch) * sinf(yaw)    // z (OpenGL: -z is forward)
    };
    
    return vec3_normalize(&forward);
}

Vec3 camera_right(const Camera* cam) {
    if (!cam) return (Vec3){1.0f, 0.0f, 0.0f};
    
    Vec3 forward = camera_forward(cam);
    Vec3 world_up = {0.0f, 1.0f, 0.0f};
    // Correct order: right = world_up x forward
    Vec3 right = vec3_cross(&world_up, &forward);
    return vec3_normalize(&right);
}

Vec3 camera_up(const Camera* cam) {
    if (!cam) return (Vec3){0.0f, 1.0f, 0.0f};
    
    Vec3 forward = camera_forward(cam);
    Vec3 right = camera_right(cam);
    
    Vec3 up = vec3_cross(&right, &forward);
    return vec3_normalize(&up);
}

Mat4 camera_projection_matrix(const Camera* cam) {
    if (!cam) return mat4_identity();
    return mat4_perspective(cam->fov, cam->aspect, cam->near_plane, cam->far_plane);
}

Mat4 camera_view_matrix(const Camera* cam) {
    if (!cam) return mat4_identity();
    
    Vec3 forward = camera_forward(cam);
    Vec3 pos = {cam->position->x + cam->offset_vector.x, cam->position->y + cam->offset_vector.y, cam->position->z + cam->offset_vector.z};
    
    Vec3 target = vec3_add(&pos, &forward);
    Vec3 up = {0.0f, 1.0f, 0.0f};
    
    return mat4_look_at(pos, target, up);
}

