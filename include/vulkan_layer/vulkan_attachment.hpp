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

    VkImageLayout finalLayout; // layout after render pass

    bool isImageCreator;
    bool isSinglesampled;
    bool isResolve; 

    public:
    VkClearValue getClearValue () const {
        return clearValue;
    }

    VkImageView getImageView () const {
        return imageView;
    }

    VkImage getImage () const {
        return image;
    }

    VulkanAttachmentType getType () const {
        return type;
    }

    bool isColorResolveAttachment () const {
        return isResolve and type == VulkanAttachmentType::Color;
    }

    VkClearValue clearValue;
    VkAttachmentDescription createAttachmentDesc (VkAttachmentLoadOp loadOp) const {
        switch (type) {
        case VulkanAttachmentType::Color:
            return createColorAttachment (isSinglesampled, loadOp, finalLayout);

        case VulkanAttachmentType::Depth: return createDepthAttachment (loadOp);

        case VulkanAttachmentType::ShadowMap:
            return createShadowDepthAttachment (loadOp);
        }
    }
    VkExtent2D getExtent () const {
        return extent;
    }


    VulkanAttachment (const VulkanDevice& device,
                      VulkanAttachmentType type,
                      VkExtent2D extent,
                      bool isResolve,
                      bool isSinglesampled,
                      VkClearValue clearValue,
                      std::string name,
                      
                      bool isSampleable = false,
                      VkImageLayout finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
    : device (device), extent (extent), type (type), isImageCreator (true), clearValue (clearValue),
      finalLayout (finalLayout), isResolve (isResolve), isSinglesampled(isSinglesampled) {
        switch (type) {
        case VulkanAttachmentType::Color:
            image =
            createColorImage (device, extent, isSinglesampled, name + " Color Image",
                              isSampleable ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT :
                                             VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
            memory = allocateAndBindImageMemory (device, image);
            imageView = createColorImageView (device, image, name + " Color Image View");


            break;
        case VulkanAttachmentType::Depth:

            image =
            createDepthImage (device, extent, isSinglesampled, name + " Depth Image",
                              isSampleable ?
                              VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT :
                              VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
            memory = allocateAndBindImageMemory (device, image);
            imageView = createDepthImageView (device, image, name + " Depth Image View");

            break;
        case VulkanAttachmentType::ShadowMap:
            image  = createImage (device, extent, DEFAULT_SHADOW_FORMAT,
                                  VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                                  VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                  true, name + " Depth Image");
            memory = allocateAndBindImageMemory (device, image);
            imageView = createDepthImageView (device, image, name + " Depth Image View");

            break;
        }
    }

    VulkanAttachment (const VulkanDevice& device,
                      VulkanAttachmentType type,
                      VkImage image,
                      VkExtent2D extent,
                      bool isResolve,
                      bool isSinglesampled,
                      VkClearValue clearValue,
                      std::string name,
                      VkImageLayout finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
    : device (device), type (type), image (image), extent (extent),
      isImageCreator (false), isResolve (isResolve), isSinglesampled(isSinglesampled), finalLayout (finalLayout) {

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
        
        if (imageView != VK_NULL_HANDLE) {
            vkDestroyImageView (device.getDevice (), imageView, nullptr);
            imageView = VK_NULL_HANDLE;
        }
        if (image != VK_NULL_HANDLE and isImageCreator) {
            vkDestroyImage (device.getDevice (), image, nullptr);
            image = VK_NULL_HANDLE;
        }
        
        if (memory != VK_NULL_HANDLE and isImageCreator) {
            vkFreeMemory (device.getDevice (), memory, nullptr);
            memory = VK_NULL_HANDLE;
        }
    }
};