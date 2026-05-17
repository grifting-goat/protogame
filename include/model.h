#ifndef MODEL_H
#define MODEL_H

#include<glad/glad.h>
#include <stdint.h>
#include <stdbool.h>

#include "engine_math.h"


// Vertex structure
typedef struct {
    float position[3];
    float normal[3];
    float texcoord[2];
} Vertex;


typedef struct {
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    uint32_t vertex_count;
    uint32_t index_count;
    bool has_indices;

    GLuint texture;

} Mesh;


typedef struct {
    Mesh mesh; //only one mesh for now
    Vec3 offset;
    Vec3 scale;
    Vec3 rotation;
    bool use_lighting;
    //Uint32 mesh_count;
} Model;

bool model_create(Model* model, const Vertex* vertices, uint32_t vertex_count, const uint32_t* indices, uint32_t index_count, const char* texture_path);
Model model_create_empty(void);
Model model_load(const char* obj_path, const char* tex_path);
void model_draw(const Model* model);
void model_destroy(Model* model);

#endif // MODEL_H