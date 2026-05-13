#include "render.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


void render_entity(Entity* ent, Shader* shader, Vec3 color) {
    Model* model = &ent->model;
    if (model->mesh.texture != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, model->mesh.texture);
        shader_set_int(shader, "texSampler", 0); // set sampler2D uniform to 0
        shader_set_int(shader, "useTexture", 1);
    } else {
        shader_set_int(shader, "useTexture", 0);
    }
    
    Mat4 offset = mat4_translate(vec3_add(&model->offset, &ent->position));
    Mat4 scale = mat4_scale(model->scale);
    Mat4 new_model = mat4_multiply(scale, offset);
    shader_set_mat4(shader, "model", &new_model);
    shader_set_vec3(shader, "objectColor", &color);
    model_draw(model);

}

void render_model(Model* model, Vec3 pos, Shader* shader, Vec3 color) {
    if (model->mesh.texture != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, model->mesh.texture);
        shader_set_int(shader, "texSampler", 0); // set sampler2D uniform to 0
        shader_set_int(shader, "useTexture", 1);
    } else {
        shader_set_int(shader, "useTexture", 0);
    }
    
    Mat4 offset = mat4_translate(vec3_add(&model->offset, &pos));
    Mat4 scale = mat4_scale(model->scale);
    Mat4 new_model = mat4_multiply(scale, offset);
    shader_set_mat4(shader, "model", &new_model);
    shader_set_vec3(shader, "objectColor", &color);
    model_draw(model);

}

GLuint load_texture(const char* filename) {
    static TexCacheEntry cache[TEX_CACHE_SIZE];
    static int cache_count = 0;

    // Check cache
    for (int i = 0; i < cache_count; ++i) {
        if (strcmp(cache[i].path, filename) == 0) {
            return cache[i].tex;
        }
    }


    int width, height, channels;
    stbi_set_flip_vertically_on_load(1);
    unsigned char* data = stbi_load(filename, &width, &height, &channels, 0);
    if (!data) {printf("\n Texture failed to load: %s\n", filename); return 0;}

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                 channels == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);

    // Add to cache if space
    if (cache_count < TEX_CACHE_SIZE) {
        strncpy(cache[cache_count].path, filename, 255);
        cache[cache_count].path[255] = '\0';
        cache[cache_count].tex = tex;
        ++cache_count;
    }

    return tex;
}