#include "model.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MODEL_LOADER_INIT_CAP 1024


Model model_create_empty(void) {
    Model mdl = {0};
    mdl.offset = (Vec3){0.0f, 0.0f, 0.0f};
    mdl.scale = (Vec3){1.0f, 1.0f, 1.0f};
    mdl.rotation = (Vec3){0.0f, 0.0f, 0.0f};
    mdl.use_lighting = true;
    return mdl;
}


bool model_create(Model* model, const Vertex* vertices, uint32_t vertex_count, const uint32_t* indices, uint32_t index_count, const char* texture_path) {
    if (!model || !vertices) return false;
    
    model->mesh.vertex_count = vertex_count;
    model->mesh.index_count = index_count;
    model->mesh.has_indices = (indices != NULL);
    
    // Generate and bind VAO
    glGenVertexArrays(1, &model->mesh.vao);
    glBindVertexArray(model->mesh.vao);
    
    // Generate and bind VBO
    glGenBuffers(1, &model->mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, model->mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertex_count * sizeof(Vertex), vertices, GL_STATIC_DRAW);
    
    // Set vertex attributes
    // Position (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    
    //Normal (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    //Texture coordinates (location 2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    
    // Handle indices if provided
    if (model->mesh.has_indices) {
        glGenBuffers(1, &model->mesh.ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->mesh.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
    }
    
    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    //texture
    if (texture_path != NULL) {
        model->mesh.texture = load_texture(texture_path);
    }
    else {
        model->mesh.texture = 0;
    }
    

    //set to defaults
    model->offset = (Vec3){0.0f,0.0f,0.0f};
    model->scale = (Vec3){1.0f,1.0f,1.0f};
    model->rotation = (Vec3){0.0f,0.0f,0.0f};
    model->use_lighting = true;
    
    return true;
}

void model_draw(const Model* model) {
    if (!model) return;
    
    glBindVertexArray(model->mesh.vao);
    
    if (model->mesh.has_indices) {
        glDrawElements(GL_TRIANGLES, model->mesh.index_count, GL_UNSIGNED_INT, 0);
    } else {
        glDrawArrays(GL_TRIANGLES, 0, model->mesh.vertex_count);
    }
    
    glBindVertexArray(0);
}

void model_destroy(Model* model) {
    if (!model) return;
    
    if (model->mesh.has_indices) {
        glDeleteBuffers(1, &model->mesh.ebo);
    }
    glDeleteBuffers(1, &model->mesh.vbo);
    glDeleteVertexArrays(1, &model->mesh.vao);
    
    model->mesh.vao = model->mesh.vbo = model->mesh.ebo = 0;
    model->mesh.vertex_count = model->mesh.index_count = 0;
    model->mesh.has_indices = false;
}

//painful
Model model_load(const char* obj_path, const char* tex_path) {
    FILE *fp = fopen(obj_path, "r");
    if (!fp) {
        return model_create_empty();
    }

    Model mdl = model_create_empty();

    uint32_t pos_cap = MODEL_LOADER_INIT_CAP;
    uint32_t vt_cap = MODEL_LOADER_INIT_CAP;
    uint32_t vn_cap = MODEL_LOADER_INIT_CAP;
    uint32_t out_cap = MODEL_LOADER_INIT_CAP;

    float (*positions)[3] = calloc(pos_cap, sizeof(*positions));
    float (*texcoords)[2] = calloc(vt_cap, sizeof(*texcoords));
    float (*normals)[3] = calloc(vn_cap, sizeof(*normals));
    Vertex* vert = calloc(out_cap, sizeof(*vert));

    if (!positions || !texcoords || !normals || !vert) {
        free(positions);
        free(texcoords);
        free(normals);
        free(vert);
        fclose(fp);
        return mdl;
    }

    uint32_t pos_count = 0;
    uint32_t vt_count = 0;
    uint32_t vn_count = 0;
    uint32_t out_vert_count = 0;
    bool load_failed = false;

    size_t line_cap = 256;
    char* line = malloc(line_cap);
    if (!line) {
        free(positions);
        free(texcoords);
        free(normals);
        free(vert);
        fclose(fp);
        return mdl;
    }

    while (1) {
        size_t line_len = 0;
        int c = 0;

        while ((c = fgetc(fp)) != EOF && c != '\n') {
            if (c == '\r') {
                continue;
            }

            if (line_len + 1 >= line_cap) {
                size_t new_cap = line_cap * 2;
                char* new_line = realloc(line, new_cap);
                if (!new_line) {
                    load_failed = true;
                    break;
                }
                line = new_line;
                line_cap = new_cap;
            }

            line[line_len++] = (char)c;
        }

        if (load_failed) {
            break;
        }

        if (c == EOF && line_len == 0) {
            break;
        }

        line[line_len] = '\0';

        if (line[0] == 'v' && line[1] == ' ') { //get vertex positions
            if (pos_count >= pos_cap) {
                uint32_t new_cap = pos_cap * 2;
                float (*new_positions)[3] = realloc(positions, (size_t)new_cap * sizeof(*positions));
                if (!new_positions) {
                    load_failed = true;
                    break;
                }
                positions = new_positions;
                memset(&positions[pos_cap], 0, (size_t)(new_cap - pos_cap) * sizeof(*positions));
                pos_cap = new_cap;
            }
            sscanf(line + 2, "%f %f %f", &positions[pos_count][0], &positions[pos_count][1], &positions[pos_count][2]);
            pos_count++;
            continue;
        }

        if (line[0] == 'v' && line[1] == 't' && line[2] == ' ') { //get vertex uv
            if (vt_count >= vt_cap) {
                uint32_t new_cap = vt_cap * 2;
                float (*new_texcoords)[2] = realloc(texcoords, (size_t)new_cap * sizeof(*texcoords));
                if (!new_texcoords) {
                    load_failed = true;
                    break;
                }
                texcoords = new_texcoords;
                memset(&texcoords[vt_cap], 0, (size_t)(new_cap - vt_cap) * sizeof(*texcoords));
                vt_cap = new_cap;
            }
            sscanf(line + 3, "%f %f", &texcoords[vt_count][0], &texcoords[vt_count][1]);
            vt_count++;
            continue;
        }

        if (line[0] == 'v' && line[1] == 'n' && line[2] == ' ') { //get vertex normals
            if (vn_count >= vn_cap) {
                uint32_t new_cap = vn_cap * 2;
                float (*new_normals)[3] = realloc(normals, (size_t)new_cap * sizeof(*normals));
                if (!new_normals) {
                    load_failed = true;
                    break;
                }
                normals = new_normals;
                memset(&normals[vn_cap], 0, (size_t)(new_cap - vn_cap) * sizeof(*normals));
                vn_cap = new_cap;
            }
            sscanf(line + 3, "%f %f %f", &normals[vn_count][0], &normals[vn_count][1], &normals[vn_count][2]);
            vn_count++;
            continue;
        }

        if (line[0] == 'f' && line[1] == ' ') { //get face indexes
            int vi[3] = {0}, ti[3] = {0}, ni[3] = {0};
            int parsed = sscanf(
                line + 2,
                "%d/%d/%d %d/%d/%d %d/%d/%d",
                &vi[0], &ti[0], &ni[0],
                &vi[1], &ti[1], &ni[1],
                &vi[2], &ti[2], &ni[2]
            );

            if (parsed != 9) {
                parsed = sscanf(
                    line + 2,
                    "%d//%d %d//%d %d//%d",
                    &vi[0], &ni[0],
                    &vi[1], &ni[1],
                    &vi[2], &ni[2]
                );
                if (parsed == 6) {
                    ti[0] = ti[1] = ti[2] = 0;
                } else {
                    continue;
                }
            }

            for (int k = 0; k < 3; k++) {
                if (out_vert_count >= out_cap) {
                    uint32_t new_cap = out_cap * 2;
                    Vertex* new_vert = realloc(vert, (size_t)new_cap * sizeof(*vert));
                    if (!new_vert) {
                        load_failed = true;
                        break;
                    }
                    vert = new_vert;
                    memset(&vert[out_cap], 0, (size_t)(new_cap - out_cap) * sizeof(*vert));
                    out_cap = new_cap;
                }

                if (load_failed) {
                    break;
                }

                int p_idx = vi[k] - 1;
                int t_idx = ti[k] - 1;
                int n_idx = ni[k] - 1;

                if (p_idx < 0 || (uint32_t)p_idx >= pos_count) {
                    continue;
                }

                vert[out_vert_count].position[0] = positions[p_idx][0];
                vert[out_vert_count].position[1] = positions[p_idx][1];
                vert[out_vert_count].position[2] = positions[p_idx][2];

                if (t_idx >= 0 && (uint32_t)t_idx < vt_count) {
                    vert[out_vert_count].texcoord[0] = texcoords[t_idx][0];
                    vert[out_vert_count].texcoord[1] = texcoords[t_idx][1];
                }

                if (n_idx >= 0 && (uint32_t)n_idx < vn_count) {
                    vert[out_vert_count].normal[0] = normals[n_idx][0];
                    vert[out_vert_count].normal[1] = normals[n_idx][1];
                    vert[out_vert_count].normal[2] = normals[n_idx][2];
                }

                out_vert_count++;
            }

            if (load_failed) {
                break;
            }
        }

        if (load_failed) {
            break;
        }
    }

    fclose(fp);

    if (load_failed) {
        free(line);
        free(positions);
        free(texcoords);
        free(normals);
        free(vert);
        return model_create_empty();
    }

    if (out_vert_count == 0) {
        free(line);
        free(positions);
        free(texcoords);
        free(normals);
        free(vert);
        return mdl;
    }

    if (!model_create(&mdl, vert, out_vert_count, NULL, 0, tex_path)) {
        free(line);
        free(positions);
        free(texcoords);
        free(normals);
        free(vert);
        return model_create_empty();
    }

    free(line);
    free(positions);
    free(texcoords);
    free(normals);
    free(vert);

    return mdl;
}