#ifndef RENDER_H
#define RENDER_H

#include "model.h"
#include "shader.h"
#include "entity.h"
#include "camera.h"

#define TEX_CACHE_SIZE 32
typedef struct {
    char path[256];
    GLuint tex;
} TexCacheEntry; //eventually move this to a hashmap

void render_entity(Entity* ent, Shader* shader, Vec3 color);

void render_model(Model* model, Vec3 pos, Shader* shader, Vec3 color);

void render_model_static(Model* model, const Camera* camera, Vec3 view_offset, Shader* shader, Vec3 color);

void render_line(const Vec3 start, const Vec3 end);

GLuint load_texture(const char* filename);


#endif // RENDER_H