#ifndef CAMERA_H
#define CAMERA_H

#include <stdint.h>
#include "engine_math.h"
#include "shader.h"

typedef struct {
    Vec3* position; //so you can attach it to things
    Vec3 static_position;
    Vec3 angles;
    float fov;
    float aspect;
    float near_plane;
    float far_plane;

    bool mode; // 1st : 3rd
    Vec3 offset_vector;

    Shader shader;
    
} Camera;

void camera_init(Camera* cam); //populate with default values

void camera_attach(Camera* cam, Vec3* position_ptr, Vec3* offset);
void camera_deattach(Camera* cam);

void camera_mode_control(Camera* cam, Vec3* offset_1, Vec3* offset_3);

//big math guy
Mat4 camera_view_matrix(const Camera* cam);
Mat4 camera_projection_matrix(const Camera* cam);

//direction vectors
Vec3 camera_forward(const Camera* cam); //the direction the camera is facing
Vec3 camera_right(const Camera* cam);   //the direction to the right of the camera
Vec3 camera_up(const Camera* cam);   //up if we want to do funny stuff



#endif //CAMERA_H