#include "shader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//empty and vacuous

static char* read_file(const char* filepath) {
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        printf("Failed to open file: %s\n", filepath);
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* buffer = malloc(size + 1);
    if (!buffer) {
        fclose(file);
        return NULL;
    }
    
    fread(buffer, 1, size, file);
    buffer[size] = '\0';
    fclose(file);
    
    return buffer;
}

bool shader_create_from_files(Shader* shader, const char* vertex_path, const char* fragment_path, 
                              VkDevice device, VkRenderPass render_pass) {
    if (!shader || !vertex_path || !fragment_path || !device || !render_pass) return false;
    

    shader->vertex_module = VK_NULL_HANDLE;
    shader->fragment_module = VK_NULL_HANDLE;
    shader->pipeline = VK_NULL_HANDLE;
    shader->pipeline_layout = VK_NULL_HANDLE;
    
    return true;
}

void shader_destroy(Shader* shader, VkDevice device) {
    if (!shader || !device) return;

}