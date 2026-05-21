#include "render.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


void render_entity(Entity* ent, Shader* shader, Vec3 color) {
    Model* model = &ent->model;
    shader_set_int(shader, "useLighting", model->use_lighting ? 1 : 0);
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
    Mat4 rot_x = mat4_rotate_x(model->rotation.x);
    Mat4 rot_y = mat4_rotate_y(model->rotation.y);
    Mat4 rot_z = mat4_rotate_z(model->rotation.z);
    Mat4 rotation = mat4_multiply(mat4_multiply(rot_z, rot_y), rot_x);
    Mat4 new_model = mat4_multiply(mat4_multiply(scale, rotation), offset);
    shader_set_mat4(shader, "model", &new_model);
    shader_set_vec3(shader, "objectColor", &color);
    model_draw(model);

}

void render_model(Model* model, Vec3 pos, Shader* shader, Vec3 color) {
    shader_set_int(shader, "useLighting", model->use_lighting ? 1 : 0);
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
    Mat4 rot_x = mat4_rotate_x(model->rotation.x);
    Mat4 rot_y = mat4_rotate_y(model->rotation.y);
    Mat4 rot_z = mat4_rotate_z(model->rotation.z);
    Mat4 rotation = mat4_multiply(mat4_multiply(rot_z, rot_y), rot_x);
    Mat4 new_model = mat4_multiply(mat4_multiply(scale, rotation), offset);
    shader_set_mat4(shader, "model", &new_model);
    shader_set_vec3(shader, "objectColor", &color);
    model_draw(model);

}


void render_model_static(Model* model, const Camera* camera, Vec3 view_offset, Shader* shader, Vec3 color) {
    if (!model || !shader || !camera || !camera->position) {
        return;
    }

    shader_set_int(shader, "useLighting", model->use_lighting ? 1 : 0);
    if (model->mesh.texture != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, model->mesh.texture);
        shader_set_int(shader, "texSampler", 0); // set sampler2D uniform to 0
        shader_set_int(shader, "useTexture", 1);
    } else {
        shader_set_int(shader, "useTexture", 0);
    }

    GLboolean depth_was_enabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean depth_mask_was_enabled = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depth_mask_was_enabled);

    GLint prev_program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prev_program);
    GLint view_loc = -1;
    GLint proj_loc = -1;
    GLfloat prev_view[16] = {0};
    GLfloat prev_proj[16] = {0};
    if (prev_program != 0) {
        view_loc = glGetUniformLocation((GLuint)prev_program, "view");
        if (view_loc != -1) {
            glGetUniformfv((GLuint)prev_program, view_loc, prev_view);
        }

        proj_loc = glGetUniformLocation((GLuint)prev_program, "projection");
        if (proj_loc != -1) {
            glGetUniformfv((GLuint)prev_program, proj_loc, prev_proj);
        }
    }

    // Viewmodel pass: clear world depth so only the viewmodel depth-tests against itself.
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glClear(GL_DEPTH_BUFFER_BIT);

    Mat4 identity = mat4_identity();
    shader_set_mat4(shader, "view", &identity);

    const float viewmodel_fov = 60.0f; 
    float aspect = (camera->aspect > 0.0f) ? camera->aspect : (16.0f / 9.0f);
    Mat4 viewmodel_projection = mat4_perspective(viewmodel_fov, aspect, camera->near_plane, camera->far_plane);
    shader_set_mat4(shader, "projection", &viewmodel_projection);

    Mat4 offset = mat4_translate(vec3_add(&model->offset, &view_offset));
    Mat4 scale = mat4_scale(model->scale);
    Mat4 rot_x = mat4_rotate_x(model->rotation.x);
    Mat4 rot_y = mat4_rotate_y(model->rotation.y);
    Mat4 rot_z = mat4_rotate_z(model->rotation.z);
    Mat4 rotation = mat4_multiply(mat4_multiply(rot_z, rot_y), rot_x);
    Mat4 new_model = mat4_multiply(mat4_multiply(scale, rotation), offset);
    shader_set_mat4(shader, "model", &new_model);
    shader_set_vec3(shader, "objectColor", &color);
    model_draw(model);

    if (view_loc != -1) {
        glUniformMatrix4fv(view_loc, 1, GL_FALSE, prev_view);
    }
    if (proj_loc != -1) {
        glUniformMatrix4fv(proj_loc, 1, GL_FALSE, prev_proj);
    }

    glDepthMask(depth_mask_was_enabled);
    if (!depth_was_enabled) {
        glDisable(GL_DEPTH_TEST);
    }
}

void render_line(const Vec3 start, const Vec3 end) {
    GLint current_program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &current_program);
    if (current_program == 0) {
        return;
    }

    GLint model_loc = glGetUniformLocation((GLuint)current_program, "model");
    GLint color_loc = glGetUniformLocation((GLuint)current_program, "objectColor");
    GLint use_tex_loc = glGetUniformLocation((GLuint)current_program, "useTexture");
    GLint use_lighting_loc = glGetUniformLocation((GLuint)current_program, "useLighting");

    GLfloat previous_model[16] = {0};
    if (model_loc != -1) {
        glGetUniformfv((GLuint)current_program, model_loc, previous_model);
    }

    if (use_tex_loc != -1) {
        glUniform1i(use_tex_loc, 0);
    }
    if (use_lighting_loc != -1) {
        glUniform1i(use_lighting_loc, 0);
    }
    if (color_loc != -1) {
        glUniform3f(color_loc, 1.0f, 1.0f, 1.0f);
    }

    if (model_loc != -1) {
        const GLfloat identity[16] = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
        glUniformMatrix4fv(model_loc, 1, GL_FALSE, identity);
    }

    const GLfloat line_vertices[6] = {
        start.x, start.y, start.z,
        end.x, end.y, end.z
    };

    GLuint vao = 0;
    GLuint vbo = 0;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(line_vertices), line_vertices, GL_STREAM_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);

    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, 2);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    if (model_loc != -1) {
        glUniformMatrix4fv(model_loc, 1, GL_FALSE, previous_model);
    }
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