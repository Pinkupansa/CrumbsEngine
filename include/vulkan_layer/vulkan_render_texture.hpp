#pragma once
#include "vulkan_attachment.hpp"
#include "vulkan_texture_descriptor.hpp"
#include <vulkan/vulkan.h>

// A texture that can be renderered to and sampled in a shader
class VulkanRenderTexture {
    private:
    const VulkanDevice& device;
    VulkanAttachment colorAttachment;
    VulkanAttachment resolveAttachment;
    VulkanTextureDescriptor textureDescriptor;
    VulkanSyncObjects syncObjects;

    public:
    VulkanRenderTexture (const VulkanDevice& device, VkExtent2D extent, std::string name)
    : device (device), colorAttachment (device,
                                        VulkanAttachmentType::Color,
                                        extent,
                                        false,
                                        false, createColorClearValue({0, 0, 0, 0}),
                                        name + " Attachment",
                                        true,
                                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
      resolveAttachment (device,
                         VulkanAttachmentType::Color,
                         extent,
                         true,
                         true, createColorClearValue({0, 0, 0, 0}),
                         name + " Resolve Attachment",
                         true,
                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
      textureDescriptor (device, resolveAttachment.getImageView (), name + " Texture Descriptor"),
      syncObjects (device, 1, false, name + " Sync") {
    }

    ~VulkanRenderTexture () {
        destroy ();
    }
    std::function<void ()> getFenceResetCallback () const {
        return std::bind (&VulkanRenderTexture::waitAndResetFences, this);
    }

    const VulkanSyncObjects* getSyncObjects () const {

        return &syncObjects;
    }

    VulkanAttachment* getColorAttachment () {
        return &colorAttachment;
    }
    VulkanAttachment* getResolveAttachment () {
        return &resolveAttachment;
    }
    VulkanTextureDescriptor& getTextureDescriptor () {
        return textureDescriptor;
    }
    void waitAndResetFences () const {
        vkWaitForFences (device.getDevice (), 1, &syncObjects.inFlightFence[0],
                         VK_TRUE, UINT64_MAX);
        vkResetFences (device.getDevice (), 1, &syncObjects.inFlightFence[0]);
    }


    void destroy () {
        syncObjects.destroy ();
        textureDescriptor.destroy ();
        colorAttachment.destroy ();
        resolveAttachment.destroy();
    }
};