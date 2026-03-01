#include "client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void vulkan_setup(Client* client);
void createInstance(Client *client);
void createSurface(Client *client);
void pickPhysicalDevice(Client *client);
void createLogicalDevice(Client *client);
void createSwapChain(Client *client);
void createImageViews(Client *client);
void createGraphicsPipeline(Client *client);
void createCommandPool(Client *client);
void createCommandBuffer(Client *client);
void createSyncObjects(Client *client);
void cleanup(Client *client);
uint32_t *readFile(const char *filename, size_t *fileSize);


bool client_startup(Client* client) {
    if (!client) return false;

    level_create(&client->level, 128);

    if (!window_init()) {return 0;}

    if (!window_create(&client->win, "Proto Game", 1920, 1080, true)) {window_quit(); return 0;}
    window_set_icon(&client->win, "icon.bmp");

    input_init(&client->player_input, &client->win);

    
    vulkan_setup(client);

    /* Initialize player and camera */
    client->player = player_create();
    camera_init(&client->player_camera);
    camera_attach(&client->player_camera, &client->player.entity.position, &client->player_camera.offset_vector);
    
    client->player.entity.model = temp_create_sphere(32, 16, 1.0f, client->device, client->physical_device, client->command_pool, client->graphics_queue);
    client->player_camera.mode = !client->player_camera.mode;
    client->player.entity.position.y = 0.0f;
    

    return 1;
}


bool client_run(Client* client) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {if (event.type == SDL_EVENT_QUIT) {return 0;}}

    //timing
    Uint64 now = SDL_GetPerformanceCounter();
    Uint64 frame_ticks = now - client->level.last_time;
    float dt = (float)frame_ticks / (float)client->level.perf_freq;
    client->level.last_time = now;

    dMouse delta_mouse = input_mouse(&client->player_input);
    float dx = delta_mouse.x;
    float dy = delta_mouse.y;
    Uint32 mouse_buttons = delta_mouse.mb;

    uint32_t mouse_sensitivity = 0.07f;

    client->player_camera.angles.x -= dx * mouse_sensitivity; // yaw
    client->player_camera.angles.y -= dy * mouse_sensitivity; // pitch


    // Clamp pitch to avoid flipping
    if (client->player_camera.angles.y > 1.5f) client->player_camera.angles.y = 1.5f;
    if (client->player_camera.angles.y < -1.5f) client->player_camera.angles.y = -1.5f;


    //printing fps move to hud controler later
    client->level.fps_time_accum += frame_ticks;
    client->level.frame_count++;
        if (client->level.fps_time_accum >= client->level.perf_freq) {
            double fps = (double)client->level.frame_count * (double)client->level.perf_freq / (double)client->level.fps_time_accum;
            client->level.fps_time_accum = 0;
            client->level.frame_count = 0;
        }

    level_update(&client->level, dt);
    

    //render_frame(client);

    return 1;
}


void client_close(Client* client) {
    if (!client) return;

    cleanup(client);

    for (uint32_t i = 0; i < client->swap_chain_image_count; ++i) {
        vkDestroyImageView(client->device, client->swap_chain_image_views[i], NULL);
    }

    free(client->swap_chain_image_views);


    vkDestroySemaphore(client->device, client->present_complete_semaphore, NULL);
    vkDestroySemaphore(client->device, client->render_finished_semaphore, NULL);
    vkDestroyFence(client->device, client->draw_fence, NULL);


    window_destroy(&client->win);
    window_quit();
    level_destroy(&client->level);
}


void vulkan_setup(Client* client) {
     createInstance(client);
     createSurface(client);
     pickPhysicalDevice(client);
     createLogicalDevice(client);
     createSwapChain(client);
     createImageViews(client);
     createGraphicsPipeline(client);
     createCommandPool(client);
     createCommandBuffer(client);
     createSyncObjects(client);
}


