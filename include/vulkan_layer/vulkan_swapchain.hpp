#pragma once
#include "vulkan_device.hpp"
#include "vulkan_object_creation_utils.hpp"
#include "vulkan_surface.hpp"
#include "vulkan_image_drawer.hpp"
#include "vulkan_attachment.hpp"
#include <vector>
#include <vulkan/vulkan.h>
class VulkanSwapchain {
    private:
    VulkanDevice& pDevice;
    VkSwapchainKHR swapchain;
    

    std::vector<VkImage> swapchainImages;
    std::vector<VulkanAttachment> swapchainAttachments;

    std::vector<VulkanAttachment> msaaColorAttachments;
    VulkanSyncObjects syncObjects; 

    uint32_t currentFrame;

    int currentSyncIndex;

    public:
    const VkSwapchainKHR& getSwapchain () const {
        return swapchain;
    }
 

    std::vector<std::vector<VulkanAttachment>> getAttachmentsPerFrameBuffer () {
        return { { msaaColorAttachments[0], swapchainAttachments [0]},
                 { msaaColorAttachments[1], swapchainAttachments [1]},
                 { msaaColorAttachments[2], swapchainAttachments [2]}};
    }
    VulkanSwapchain (VulkanDevice& device, VulkanSurface& surface)
    : pDevice (device), syncObjects(device, 3) {

        swapchain = createTripleBufferingSwapchain (device, surface, "Main Swapchain");

        // get extent

        // 1. Get swapchain images (3 because triple buffering)
        uint32_t imageCount = 0;
        vkGetSwapchainImagesKHR (device.getDevice (), swapchain, &imageCount, nullptr);
        swapchainImages.resize (imageCount);

        vkGetSwapchainImagesKHR (device.getDevice (), swapchain, &imageCount,
                                 swapchainImages.data ());


        msaaColorAttachments.resize(imageCount);
        for(int i = 0; i < swapchainImages.size(); i++){
            VulkanAttachment msaaColorAttachment(device, VulkanAttachmentType::Color, surface.getCapabilities().currentExtent, false, "MSAA Color Attachment " + std::to_string(i));
            msaaColorAttachments.push_back(msaaColorAttachment);
        }

        // 2. Create image views
        swapchainAttachments.resize(imageCount);
        for (int i = 0; i < imageCount; i++) {
            VulkanAttachment swapchainAttachment(device, VulkanAttachmentType::Color, swapchainImages[i], surface.getCapabilities().currentExtent, true, "Swapchain Attachment " + std::to_string(i));
            swapchainAttachments.push_back(swapchainAttachment);
        }

        currentFrame = 0;
        currentSyncIndex = 0;
    }


    

    void present () {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores    = &syncObjects.renderFinishedSemaphore[currentSyncIndex];
        presentInfo.swapchainCount     = 1;
        presentInfo.pSwapchains        = &swapchain;
        presentInfo.pImageIndices      = &currentFrame;
        vkQueuePresentKHR (pDevice.getGraphicsQueue (), &presentInfo);

        currentSyncIndex = (currentSyncIndex + 1) % 3;
        
        
    }

    void updateFrameIndex () {
        vkAcquireNextImageKHR (pDevice.getDevice (), swapchain, UINT64_MAX,
                               syncObjects.imageAvailableSemaphore[currentSyncIndex], VK_NULL_HANDLE, &currentFrame);
    }

    void waitAndResetFences () const {
        vkWaitForFences (pDevice.getDevice (), 1,
                         &syncObjects.inFlightFence[currentSyncIndex], VK_TRUE, UINT64_MAX);
        vkResetFences (pDevice.getDevice (), 1, &syncObjects.inFlightFence[currentSyncIndex]);
    }

    void drawWithDrawer (VulkanImageDrawer& imageDrawer,
                         const VulkanBuffer& vertexBuffer,
                         const VulkanBuffer& indexBuffer,
                         const std::vector<MeshDrawInfo>& meshPool,
                         bool isFirstPass,
                         const std::vector<uint32_t>& drawCallMeshIndices) {
        waitAndResetFences ();
        imageDrawer.draw (vertexBuffer, indexBuffer, meshPool, drawCallMeshIndices, syncObjects, true, true, isFirstPass, currentSyncIndex, currentFrame);
    }
    ~VulkanSwapchain () {
        destroy ();
    }

    void destroy () {
        syncObjects.destroy();
        
        for(int i = 0; i < swapchainImages.size(); i++){
            swapchainAttachments[i].destroy();
            msaaColorAttachments[i].destroy();
        }
        if (swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR (pDevice.getDevice (), swapchain, nullptr);
            swapchain = VK_NULL_HANDLE;
        }

        
    }
};
