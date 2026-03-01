#include "model.h"
#include <string.h>


#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

static uint32_t find_memory_type(uint32_t typeFilter,
                                 VkMemoryPropertyFlags properties,
                                 VkPhysicalDevice physical_device)
{
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memProps);

    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProps.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    return 0;
}

static void create_buffer(VkDevice device,
                          VkPhysicalDevice physical_device,
                          VkDeviceSize size,
                          VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags properties,
                          VkBuffer* buffer,
                          VkDeviceMemory* memory)
{
    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    vkCreateBuffer(device, &bufferInfo, NULL, buffer);

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device, *buffer, &memReq);

    VkMemoryAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memReq.size,
        .memoryTypeIndex = find_memory_type(
            memReq.memoryTypeBits,
            properties,
            physical_device)
    };

    vkAllocateMemory(device, &allocInfo, NULL, memory);
    vkBindBufferMemory(device, *buffer, *memory, 0);
}

static VkCommandBuffer begin_single_time_commands(VkDevice device, VkCommandPool command_pool)
{
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    VkCommandBuffer cmd;
    vkAllocateCommandBuffers(device, &allocInfo, &cmd);

    VkCommandBufferBeginInfo begin = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };
    vkBeginCommandBuffer(cmd, &begin);

    return cmd;
}

static void end_single_time_commands(VkDevice device,
                                     VkQueue graphics_queue,
                                     VkCommandPool command_pool,
                                     VkCommandBuffer cmd)
{
    vkEndCommandBuffer(cmd);

    VkSubmitInfo submit = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmd
    };

    vkQueueSubmit(graphics_queue, 1, &submit, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphics_queue);

    vkFreeCommandBuffers(device, command_pool, 1, &cmd);
}

static void copy_buffer(VkDevice device,
                        VkCommandPool command_pool,
                        VkQueue queue,
                        VkBuffer src,
                        VkBuffer dst,
                        VkDeviceSize size)
{
    VkCommandBuffer cmd = begin_single_time_commands(device, command_pool);

    VkBufferCopy region = { .size = size };
    vkCmdCopyBuffer(cmd, src, dst, 1, &region);

    end_single_time_commands(device, queue, command_pool, cmd);
}

static void create_gpu_buffer(VkDevice device,
                              VkPhysicalDevice physical_device,
                              VkCommandPool command_pool,
                              VkQueue graphics_queue,
                              const void* data,
                              VkDeviceSize size,
                              VkBufferUsageFlags usage,
                              VkBuffer* out_buf,
                              VkDeviceMemory* out_mem)
{
    VkBuffer staging_buf;
    VkDeviceMemory staging_mem;

    create_buffer(device, physical_device, size,
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  &staging_buf, &staging_mem);

    void* mapped;
    vkMapMemory(device, staging_mem, 0, size, 0, &mapped);
    memcpy(mapped, data, size);
    vkUnmapMemory(device, staging_mem);

    create_buffer(device, physical_device, size,
                  usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                  out_buf, out_mem);

    copy_buffer(device, command_pool, graphics_queue, staging_buf, *out_buf, size);

    vkDestroyBuffer(device, staging_buf, NULL);
    vkFreeMemory(device, staging_mem, NULL);
}

static void transition_image(VkDevice device,
                             VkCommandPool cmd_pool,
                             VkQueue queue,
                             VkImage image,
                             VkImageLayout old_layout,
                             VkImageLayout new_layout)
{
    VkCommandBuffer cmd = begin_single_time_commands(device, cmd_pool);

    VkImageMemoryBarrier barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.levelCount = 1,
        .subresourceRange.layerCount = 1
    };

    vkCmdPipelineBarrier(cmd,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                         0,
                         0, NULL,
                         0, NULL,
                         1, &barrier);

    end_single_time_commands(device, queue, cmd_pool, cmd);
}

static void copy_buffer_to_image(VkDevice device,
                                 VkCommandPool command_pool,
                                 VkQueue queue,
                                 VkBuffer buffer,
                                 VkImage image,
                                 uint32_t width,
                                 uint32_t height)
{
    VkCommandBuffer cmd = begin_single_time_commands(device, command_pool);

    VkBufferImageCopy region = {
        .imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .imageSubresource.layerCount = 1,
        .imageExtent.width = width,
        .imageExtent.height = height,
        .imageExtent.depth = 1
    };

    vkCmdCopyBufferToImage(cmd, buffer, image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &region);

    end_single_time_commands(device, queue, command_pool, cmd);
}

