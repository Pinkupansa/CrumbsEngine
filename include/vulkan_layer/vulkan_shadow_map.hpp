#pragma once
#include "vulkan_descriptor_data.hpp"
#include "vulkan_device.hpp"
#include "vulkan_object_creation_utils.hpp"
#include "vulkan_texture_sampler.hpp"
#include <vector>
#include <vulkan/vulkan.h>
class VulkanShadowMap {
    private:
    VulkanDevice& pDevice;

    VulkanAttachment shadowAttachment;
    VulkanTextureSampler shadowSampler;
    VulkanSyncObjects syncObjects;

    public:
    std::vector<std::vector<VulkanAttachment*>> getAttachmentsPerFrameBuffer () {
        return { { &shadowAttachment } };
    }
    
    VulkanAttachment* getShadowAttachment (){
        return &shadowAttachment;
    }

    const VulkanTextureSampler& getTextureSampler(){
        return shadowSampler;
    }

    VulkanShadowMap (VulkanDevice& device, uint width, uint height, VkFormat format)
    : pDevice (device), syncObjects (device, 1, "Shadow Sync "),
      shadowAttachment (device,
                        VulkanAttachmentType::ShadowMap,
                        { width, height },
                        true,
                        "Shadow Attachment "), shadowSampler(device, shadowAttachment.getImageView(), "Shadow Texture Sampler "){}
    ~VulkanShadowMap () {
        destroy ();
    }

    void waitAndResetFences () const {
        vkWaitForFences (pDevice.getDevice (), 1, &syncObjects.inFlightFence[0],
                         VK_TRUE, UINT64_MAX);
        vkResetFences (pDevice.getDevice (), 1, &syncObjects.inFlightFence[0]);
    }

    void drawWithDrawer (VulkanImageDrawer& imageDrawer,
                         const VulkanBuffer& vertexBuffer,
                         const VulkanBuffer& indexBuffer,
                         const std::vector<MeshDrawInfo>& meshPool,
                         const std::vector<uint32_t>& drawCallMeshIndices) {

        waitAndResetFences ();
        imageDrawer.draw (vertexBuffer, indexBuffer, meshPool, drawCallMeshIndices,
                          syncObjects, false, false, 0, 0);
    }
    void destroy () {
        syncObjects.destroy ();
        shadowSampler.destroy();
        shadowAttachment.destroy();
    }
};