void createInstance(Client *client) {


    VkApplicationInfo appInfo = {
        .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext              = NULL,
        .pApplicationName   = "Proto Game",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName        = "No Engine",
        .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion         = VK_API_VERSION_1_4
    };

    uint32_t sdlExtensionCount = 0;
    const char * const *sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);

    VkExtensionProperties *extensionProperties = malloc(extensionCount * sizeof(VkExtensionProperties));
    if (!extensionProperties)
    {
        fprintf(stderr, "Failed to allocate memory for extension properties\n");
        exit(EXIT_FAILURE);
    }
    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensionProperties);

    for (uint32_t i = 0; i < sdlExtensionCount; ++i)
    {
        bool found = false;
        for (uint32_t j = 0; j < extensionCount; ++j)
        {
            if (strcmp(extensionProperties[j].extensionName, sdlExtensions[i]) == 0)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            fprintf(stderr, "Required SDL extension not supported: %s\n", sdlExtensions[i]);
            free(extensionProperties);
            exit(EXIT_FAILURE);
        }
    }

    free(extensionProperties);

    VkInstanceCreateInfo createInfo = {
        .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext                   = NULL,
        .flags                   = 0,
        .pApplicationInfo        = &appInfo,
        .enabledLayerCount       = 0,
        .ppEnabledLayerNames     = NULL,
        .enabledExtensionCount   = sdlExtensionCount,
        .ppEnabledExtensionNames = sdlExtensions
    };

    VkResult result = vkCreateInstance(&createInfo, NULL, &client->instance);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create Vulkan instance: %d\n", result);
        exit(EXIT_FAILURE);
    }

}

void createSurface(Client *client)
{
    if (!SDL_Vulkan_CreateSurface(client->win.window, client->instance, NULL, &client->surface))
    {
        fprintf(stderr, "Failed to create window surface: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }
}

bool isDeviceSuitable(VkPhysicalDevice physicalDevice) {

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    /* Check Vulkan 1.3 support */
    bool supportsVulkan1_3 = properties.apiVersion >= VK_API_VERSION_1_3;

    /* Check if discrete*/
    bool isDiscrete = properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

    /* Check graphics queue family support */
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);

    VkQueueFamilyProperties *queueFamilies = malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies);

    bool supportsGraphics = false;
    for (uint32_t i = 0; i < queueFamilyCount; ++i)
    {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            supportsGraphics = true;
            break;
        }
    }
    free(queueFamilies);

    /* Check required device extensions */
    uint32_t availableExtensionCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &availableExtensionCount, NULL);

    VkExtensionProperties *availableExtensions = malloc(availableExtensionCount * sizeof(VkExtensionProperties));
    vkEnumerateDeviceExtensionProperties(physicalDevice, NULL, &availableExtensionCount, availableExtensions);

    bool supportsAllRequiredExtensions = true;
    for (uint32_t i = 0; i < requiredDeviceExtensionCount; ++i)
    {
        bool found = false;
        for (uint32_t j = 0; j < availableExtensionCount; ++j)
        {
            if (strcmp(availableExtensions[j].extensionName, requiredDeviceExtensions[i]) == 0)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            supportsAllRequiredExtensions = false;
            break;
        }
    }
    free(availableExtensions);

    /* Check required features via chained pNext structs */
    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeatures = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT,
        .pNext = NULL
    };
    VkPhysicalDeviceVulkan13Features vulkan13Features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &extendedDynamicStateFeatures
    };
    VkPhysicalDeviceFeatures2 features2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &vulkan13Features
    };
    vkGetPhysicalDeviceFeatures2(physicalDevice, &features2);

    bool supportsRequiredFeatures = vulkan13Features.dynamicRendering &&
                                    extendedDynamicStateFeatures.extendedDynamicState;

    return supportsVulkan1_3 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures;
}


void pickPhysicalDevice(Client* client)
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(client->instance, &deviceCount, NULL);

    if (deviceCount == 0)
    {
        fprintf(stderr, "Failed to find any GPU with Vulkan support!\n");
        exit(EXIT_FAILURE);
    }

    VkPhysicalDevice *devices = malloc(deviceCount * sizeof(VkPhysicalDevice));
    if (!devices)
    {
        fprintf(stderr, "Failed to allocate memory for physical devices\n");
        exit(EXIT_FAILURE);
    }
    vkEnumeratePhysicalDevices(client->instance, &deviceCount, devices);

    client->physical_device = VK_NULL_HANDLE;
    for (uint32_t i = 0; i < deviceCount; ++i)
    {
        if (isDeviceSuitable(devices[i]))
        {
            client->physical_device = devices[i];
            break;
        }
    }

    free(devices);

    if (client->physical_device == VK_NULL_HANDLE)
    {
        fprintf(stderr, "Failed to find a suitable GPU!\n");
        exit(EXIT_FAILURE);
    }
}

