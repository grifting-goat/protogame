#ifndef EERIE_SHADER_H
#define EERIE_SHADER_H

#include <stdbool.h>
#include <glad/glad.h>
#include "engine_math.h"


// this is AI i admit, this aint really that intersting to me rn

typedef struct {
    GLuint program;
} Shader;

// Shader functions
bool shader_create_from_source(Shader* shader, const char* vertex_source, const char* fragment_source);
bool shader_create_from_files(Shader* shader, const char* vertex_path, const char* fragment_path);
void shader_use(const Shader* shader);
void shader_set_mat4(const Shader* shader, const char* name, const Mat4* matrix);
void shader_set_vec3(const Shader* shader, const char* name, const Vec3* vector);
void shader_set_int(const Shader* shader, const char* name, int value);
void shader_destroy(Shader* shader);

#endif // EERIE_SHADER_H