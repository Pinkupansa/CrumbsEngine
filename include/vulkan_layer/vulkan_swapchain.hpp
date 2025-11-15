#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <vulkan_device.hpp>

class VulkanSwapchain {
private:
    VulkanDevice& pDevice;
    VkSwapchainKHR swapchain;
    VkExtent2D extent;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    VkFormat colorFormat;

    VkImage depthImage;
    VkImageView depthImageView;
    VkFormat depthFormat; 
    VkDeviceMemory depthMemory;
public:
    
    const VkSwapchainKHR& getSwapchain() const {
        return swapchain;
    }

    VkFormat getFormat() const{
        return colorFormat;
    }

    VkExtent2D getExtent() const{
        return extent;
    }

    const std::vector<VkImageView>& getImageViews() const{
        return swapchainImageViews;
    }

    const VkImageView& getDepthView() const{
        return depthImageView;
    }

    const VkFormat& getDepthFormat() const{
        return depthFormat;
    }

    std::vector<std::vector<VkImageView>> getAttachmentPerSwapchainImage(){
        return {{swapchainImageViews[0], depthImageView}, {swapchainImageViews[1], depthImageView}, {swapchainImageViews[2], depthImageView}};
    }
    VulkanSwapchain(VulkanDevice& device, VulkanInstance& instance, uint32_t width, uint32_t height): pDevice(device){
        
        VkSurfaceKHR surface = instance.getSurface();

        colorFormat = VK_FORMAT_B8G8R8A8_SRGB; 
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.getPhysicalDevice(), surface, &surfaceCapabilities);

        VkSwapchainCreateInfoKHR swapchainInfo{};
        swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainInfo.surface = surface;
        swapchainInfo.minImageCount = 3; // triple buffering
        swapchainInfo.imageFormat = colorFormat; // pick first supported format
        swapchainInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        swapchainInfo.imageExtent = surfaceCapabilities.currentExtent;
        swapchainInfo.imageArrayLayers = 1; //just means 2d image
        swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainInfo.preTransform = surfaceCapabilities.currentTransform;
        swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device.getPhysicalDevice(), surface, &presentModeCount, nullptr);
        
        std::vector<VkPresentModeKHR> availablePresentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device.getPhysicalDevice(), surface, &presentModeCount, availablePresentModes.data());
        
        VkPresentModeKHR chosenMode = VK_PRESENT_MODE_FIFO_KHR;

        for (const auto& availableMode : availablePresentModes) {
            if (availableMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                chosenMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
                break;
            }
        }
        
        swapchainInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR; // guaranteed to be available
        swapchainInfo.clipped = VK_TRUE;
        swapchainInfo.oldSwapchain = VK_NULL_HANDLE;
        

        if (vkCreateSwapchainKHR(device.getDevice(), &swapchainInfo, nullptr, &swapchain) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create swapchain!");
        }

        extent = swapchainInfo.imageExtent;

        // 1. Get swapchain images (3 because triple buffering)
        uint32_t imageCount = 0;
        vkGetSwapchainImagesKHR(device.getDevice(), swapchain, &imageCount, nullptr);
        swapchainImages.resize(imageCount);

        vkGetSwapchainImagesKHR(device.getDevice(), swapchain, &imageCount, swapchainImages.data());

        // 2. Create image views
        swapchainImageViews.resize(imageCount);
        for (int i = 0; i < imageCount; i++) {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = swapchainImages[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;          // 2D texture
            viewInfo.format = colorFormat;         // same as swapchain
            viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device.getDevice(), &viewInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create image view!");
            }
            
            device.nameObject((uint64_t)swapchainImageViews[i], VK_OBJECT_TYPE_IMAGE_VIEW, "Image View " + std::to_string(i));
        }
        
        //Depth buffer
        depthFormat = VK_FORMAT_D16_UNORM;
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width  = swapchainInfo.imageExtent.width;
        imageInfo.extent.height = swapchainInfo.imageExtent.height;
        imageInfo.extent.depth  = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = depthFormat;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkImage depthImagetest;
        if (vkCreateImage(device.getDevice(), &imageInfo, nullptr, &depthImagetest) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create depth image!");
        }
        depthImage = depthImagetest; 
        VkMemoryRequirements memReq; 
        vkGetImageMemoryRequirements(device.getDevice(), depthImage, &memReq);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex = device.findMemoryType(
            memReq.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        
        vkAllocateMemory(device.getDevice(), &allocInfo, nullptr, &depthMemory);
        vkBindImageMemory(device.getDevice(), depthImage, depthMemory, 0);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = depthImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = depthFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT; // important!
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        vkCreateImageView(device.getDevice(), &viewInfo, nullptr, &depthImageView);

    }
    ~VulkanSwapchain(){
        destroy();
    }

    void destroy(){
        for (int i = 0; i < swapchainImageViews.size(); i++) {
            if(swapchainImageViews[i] != VK_NULL_HANDLE){
                vkDestroyImageView(pDevice.getDevice(), swapchainImageViews[i], nullptr);
                swapchainImageViews[i] = VK_NULL_HANDLE;
            }
        }
        swapchainImageViews.clear();
        
        if(depthImageView != VK_NULL_HANDLE){
            vkDestroyImageView(pDevice.getDevice(), depthImageView, nullptr);
            depthImageView = VK_NULL_HANDLE;
        } 

        if(depthImage != VK_NULL_HANDLE){
            vkDestroyImage(pDevice.getDevice(), depthImage, nullptr);
            depthImage = VK_NULL_HANDLE;
        }
        if(depthMemory != VK_NULL_HANDLE){
            vkFreeMemory(pDevice.getDevice(), depthMemory, nullptr);
            depthMemory = VK_NULL_HANDLE;
        }
        if(swapchain != VK_NULL_HANDLE){
            vkDestroySwapchainKHR(pDevice.getDevice(), swapchain, nullptr);
            swapchain = VK_NULL_HANDLE;
        }
    }

};