void createLogicalDevice(Client* client)
{
    /* Find first queue family that supports graphics */
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(client->physical_device, &queueFamilyCount, NULL);

    VkQueueFamilyProperties *queueFamilies = malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(client->physical_device, &queueFamilyCount, queueFamilies);

    uint32_t graphicsIndex = UINT32_MAX;
    for (uint32_t i = 0; i < queueFamilyCount; ++i)
    {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            graphicsIndex = i;
            break;
        }
    }
    free(queueFamilies);

    assert(graphicsIndex != UINT32_MAX && "No graphics queue family found!");

    /* Build feature chain */
    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeatures = {
        .sType                = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT,
        .pNext                = NULL,
        .extendedDynamicState = VK_TRUE
    };
    VkPhysicalDeviceVulkan13Features vulkan13Features = {
        .sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext            = &extendedDynamicStateFeatures,
        .dynamicRendering = VK_TRUE
    };
    VkPhysicalDeviceFeatures2 features2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &vulkan13Features
    };

    /* Create logical device */
    float queuePriority = 0.5f;
    VkDeviceQueueCreateInfo deviceQueueCreateInfo = {
        .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .pNext            = NULL,
        .queueFamilyIndex = graphicsIndex,
        .queueCount       = 1,
        .pQueuePriorities = &queuePriority
    };

    VkDeviceCreateInfo deviceCreateInfo = {
        .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext                   = &features2,
        .queueCreateInfoCount    = 1,
        .pQueueCreateInfos       = &deviceQueueCreateInfo,
        .enabledExtensionCount   = requiredDeviceExtensionCount,
        .ppEnabledExtensionNames = requiredDeviceExtensions
    };

    VkResult result = vkCreateDevice(client->physical_device, &deviceCreateInfo, NULL, &client->device);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create logical device: %d\n", result);
        exit(EXIT_FAILURE);
    }

    /* Retrieve the graphics queue and store the family index */
    vkGetDeviceQueue(client->device, graphicsIndex, 0, &client->graphics_queue);
    client->graphics_queue_family_index = graphicsIndex;
}


uint32_t chooseSwapMinImageCount(const VkSurfaceCapabilitiesKHR *capabilities)
{
    uint32_t minImageCount = capabilities->minImageCount > 3 ? capabilities->minImageCount : 3;

    if (capabilities->maxImageCount > 0 && capabilities->maxImageCount < minImageCount)
    {
        minImageCount = capabilities->maxImageCount;
    }

    return minImageCount;
}


VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR *capabilities, SDL_Window *window) {
    if (capabilities->currentExtent.width != UINT32_MAX)
    {
        return capabilities->currentExtent;
    }

    int width, height;
    SDL_GetWindowSizeInPixels(window, &width, &height);

    VkExtent2D extent = {
        .width  = (uint32_t)width  < capabilities->minImageExtent.width  ? capabilities->minImageExtent.width  :
                  (uint32_t)width  > capabilities->maxImageExtent.width  ? capabilities->maxImageExtent.width  : (uint32_t)width,
        .height = (uint32_t)height < capabilities->minImageExtent.height ? capabilities->minImageExtent.height :
                  (uint32_t)height > capabilities->maxImageExtent.height ? capabilities->maxImageExtent.height : (uint32_t)height
    };

    return extent;
}

VkPresentModeKHR chooseSwapPresentMode(const VkPresentModeKHR *availablePresentModes, uint32_t presentModeCount)
{
    bool fifoSupported    = false;
    bool mailboxSupported = false;

    for (uint32_t i = 0; i < presentModeCount; ++i)
    {
        if (availablePresentModes[i] == VK_PRESENT_MODE_FIFO_KHR)
            fifoSupported = true;
        if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
            mailboxSupported = true;
    }

    assert(fifoSupported && "FIFO present mode not supported!");

    return mailboxSupported ? VK_PRESENT_MODE_MAILBOX_KHR : VK_PRESENT_MODE_FIFO_KHR;
}


VkSurfaceFormatKHR chooseSwapSurfaceFormat(const VkSurfaceFormatKHR *availableFormats, uint32_t formatCount)
{
    assert(formatCount > 0);

    for (uint32_t i = 0; i < formatCount; ++i)
    {
        if (availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormats[i];
        }
    }

    return availableFormats[0];
}

