#pragma once
#include <vulkan/vulkan.h>
#include <stdexcept>
#include "vulkan_device.hpp"
#include <vector>
class VulkanSyncObjects {

private:
    VulkanDevice& pDevice;
public:
    std::vector<VkSemaphore> imageAvailableSemaphore;
    std::vector<VkSemaphore> renderFinishedSemaphore;
    std::vector<VkFence> inFlightFence;

    VulkanSyncObjects(VulkanDevice& device, int imageCount): pDevice(device){
        imageAvailableSemaphore.resize(imageCount);
        renderFinishedSemaphore.resize(imageCount);
        inFlightFence.resize(imageCount);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start signaled so first frame runs immediately

        for(int i = 0; i < imageCount; i++){
            if (vkCreateSemaphore(device.getDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphore[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device.getDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphore[i]) != VK_SUCCESS ||
                vkCreateFence(device.getDevice(), &fenceInfo, nullptr, &inFlightFence[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create synchronization objects!");
            }
        }
    }
    ~VulkanSyncObjects() {
        destroy();
    }

    void destroy() {
        for (size_t i = 0; i < inFlightFence.size(); ++i) {
            if (inFlightFence[i] != VK_NULL_HANDLE) {
                vkDestroyFence(pDevice.getDevice(), inFlightFence[i], nullptr);
                inFlightFence[i] = VK_NULL_HANDLE;
            }
            if (renderFinishedSemaphore[i] != VK_NULL_HANDLE) {
                vkDestroySemaphore(pDevice.getDevice(), renderFinishedSemaphore[i], nullptr);
                renderFinishedSemaphore[i] = VK_NULL_HANDLE;
            }
            if (imageAvailableSemaphore[i] != VK_NULL_HANDLE) {
                vkDestroySemaphore(pDevice.getDevice(), imageAvailableSemaphore[i], nullptr);
                imageAvailableSemaphore[i] = VK_NULL_HANDLE;
            }
        }

    }
};
