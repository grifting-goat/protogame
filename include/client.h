#ifndef CLIENT_H
#define CLIENT_H

#include <stdbool.h>
#include "window.h"
#include "level.h"
#include "player.h"
#include "camera.h"


static const char *requiredDeviceExtensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};
static const uint32_t requiredDeviceExtensionCount = 1;

typedef struct {
    Window win;
    Level level;
    
    Player player;
    Camera player_camera;
    InputHandle player_input;

    VkInstance instance;

    VkSurfaceKHR surface;

    VkDevice device;
    VkPhysicalDevice physical_device;
    VkCommandPool command_pool;
    VkQueue graphics_queue;
    uint32_t graphics_queue_family_index;

    VkSwapchainKHR     swap_chain;
    VkExtent2D         swap_chain_extent;
    VkSurfaceFormatKHR swap_chain_surface_format;
    VkImage           *swap_chain_images;
    uint32_t           swap_chain_image_count;
    VkImageView *swap_chain_image_views;

    VkPipelineLayout pipeline_layout;
    VkPipeline       graphics_pipeline;

    VkCommandBuffer command_buffer;

    VkSemaphore present_complete_semaphore;
    VkSemaphore render_finished_semaphore;
    VkFence     draw_fence;

} Client;

bool client_startup(Client* client);

bool client_run(Client* client);

void client_close(Client* client);

#endif // CLIENT_H