static void create_texture(const char* file,
                           VkDevice device,
                           VkPhysicalDevice physical_device,
                           VkCommandPool cmd_pool,
                           VkQueue queue,
                           VkImage* out_img,
                           VkDeviceMemory* out_mem,
                           VkImageView* out_view,
                           VkSampler* out_sampler)
{
    int texW, texH, texC;
    stbi_uc* pixels = stbi_load(file, &texW, &texH, &texC, STBI_rgb_alpha);

    VkDeviceSize size = texW * texH * 4;

    VkBuffer staging_buf;
    VkDeviceMemory staging_mem;
    create_buffer(device, physical_device, size,
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  &staging_buf, &staging_mem);

    void* mapped;
    vkMapMemory(device, staging_mem, 0, size, 0, &mapped);
    memcpy(mapped, pixels, size);
    vkUnmapMemory(device, staging_mem);
    stbi_image_free(pixels);

    VkImageCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .extent.width = texW,
        .extent.height = texH,
        .extent.depth = 1,
        .mipLevels = 1,
        .arrayLayers = 1,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                 VK_IMAGE_USAGE_SAMPLED_BIT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    vkCreateImage(device, &info, NULL, out_img);

    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(device, *out_img, &memReq);

    VkMemoryAllocateInfo alloc = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = memReq.size,
        .memoryTypeIndex = find_memory_type(
            memReq.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            physical_device)
    };

    vkAllocateMemory(device, &alloc, NULL, out_mem);
    vkBindImageMemory(device, *out_img, *out_mem, 0);

    transition_image(device, cmd_pool, queue,
                     *out_img,
                     VK_IMAGE_LAYOUT_UNDEFINED,
                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    copy_buffer_to_image(device, cmd_pool, queue,
                         staging_buf, *out_img,
                         texW, texH);

    transition_image(device, cmd_pool, queue,
                     *out_img,
                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device, staging_buf, NULL);
    vkFreeMemory(device, staging_mem, NULL);

    VkImageViewCreateInfo view = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = *out_img,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = VK_FORMAT_R8G8B8A8_SRGB,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.levelCount = 1,
        .subresourceRange.layerCount = 1
    };

    vkCreateImageView(device, &view, NULL, out_view);

    VkSamplerCreateInfo samp = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_LINEAR,
        .minFilter = VK_FILTER_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = 16,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
        .compareEnable = VK_FALSE,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR
    };

    vkCreateSampler(device, &samp, NULL, out_sampler);
}

bool model_create(Model* model,
                  const Vertex* vertices, uint32_t vertex_count,
                  const uint32_t* indices, uint32_t index_count,
                  const char* texture_path,
                  VkDevice device,
                  VkPhysicalDevice physical_device,
                  VkCommandPool cmd_pool,
                  VkQueue graphics_queue)
{
    model->mesh.vertex_count = vertex_count;
    model->mesh.index_count = index_count;
    model->mesh.has_indices = (indices != NULL);

    create_gpu_buffer(device, physical_device,
                      cmd_pool, graphics_queue,
                      vertices,
                      vertex_count * sizeof(Vertex),
                      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                      &model->mesh.vertex_buffer,
                      &model->mesh.vertex_memory);

    if (indices) {
        create_gpu_buffer(device, physical_device,
                          cmd_pool, graphics_queue,
                          indices,
                          index_count * sizeof(uint32_t),
                          VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                          &model->mesh.index_buffer,
                          &model->mesh.index_memory);
    }

    if (texture_path) {
        create_texture(texture_path,
                       device, physical_device,
                       cmd_pool, graphics_queue,
                       &model->mesh.texture_image,
                       &model->mesh.texture_memory,
                       &model->mesh.texture_view,
                       &model->mesh.texture_sampler);
    }

    model->offset = (Vec3){0,0,0};
    model->scale  = (Vec3){1,1,1};
    model->rotation = (Vec3){0,0,0};

    return true;
}

void model_record_draw_commands(const Model* model,
                                VkCommandBuffer cmd)
{
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd, 0, 1,
                           &model->mesh.vertex_buffer,
                           &offset);

    if (model->mesh.has_indices) {
        vkCmdBindIndexBuffer(cmd,
                             model->mesh.index_buffer,
                             0,
                             VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(cmd,
                         model->mesh.index_count,
                         1, 0, 0, 0);
    } else {
        vkCmdDraw(cmd,
                  model->mesh.vertex_count,
                  1, 0, 0);
    }
}

void model_destroy(Model* model, VkDevice device)
{
    vkDestroyBuffer(device, model->mesh.vertex_buffer, NULL);
    vkFreeMemory(device, model->mesh.vertex_memory, NULL);

    if (model->mesh.has_indices) {
        vkDestroyBuffer(device, model->mesh.index_buffer, NULL);
        vkFreeMemory(device, model->mesh.index_memory, NULL);
    }

    if (model->mesh.texture_sampler)
        vkDestroySampler(device, model->mesh.texture_sampler, NULL);

    if (model->mesh.texture_view)
        vkDestroyImageView(device, model->mesh.texture_view, NULL);

    if (model->mesh.texture_image)
        vkDestroyImage(device, model->mesh.texture_image, NULL);

    if (model->mesh.texture_memory)
        vkFreeMemory(device, model->mesh.texture_memory, NULL);
}