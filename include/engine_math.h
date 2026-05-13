#ifndef ENGINE_MATH_H
#define ENGINE_MATH_H

#include <stdint.h>
#include <math.h>
#include <stdbool.h>

//Vector3 struct
typedef struct {
    float x;
    float y;
    float z;
} Vec3;

//Vec3 basic operations (inplace and inlined)
static inline void vec3_add_inplace(Vec3* a, const Vec3* b) {
    a->x += b->x;
    a->y += b->y;
    a->z += b->z;
}

static inline void vec3_subtract_inplace(Vec3* a, const Vec3* b) {
    a->x -= b->x;
    a->y -= b->y;
    a->z -= b->z;
}

static inline void vec3_multiply_inplace(Vec3* v, float scalar) {
    v->x *= scalar;
    v->y *= scalar;
    v->z *= scalar;
}

//vec3 Basic operations (inlined)
static inline Vec3 vec3_add(const Vec3* a, const Vec3* b) {
    return (Vec3){a->x + b->x, a->y + b->y, a->z + b->z};
}

static inline Vec3 vec3_subtract(const Vec3* a, const Vec3* b) {
    return (Vec3){a->x - b->x, a->y - b->y, a->z - b->z};
}

static inline Vec3 vec3_multiply(const Vec3* v, float scalar) {
    return (Vec3){v->x * scalar, v->y * scalar, v->z * scalar};
}

//vec3 linear operations (inlined)
static inline float vec3_dot(const Vec3* a, const Vec3* b) {
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

static inline float vec3_mag_squared(const Vec3* v) {
    return v->x * v->x + v->y * v->y + v->z * v->z;
}

static inline float vec3_mag(const Vec3* v) {
    return sqrtf(vec3_mag_squared(v));
}

static inline float vec3_distance_squared(const Vec3* a, const Vec3* b) {
    Vec3 diff = vec3_subtract(a, b);
    return vec3_mag_squared(&diff);
}

//vec3 more linear
void vec3_normalize_inplace(Vec3* v);
Vec3 vec3_normalize(const Vec3* v);
Vec3 vec3_cross(const Vec3* a, const Vec3* b);


//Mat4 struct
typedef struct {
    float m[16];  //Column-major order like OpenGL
} Mat4;

//Mat4 functions
Mat4 mat4_identity(void);
Mat4 mat4_multiply(Mat4 a, Mat4 b);
Mat4 mat4_translate(Vec3 translation);
Mat4 mat4_rotate_x(float angle);
Mat4 mat4_rotate_y(float angle);
Mat4 mat4_rotate_z(float angle);
Mat4 mat4_scale(Vec3 scale);
Mat4 mat4_perspective(float fov, float aspect, float near, float far);
Mat4 mat4_look_at(Vec3 eye, Vec3 center, Vec3 up);

#endif //ENGINE_MATH_H