void createImageViews(Client *client)
{
    assert(client->swap_chain_image_count > 0);

    client->swap_chain_image_views = malloc(client->swap_chain_image_count * sizeof(VkImageView));

    for (uint32_t i = 0; i < client->swap_chain_image_count; ++i)
    {
        VkImageViewCreateInfo imageViewCreateInfo = {
            .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext    = NULL,
            .image    = client->swap_chain_images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format   = client->swap_chain_surface_format.format,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY
            },
            .subresourceRange = {
                .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel   = 0,
                .levelCount     = 1,
                .baseArrayLayer = 0,
                .layerCount     = 1
            }
        };

        VkResult result = vkCreateImageView(client->device, &imageViewCreateInfo, NULL, &client->swap_chain_image_views[i]);
        if (result != VK_SUCCESS)
        {
            fprintf(stderr, "Failed to create image view %u: %d\n", i, result);
            exit(EXIT_FAILURE);
        }
    }
}



void createSwapChain(Client *client)
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(client->physical_device, client->surface, &surfaceCapabilities);

    client->swap_chain_extent = chooseSwapExtent(&surfaceCapabilities, client->win.window);
    uint32_t minImageCount    = chooseSwapMinImageCount(&surfaceCapabilities);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(client->physical_device, client->surface, &formatCount, NULL);
    VkSurfaceFormatKHR *availableFormats = malloc(formatCount * sizeof(VkSurfaceFormatKHR));
    vkGetPhysicalDeviceSurfaceFormatsKHR(client->physical_device, client->surface, &formatCount, availableFormats);
    client->swap_chain_surface_format = chooseSwapSurfaceFormat(availableFormats, formatCount);
    free(availableFormats);

    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(client->physical_device, client->surface, &presentModeCount, NULL);
    VkPresentModeKHR *availablePresentModes = malloc(presentModeCount * sizeof(VkPresentModeKHR));
    vkGetPhysicalDeviceSurfacePresentModesKHR(client->physical_device, client->surface, &presentModeCount, availablePresentModes);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(availablePresentModes, presentModeCount);
    free(availablePresentModes);

    VkSwapchainCreateInfoKHR swapChainCreateInfo = {
        .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .pNext            = NULL,
        .surface          = client->surface,
        .minImageCount    = minImageCount,
        .imageFormat      = client->swap_chain_surface_format.format,
        .imageColorSpace  = client->swap_chain_surface_format.colorSpace,
        .imageExtent      = client->swap_chain_extent,
        .imageArrayLayers = 1,
        .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform     = surfaceCapabilities.currentTransform,
        .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode      = presentMode,
        .clipped          = VK_TRUE,
        .oldSwapchain     = VK_NULL_HANDLE
    };

    VkResult result = vkCreateSwapchainKHR(client->device, &swapChainCreateInfo, NULL, &client->swap_chain);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create swapchain: %d\n", result);
        exit(EXIT_FAILURE);
    }

    vkGetSwapchainImagesKHR(client->device, client->swap_chain, &client->swap_chain_image_count, NULL);
    client->swap_chain_images = malloc(client->swap_chain_image_count * sizeof(VkImage));
    vkGetSwapchainImagesKHR(client->device, client->swap_chain, &client->swap_chain_image_count, client->swap_chain_images);
}


VkShaderModule createShaderModule(Client *client, const uint32_t *code, size_t codeSize)
{
    VkShaderModuleCreateInfo createInfo = {
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext    = NULL,
        .codeSize = codeSize,
        .pCode    = code
    };

    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(client->device, &createInfo, NULL, &shaderModule);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create shader module: %d\n", result);
        exit(EXIT_FAILURE);
    }

    return shaderModule;
}

