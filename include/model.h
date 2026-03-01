#ifndef MODEL_H
#define MODEL_H

#include <vulkan/vulkan.h>
#include <stdint.h>
#include <stdbool.h>

#include "engine_math.h"

typedef struct {
    float position[3];
    float normal[3];
    float texcoord[2];
} Vertex;

typedef struct {
    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_memory;

    VkBuffer index_buffer;
    VkDeviceMemory index_memory;

    uint32_t vertex_count;
    uint32_t index_count;
    bool has_indices;

    VkImage texture_image;
    VkDeviceMemory texture_memory;
    VkImageView texture_view;
    VkSampler texture_sampler;
} Mesh;

typedef struct {
    Mesh mesh;
    Vec3 offset;
    Vec3 scale;
    Vec3 rotation;
} Model;

bool model_create(
    Model* model,
    const Vertex* vertices, uint32_t vertex_count,
    const uint32_t* indices, uint32_t index_count,
    const char* texture_path,
    VkDevice device,
    VkPhysicalDevice physical_device,
    VkCommandPool command_pool,
    VkQueue graphics_queue
);

void model_record_draw_commands(
    const Model* model,
    VkCommandBuffer cmd,
    VkPipelineLayout pipeline_layout
);

void model_destroy(Model* model, VkDevice device);

#endif