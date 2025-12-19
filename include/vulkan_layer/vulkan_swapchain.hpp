#pragma once
#include "vulkan_device.hpp"
#include "vulkan_object_creation_utils.hpp"
#include "vulkan_surface.hpp"
#include "vulkan_image_drawer.hpp"
#include <vector>
#include <vulkan/vulkan.h>
class VulkanSwapchain {
    private:
    VulkanDevice& pDevice;
    VkSwapchainKHR swapchain;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;

    std::vector<VkImage> msaaColorImages;
    std::vector<VkDeviceMemory> msaaColorMemories;
    std::vector<VkImageView> msaaColorImageViews;

    VkImage depthImage;
    VkImageView depthImageView;
    VkDeviceMemory depthMemory;

    public:
    const VkSwapchainKHR& getSwapchain () const {
        return swapchain;
    }

    const std::vector<VkImageView>& getImageViews () const {
        return swapchainImageViews;
    }

    const VkImageView& getDepthView () const {
        return depthImageView;
    }

    std::vector<std::vector<VkImageView>> getAttachmentsPerFramebuffer () {
        return { { msaaColorImageViews[0], getDepthView (), getImageViews ()[0]},
                 { msaaColorImageViews[1], getDepthView (), getImageViews ()[1]},
                 { msaaColorImageViews[2], getDepthView (), getImageViews ()[2]}};
    }
    VulkanSwapchain (VulkanDevice& device, VulkanSurface& surface)
    : pDevice (device) {

        swapchain = createTripleBufferingSwapchain (device, surface, "Main Swapchain");

        // get extent
        VkSurfaceCapabilitiesKHR surfaceCapabilities;

        // 1. Get swapchain images (3 because triple buffering)
        uint32_t imageCount = 0;
        vkGetSwapchainImagesKHR (device.getDevice (), swapchain, &imageCount, nullptr);
        swapchainImages.resize (imageCount);

        vkGetSwapchainImagesKHR (device.getDevice (), swapchain, &imageCount,
                                 swapchainImages.data ());


        msaaColorImages.resize(swapchainImages.size());
        msaaColorMemories.resize(swapchainImages.size());
        msaaColorImageViews.resize(swapchainImages.size());
        for(int i = 0; i < swapchainImages.size(); i++){
            msaaColorImages[i] = createImage(device, surface.getCapabilities().currentExtent, DEFAULT_COLOR_FORMAT, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
          VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, false, "MSAA Color " + std::to_string(i));
            msaaColorMemories[i] = allocateAndBindImageMemory(device, msaaColorImages[i]);
            msaaColorImageViews[i] = createColorImageView(device, msaaColorImages[i], "MSAA Color Image View " + std::to_string(i));
        }
        // 2. Create image views
        swapchainImageViews.resize (imageCount);
        for (int i = 0; i < imageCount; i++) {
            swapchainImageViews[i] =
            createColorImageView (device, swapchainImages[i],
                                  "Swapchain Color Image View " + std::to_string (i));
        }

        // 3. Depth buffer
        depthImage  = createDepthImage (device, surface.getCapabilities().currentExtent, false, "Main Depth Image");
        depthMemory = allocateAndBindImageMemory (device, depthImage);
        depthImageView = createDepthImageView (device, depthImage, "Depth Buffer Main");
    }

    void present (const VkSemaphore& renderFinishedSemaphore, uint32_t imageIndex) {
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores    = &renderFinishedSemaphore;
        presentInfo.swapchainCount     = 1;
        presentInfo.pSwapchains        = &swapchain;
        presentInfo.pImageIndices      = &imageIndex;
        vkQueuePresentKHR (pDevice.getGraphicsQueue (), &presentInfo);
    }

    uint32_t acquireNextImageIndex (const VkSemaphore& imageAvailableSemaphore) {
        uint32_t imageIndex;
        vkAcquireNextImageKHR (pDevice.getDevice (), swapchain, UINT64_MAX,
                               imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        return imageIndex;
    }

    void drawWithDrawer (VulkanImageDrawer& imageDrawer,
                         const VulkanBuffer& vertexBuffer,
                         const VulkanBuffer& indexBuffer,
                         const std::vector<MeshDrawInfo>& meshPool,
                         const std::vector<uint32_t>& drawCallMeshIndices) {
        uint32_t currentFrame = imageDrawer.getCurrentFrame ();
        imageDrawer.waitAndReset ();
        uint32_t imageIndex = acquireNextImageIndex (
        imageDrawer.getSyncObjects ().imageAvailableSemaphore[currentFrame]);

        imageDrawer.draw (vertexBuffer, indexBuffer, meshPool, drawCallMeshIndices, 1, 1, imageIndex);

        present (imageDrawer.getSyncObjects ().renderFinishedSemaphore[currentFrame],
                           imageIndex);
    }
    ~VulkanSwapchain () {
        destroy ();
    }

    void destroy () {
        for (int i = 0; i < swapchainImageViews.size (); i++) {
            if (swapchainImageViews[i] != VK_NULL_HANDLE) {
                vkDestroyImageView (pDevice.getDevice (), swapchainImageViews[i], nullptr);
                swapchainImageViews[i] = VK_NULL_HANDLE;
            }
            if(msaaColorImages[i] != VK_NULL_HANDLE){
                vkDestroyImage(pDevice.getDevice(), msaaColorImages[i], nullptr); 
                msaaColorImages[i] = VK_NULL_HANDLE;
            }
            if(msaaColorImageViews[i] != VK_NULL_HANDLE){
                vkDestroyImageView(pDevice.getDevice(), msaaColorImageViews[i], nullptr); 
                msaaColorImageViews[i] = VK_NULL_HANDLE;
            }
            if(msaaColorMemories[i] != VK_NULL_HANDLE){
                vkFreeMemory(pDevice.getDevice(), msaaColorMemories[i], nullptr);
                msaaColorMemories[i] = VK_NULL_HANDLE;
            }
        }
        swapchainImageViews.clear ();
        msaaColorImages.clear();
        msaaColorImageViews.clear();
        msaaColorMemories.clear();

        if (depthImageView != VK_NULL_HANDLE) {
            vkDestroyImageView (pDevice.getDevice (), depthImageView, nullptr);
            depthImageView = VK_NULL_HANDLE;
        }

        if (depthImage != VK_NULL_HANDLE) {
            vkDestroyImage (pDevice.getDevice (), depthImage, nullptr);
            depthImage = VK_NULL_HANDLE;
        }
        if (depthMemory != VK_NULL_HANDLE) {
            vkFreeMemory (pDevice.getDevice (), depthMemory, nullptr);
            depthMemory = VK_NULL_HANDLE;
        }
        if (swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR (pDevice.getDevice (), swapchain, nullptr);
            swapchain = VK_NULL_HANDLE;
        }

        
    }
};
