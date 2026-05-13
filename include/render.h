#ifndef RENDER_H
#define RENDER_H

#include "model.h"
#include "shader.h"
#include "entity.h"

#define TEX_CACHE_SIZE 32
typedef struct {
    char path[256];
    GLuint tex;
} TexCacheEntry; //eventually move this to a hashmap

void render_entity(Entity* ent, Shader* shader, Vec3 color);

void render_model(Model* model, Vec3 pos, Shader* shader, Vec3 color);

GLuint load_texture(const char* filename);


#endif // RENDER_H