void createGraphicsPipeline(Client *client)
{
    /* Load and create shader module */
    size_t    codeSize = 0;
    uint32_t *code     = readFile("shaders/slang.spv", &codeSize);
    VkShaderModule shaderModule = createShaderModule(client, code, codeSize);
    free(code);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext  = NULL,
        .stage  = VK_SHADER_STAGE_VERTEX_BIT,
        .module = shaderModule,
        .pName  = "vertMain"
    };

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext  = NULL,
        .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = shaderModule,
        .pName  = "fragMain"
    };

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    /* Vertex input */
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext                           = NULL,
        .vertexBindingDescriptionCount   = 0,
        .pVertexBindingDescriptions      = NULL,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions    = NULL
    };

    /* Input assembly */
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext                  = NULL,
        .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    /* Viewport state */
    VkPipelineViewportStateCreateInfo viewportState = {
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext         = NULL,
        .viewportCount = 1,
        .scissorCount  = 1
    };

    /* Rasterizer */
    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext                   = NULL,
        .depthClampEnable        = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode             = VK_POLYGON_MODE_FILL,
        .cullMode                = VK_CULL_MODE_BACK_BIT,
        .frontFace               = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable         = VK_FALSE,
        .depthBiasSlopeFactor    = 1.0f,
        .lineWidth               = 1.0f
    };

    /* Multisampling */
    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext                = NULL,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable  = VK_FALSE
    };

    /* Color blend attachment */
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .blendEnable    = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    /* Color blending */
    VkPipelineColorBlendStateCreateInfo colorBlending = {
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext           = NULL,
        .logicOpEnable   = VK_FALSE,
        .logicOp         = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments    = &colorBlendAttachment
    };

    /* Dynamic state */
    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext             = NULL,
        .dynamicStateCount = 2,
        .pDynamicStates    = dynamicStates
    };

    /* Pipeline layout */
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext                  = NULL,
        .setLayoutCount         = 0,
        .pSetLayouts            = NULL,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges    = NULL
    };

    VkResult result = vkCreatePipelineLayout(client->device, &pipelineLayoutInfo, NULL, &client->pipeline_layout);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create pipeline layout: %d\n", result);
        exit(EXIT_FAILURE);
    }

    /* Dynamic rendering via pNext chain */
    VkPipelineRenderingCreateInfo pipelineRenderingInfo = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext                   = NULL,
        .colorAttachmentCount    = 1,
        .pColorAttachmentFormats = &client->swap_chain_surface_format.format
    };

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext               = &pipelineRenderingInfo,
        .stageCount          = 2,
        .pStages             = shaderStages,
        .pVertexInputState   = &vertexInputInfo,
        .pInputAssemblyState = &inputAssembly,
        .pViewportState      = &viewportState,
        .pRasterizationState = &rasterizer,
        .pMultisampleState   = &multisampling,
        .pColorBlendState    = &colorBlending,
        .pDynamicState       = &dynamicState,
        .layout              = client->pipeline_layout,
        .renderPass          = VK_NULL_HANDLE
    };

    result = vkCreateGraphicsPipelines(client->device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, NULL, &client->graphics_pipeline);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create graphics pipeline: %d\n", result);
        exit(EXIT_FAILURE);
    }

    vkDestroyShaderModule(client->device, shaderModule, NULL);
}

void transitionImageLayout(Client *client,
                           uint32_t imageIndex,
                           VkImageLayout oldLayout,
                           VkImageLayout newLayout,
                           VkAccessFlags2 srcAccessMask,
                           VkAccessFlags2 dstAccessMask,
                           VkPipelineStageFlags2 srcStageMask,
                           VkPipelineStageFlags2 dstStageMask)
{
    VkImageMemoryBarrier2 barrier = {
        .sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .pNext               = NULL,
        .srcStageMask        = srcStageMask,
        .srcAccessMask       = srcAccessMask,
        .dstStageMask        = dstStageMask,
        .dstAccessMask       = dstAccessMask,
        .oldLayout           = oldLayout,
        .newLayout           = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image               = client->swap_chain_images[imageIndex],
        .subresourceRange    = {
            .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel   = 0,
            .levelCount     = 1,
            .baseArrayLayer = 0,
            .layerCount     = 1
        }
    };

    VkDependencyInfo dependencyInfo = {
        .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .pNext                   = NULL,
        .dependencyFlags         = 0,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers    = &barrier
    };

    vkCmdPipelineBarrier2(client->command_buffer, &dependencyInfo);
}

void recordCommandBuffer(Client *client, uint32_t imageIndex)
{
    /* Reset command buffer for reuse */
    vkResetCommandBuffer(client->command_buffer, 0);
    
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = NULL,
        .flags = 0
    };

    vkBeginCommandBuffer(client->command_buffer, &beginInfo);

    /* Transition to color attachment optimal */
    transitionImageLayout(client, imageIndex,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        0,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);

    /* Rendering attachment */
    VkClearValue clearColor = {.color = {.float32 = {0.0f, 0.0f, 0.0f, 1.0f}}};

    VkRenderingAttachmentInfo attachmentInfo = {
        .sType       = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .pNext       = NULL,
        .imageView   = client->swap_chain_image_views[imageIndex],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp      = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp     = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue  = clearColor
    };

    VkRenderingInfo renderingInfo = {
        .sType                = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext                = NULL,
        .renderArea           = {.offset = {0, 0}, .extent = client->swap_chain_extent},
        .layerCount           = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments    = &attachmentInfo
    };

    vkCmdBeginRendering(client->command_buffer, &renderingInfo);

    vkCmdBindPipeline(client->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, client->graphics_pipeline);

    VkViewport viewport = {
        .x        = 0.0f,
        .y        = 0.0f,
        .width    = (float)client->swap_chain_extent.width,
        .height   = (float)client->swap_chain_extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(client->command_buffer, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = client->swap_chain_extent
    };
    vkCmdSetScissor(client->command_buffer, 0, 1, &scissor);

    //vkCmdDraw(client->command_buffer, 3, 1, 0, 0);  // TODO: bind vertex buffers first

    vkCmdEndRendering(client->command_buffer);

    /* Transition to present src */
    transitionImageLayout(client, imageIndex,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        0,
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT);

    vkEndCommandBuffer(client->command_buffer);
}


