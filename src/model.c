#include "model.h"


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
    
    // Normal (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Texture coordinates (location 2)
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