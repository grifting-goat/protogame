#include "engine_math.h"

//if the imp you are looking for is not here its inlined and defined in math.h

void vec3_normalize_inplace(Vec3* v) {
    float len_sq = vec3_mag_squared(v);
    if (len_sq > 0.0f) {
        float inv_len = 1.0f / sqrtf(len_sq);
        v->x *= inv_len;
        v->y *= inv_len;
        v->z *= inv_len;
    }
}

Vec3 vec3_cross(const Vec3* a, const Vec3* b) {
    return (Vec3){
        a->y * b->z - a->z * b->y,
        a->z * b->x - a->x * b->z,
        a->x * b->y - a->y * b->x
    };
}

Vec3 vec3_normalize(const Vec3* v) {
    Vec3 result = *v;
    vec3_normalize_inplace(&result);
    return result;
}


//Mat4 implementations
Mat4 mat4_identity(void) {
    Mat4 result = {0};
    result.m[0] = result.m[5] = result.m[10] = result.m[15] = 1.0f;
    return result;
}

Mat4 mat4_multiply(Mat4 a, Mat4 b) {
    Mat4 result = {0};
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 4; k++) {
                result.m[i * 4 + j] += a.m[i * 4 + k] * b.m[k * 4 + j];
            }
        }
    }
    return result;
}

Mat4 mat4_translate(Vec3 translation) {
    Mat4 result = mat4_identity();
    result.m[12] = translation.x;
    result.m[13] = translation.y;
    result.m[14] = translation.z;
    return result;
}

Mat4 mat4_rotate_x(float angle) {
    Mat4 result = mat4_identity();
    float c = cosf(angle);
    float s = sinf(angle);
    result.m[5] = c;
    result.m[6] = s;
    result.m[9] = -s;
    result.m[10] = c;
    return result;
}

Mat4 mat4_rotate_y(float angle) {
    Mat4 result = mat4_identity();
    float c = cosf(angle);
    float s = sinf(angle);
    result.m[0] = c;
    result.m[2] = -s;
    result.m[8] = s;
    result.m[10] = c;
    return result;
}

Mat4 mat4_rotate_z(float angle) {
    Mat4 result = mat4_identity();
    float c = cosf(angle);
    float s = sinf(angle);
    result.m[0] = c;
    result.m[1] = s;
    result.m[4] = -s;
    result.m[5] = c;
    return result;
}

Mat4 mat4_scale(Vec3 scale) {
    Mat4 result = mat4_identity();
    result.m[0] = scale.x;
    result.m[5] = scale.y;
    result.m[10] = scale.z;
    return result;
}

// cool projection matrix that converts 3D position to 2D screen
// tried to understand, eventually just copy paste
Mat4 mat4_perspective(float fov, float aspect, float near, float far) {
    Mat4 result = {0};
    float tan_half_fov = tanf(fov * 0.5f);
    
    result.m[0] = 1.0f / (aspect * tan_half_fov);
    result.m[5] = 1.0f / tan_half_fov;
    result.m[10] = -(far + near) / (far - near);
    result.m[11] = -1.0f;
    result.m[14] = -(2.0f * far * near) / (far - near);
    
    return result;
}

// view matrix for camera positioning //didnt bother understanding
Mat4 mat4_look_at(Vec3 eye, Vec3 center, Vec3 up) {
    Vec3 f = vec3_subtract(&center, &eye);
    vec3_normalize_inplace(&f);
    
    Vec3 u = vec3_normalize(&up);
    Vec3 s = vec3_cross(&f, &u);
    vec3_normalize_inplace(&s);
    
    u = vec3_cross(&s, &f);
    
    Mat4 result = mat4_identity();
    result.m[0] = s.x;
    result.m[4] = s.y;
    result.m[8] = s.z;
    result.m[1] = u.x;
    result.m[5] = u.y;
    result.m[9] = u.z;
    result.m[2] = -f.x;
    result.m[6] = -f.y;
    result.m[10] = -f.z;
    result.m[12] = -vec3_dot(&s, &eye);
    result.m[13] = -vec3_dot(&u, &eye);
    result.m[14] = vec3_dot(&f, &eye);
    
    return result;
}