void createCommandPool(Client *client)
{
    VkCommandPoolCreateInfo poolInfo = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext            = NULL,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = client->graphics_queue_family_index
    };

    VkResult result = vkCreateCommandPool(client->device, &poolInfo, NULL, &client->command_pool);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create command pool: %d\n", result);
        exit(EXIT_FAILURE);
    }
}

void createCommandBuffer(Client *client)
{
    VkCommandBufferAllocateInfo allocInfo = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext              = NULL,
        .commandPool        = client->command_pool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    VkResult result = vkAllocateCommandBuffers(client->device, &allocInfo, &client->command_buffer);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to allocate command buffer: %d\n", result);
        exit(EXIT_FAILURE);
    }
}


void createSyncObjects(Client *client)
{
    VkSemaphoreCreateInfo semaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0
    };

    VkFenceCreateInfo fenceInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    VkResult result = vkCreateSemaphore(client->device, &semaphoreInfo, NULL, &client->present_complete_semaphore);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create present complete semaphore: %d\n", result);
        exit(EXIT_FAILURE);
    }

    result = vkCreateSemaphore(client->device, &semaphoreInfo, NULL, &client->render_finished_semaphore);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create render finished semaphore: %d\n", result);
        exit(EXIT_FAILURE);
    }

    result = vkCreateFence(client->device, &fenceInfo, NULL, &client->draw_fence);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to create fence: %d\n", result);
        exit(EXIT_FAILURE);
    }
}

void cleanupSwapChain(Client *client)
{
    for (uint32_t i = 0; i < client->swap_chain_image_count; ++i)
        vkDestroyImageView(client->device, client->swap_chain_image_views[i], NULL);

    free(client->swap_chain_image_views);
    client->swap_chain_image_views = NULL;

    vkDestroySwapchainKHR(client->device, client->swap_chain, NULL);
    client->swap_chain = VK_NULL_HANDLE;

    free(client->swap_chain_images);
    client->swap_chain_images = NULL;
}


void cleanup(Client *client)
{
    cleanupSwapChain(client);

    vkDestroyPipeline(client->device, client->graphics_pipeline, NULL);
    vkDestroyPipelineLayout(client->device, client->pipeline_layout, NULL);

    vkDestroySemaphore(client->device, client->present_complete_semaphore, NULL);
    vkDestroySemaphore(client->device, client->render_finished_semaphore, NULL);
    vkDestroyFence(client->device, client->draw_fence, NULL);

    vkDestroyCommandPool(client->device, client->command_pool, NULL);
    vkDestroyDevice(client->device, NULL);
    vkDestroySurfaceKHR(client->instance, client->surface, NULL);
    vkDestroyInstance(client->instance, NULL);
}



void recreateSwapChain(Client *client)
{
    /* Wait while window is minimized */
    int width = 0, height = 0;
    SDL_GetWindowSizeInPixels(client->win.window, &width, &height);
    while (width == 0 || height == 0)
    {
        SDL_GetWindowSizeInPixels(client->win.window, &width, &height);
        SDL_WaitEvent(NULL);
    }

    vkDeviceWaitIdle(client->device);

    cleanupSwapChain(client);
    createSwapChain(client);
    createImageViews(client);
}


uint32_t *readFile(const char *filename, size_t *fileSize)
{
    FILE *file = fopen(filename, "rb");
    if (!file)
    {
        fprintf(stderr, "Failed to open file: %s\n", filename);
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);
    *fileSize = ftell(file);
    rewind(file);

    uint32_t *buffer = malloc(*fileSize);
    if (!buffer)
    {
        fprintf(stderr, "Failed to allocate memory for file: %s\n", filename);
        fclose(file);
        exit(EXIT_FAILURE);
    }

    fread(buffer, 1, *fileSize, file);
    fclose(file);

    return buffer;
}