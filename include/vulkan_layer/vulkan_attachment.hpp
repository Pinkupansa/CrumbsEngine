#pragma once
#include "vulkan_device.hpp"
#include "vulkan_object_creation_utils.hpp"
#include "vulkan_sync_objects.hpp"
#include <vulkan/vulkan.h>
enum VulkanAttachmentType { Depth, Color, ShadowMap };

class VulkanAttachment {

    private:
    const VulkanDevice& device;

    VulkanAttachmentType type;
    VkImage image;
    VkImageView imageView;
    VkDeviceMemory memory;

    VkExtent2D extent;

    bool isImageCreator;
    bool isResolve;

    public:
    VkImageView getImageView () const {
        return imageView;
    }

    VkImage getImage () const {
        return image;
    }

    VulkanAttachmentType getType() const {
        return type;
    }

    bool isColorResolveAttachment() const{
        return isResolve and type == VulkanAttachmentType::Color;
    }

    VkAttachmentDescription createAttachmentDesc (VkAttachmentLoadOp loadOp) const {
        switch (type) {
        case VulkanAttachmentType::Color:
            return createColorAttachment (isResolve, loadOp);

        case VulkanAttachmentType::Depth:
            return createDepthAttachment(loadOp);
        
        case VulkanAttachmentType::ShadowMap:
            return createShadowDepthAttachment(loadOp);
        }
        
    }
    VkExtent2D getExtent () const {
        return extent;
    }


    VulkanAttachment (const VulkanDevice& device,
                      VulkanAttachmentType type,
                      VkExtent2D extent,
                      bool isResolve,
                      std::string name)
    : device (device), extent (extent), type (type), isImageCreator (true),
      isResolve (isResolve) {
        switch (type) {
        case VulkanAttachmentType::Color:
            image = createColorImage (device, extent, isResolve, name + " Color Image");
            memory                = allocateAndBindImageMemory (device, image);
            imageView = createColorImageView (device, image, name + " Color Image View");


            break;
        case VulkanAttachmentType::Depth:
            
            image = createDepthImage (device, extent, isResolve, name + " Depth Image");
            memory                = allocateAndBindImageMemory (device, image);
            imageView = createDepthImageView (device, image, name + " Depth Image View");

            break;
        case VulkanAttachmentType::ShadowMap:
            image = createImage (device, extent, DEFAULT_SHADOW_FORMAT, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, true, name + " Depth Image");
            memory                = allocateAndBindImageMemory (device, image); 
            imageView = createDepthImageView (device, image, name + " Depth Image View");

            break;
        }
    }

    VulkanAttachment (const VulkanDevice& device,
                      VulkanAttachmentType type,
                      VkImage image,
                      VkExtent2D extent,
                      bool isResolve,
                      std::string name)
    : device (device), type(type), image (image), extent (extent), isImageCreator (false),
      isResolve (isResolve) {
        
        // used for swapchain images
        switch (type) {
        case VulkanAttachmentType::Color:
            imageView = createColorImageView (device, image, name + " Color Image View");
            break;
        case VulkanAttachmentType::Depth:
            imageView = createDepthImageView (device, image, name + " Depth Image View");
            break;
        case VulkanAttachmentType::ShadowMap:
            imageView = createDepthImageView (device, image, name + " Depth Image View");
            break;
        }
    }
    ~VulkanAttachment () {
        destroy ();
    }

    void destroy () {
        if (image != VK_NULL_HANDLE and isImageCreator) {
            vkDestroyImage (device.getDevice (), image, nullptr);
            image = VK_NULL_HANDLE;
        }
        if (imageView != VK_NULL_HANDLE) {
            vkDestroyImageView (device.getDevice (), imageView, nullptr);
            imageView = VK_NULL_HANDLE;
        }
        if (memory != VK_NULL_HANDLE and isImageCreator) {
            vkFreeMemory (device.getDevice (), memory, nullptr);
            memory = VK_NULL_HANDLE;
        }
    }
};