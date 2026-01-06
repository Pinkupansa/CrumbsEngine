#include "vulkan_layer/vulkan_attachment.hpp"

#include "vulkan_layer/vulkan_constants.hpp"
#include "vulkan_layer/vulkan_object_creation_utils.hpp"
#include "vulkan_layer/vulkan_sync_objects.hpp"


VkClearValue VulkanAttachment::getClearValue () const {
    return clearValue;
}

VkImageView VulkanAttachment::getImageView () const {
    return imageView;
}

VkImage VulkanAttachment::getImage () const {
    return image;
}

VulkanAttachmentType VulkanAttachment::getType () const {
    return type;
}

bool VulkanAttachment::isColorResolveAttachment () const {
    return isResolve and type == VulkanAttachmentType::Color;
}

VkAttachmentDescription VulkanAttachment::createAttachmentDesc () {
    VkAttachmentLoadOp loadOp =
    alreadyBound ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
    VkImageLayout initialLayout = !alreadyBound? VK_IMAGE_LAYOUT_UNDEFINED : finalLayout;
    alreadyBound = true;
    
    switch (type) {
    case VulkanAttachmentType::Color:
        return createColorAttachment (isSinglesampled, loadOp, initialLayout, finalLayout);

    case VulkanAttachmentType::Depth: return createDepthAttachment (loadOp, initialLayout, finalLayout);

    case VulkanAttachmentType::ShadowMap:
        return createShadowDepthAttachment (loadOp);
    }
}

VkExtent2D VulkanAttachment::getExtent () const {
    return extent;
}

VulkanAttachment::VulkanAttachment (const VulkanDevice& device,
                                    VulkanAttachmentType type,
                                    VkExtent2D extent,
                                    bool isResolve,
                                    bool isSinglesampled,
                                    VkClearValue clearValue,
                                    std::string name,
                                    VkImageLayout finalLayout,
                                    bool isSampleable)
: device (device), extent (extent), type (type), isImageCreator (true),
  clearValue (clearValue), finalLayout (finalLayout), isResolve (isResolve),
  isSinglesampled (isSinglesampled), alreadyBound (false) {
    std::string suffix = isResolve ? " Resolve Image" : " Color Image";
    switch (type) {
    case VulkanAttachmentType::Color:
        image =
        createColorImage (device, extent, isSinglesampled, name + suffix,
                          isSampleable ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT :
                                         VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
        memory = allocateAndBindImageMemory (device, image);
        imageView = createColorImageView (device, image, name + suffix + " View");

        break;
    case VulkanAttachmentType::Depth:

        image = createDepthImage (device, extent, isSinglesampled, name + " Depth Image",
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

VulkanAttachment::VulkanAttachment (const VulkanDevice& device,
                                    VulkanAttachmentType type,
                                    VkImage image,
                                    VkExtent2D extent,
                                    bool isResolve,
                                    bool isSinglesampled,
                                    VkClearValue clearValue,
                                    std::string name,
                                    VkImageLayout finalLayout)
: device (device), type (type), image (image), extent (extent),
  isImageCreator (false), isResolve (isResolve), isSinglesampled (isSinglesampled),
  clearValue (clearValue), finalLayout (finalLayout), alreadyBound (false) {
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
VulkanAttachment::~VulkanAttachment () {
    destroy ();
}

void VulkanAttachment::destroy () {
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
