#pragma once
#include "vulkan_descriptor_data.hpp"
#include "vulkan_device.hpp"
#include "vulkan_object_creation_utils.hpp"
#include "vulkan_texture_descriptor.hpp"
#include <vector>
#include <vulkan/vulkan.h>
class VulkanShadowMap {
    private:
    VulkanDevice& pDevice;

    VulkanAttachment shadowAttachment;
    VulkanTextureDescriptor shadowTexture;
    VulkanSyncObjects syncObjects;

    public:
    std::vector<std::vector<VulkanAttachment*>> getAttachmentsPerFrameBuffer () {
        return { { &shadowAttachment } };
    }
    
    VulkanAttachment* getShadowAttachment (){
        return &shadowAttachment;
    }

    const VulkanTextureDescriptor& getTexture(){
        return shadowTexture;
    }

    VulkanShadowMap (VulkanDevice& device, uint width, uint height, VkFormat format)
    : pDevice (device), syncObjects (device, 1, false, "Shadow Sync "),
      shadowAttachment (device,
                        VulkanAttachmentType::ShadowMap,
                        { width, height },
                        true, true, createDepthClearValue({1.0f, 0}),
                        "Shadow Attachment "), shadowTexture(device, shadowAttachment.getImageView(), "Shadow Texture Sampler "){}
    ~VulkanShadowMap () {
        destroy ();
    }
    const std::function<void()> getFenceResetCallback(){
        return std::bind (&VulkanShadowMap::waitAndResetFences, this);
    }
    const VulkanSyncObjects& getSyncObjects() const{
        return syncObjects;
    }
    void waitAndResetFences () const {
        vkWaitForFences (pDevice.getDevice (), 1, &syncObjects.inFlightFence[0],
                         VK_TRUE, UINT64_MAX);
        vkResetFences (pDevice.getDevice (), 1, &syncObjects.inFlightFence[0]);
    }

    void destroy () {
        syncObjects.destroy ();
        shadowTexture.destroy();
        shadowAttachment.destroy();
    }
};