#ifndef EERIE_SHADER_H
#define EERIE_SHADER_H

#include <stdbool.h>
#include <vulkan/vulkan.h>
#include <SDL3/SDL_vulkan.h>
#include "engine_math.h"

typedef struct {
    VkShaderModule vertex_module;
    VkShaderModule fragment_module;
    VkPipeline pipeline;
    VkPipelineLayout pipeline_layout;
} Shader;

// Shader functions
bool shader_create_from_files(Shader* shader, const char* vertex_path, const char* fragment_path, 
                              VkDevice device, VkRenderPass render_pass);
void shader_destroy(Shader* shader, VkDevice device);

#endif // EERIE_SHADER_H