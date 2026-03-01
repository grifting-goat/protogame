#include "render.h"

void render_frame(Client *client) {
    vkQueueWaitIdle(client->graphics_queue);

    /* Acquire next swapchain image */
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(client->device, client->swap_chain, UINT64_MAX,
                                             client->present_complete_semaphore, VK_NULL_HANDLE, &imageIndex);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to acquire swapchain image: %d\n", result);
        exit(EXIT_FAILURE);
    }

    recordCommandBuffer(client, imageIndex);

    /* Reset fence and submit */
    vkResetFences(client->device, 1, &client->draw_fence);

    VkPipelineStageFlags waitDestinationStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo = {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext                = NULL,
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &client->present_complete_semaphore,
        .pWaitDstStageMask    = &waitDestinationStageMask,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &client->command_buffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &client->render_finished_semaphore
    };

    vkQueueSubmit(client->graphics_queue, 1, &submitInfo, client->draw_fence);

    result = vkWaitForFences(client->device, 1, &client->draw_fence, VK_TRUE, UINT64_MAX);
    if (result != VK_SUCCESS)
    {
        fprintf(stderr, "Failed to wait for fence: %d\n", result);
        exit(EXIT_FAILURE);
    }

    /* Present */
    VkPresentInfoKHR presentInfo = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext              = NULL,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &client->render_finished_semaphore,
        .swapchainCount     = 1,
        .pSwapchains        = &client->swap_chain,
        .pImageIndices      = &imageIndex
    };

    result = vkQueuePresentKHR(client->graphics_queue, &presentInfo);
    switch (result)
    {
        case VK_SUCCESS:
            break;
        case VK_SUBOPTIMAL_KHR:
            fprintf(stderr, "vkQueuePresentKHR returned VK_SUBOPTIMAL_KHR\n");
            break;
        default:
            break;
    }
}