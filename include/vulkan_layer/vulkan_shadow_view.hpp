#pragma once
#include "vulkan_descriptor_data.hpp"
#include "vulkan_device.hpp"
#include "vulkan_object_creation_utils.hpp"
#include <vector>
#include <vulkan/vulkan.h>
class VulkanShadowView {
    private:
    VulkanDevice& pDevice;

    VulkanAttachment shadowAttachment;

    VkSampler shadowSampler;
    VkDescriptorSetLayout shadowDescLayout;
    VkDescriptorPool shadowDescPool;
    VkDescriptorSet shadowDescSet;
    VulkanSyncObjects syncObjects;

    public:
    std::vector<std::vector<VulkanAttachment*>> getAttachmentsPerFrameBuffer () {
        return { { &shadowAttachment } };
    }
    
    VulkanAttachment* getShadowAttachment (){
        return &shadowAttachment;
    }

    const VulkanDescriptorData getDescData (int binding, int set) const {
        return { shadowDescSet,
                 shadowDescLayout,
                 shadowDescPool,
                 false,
                 0,
                 binding,
                 set };
    }

    const VkDescriptorSet& getDescSet () const {
        return shadowDescSet;
    }
    const VkDescriptorSetLayout getLayout () const {
        return shadowDescLayout;
    }
    VulkanShadowView (VulkanDevice& device, uint width, uint height, VkFormat format)
    : pDevice (device), syncObjects (device, 1, "Shadow Sync "),
      shadowAttachment (device,
                        VulkanAttachmentType::ShadowMap,
                        { width, height },
                        true,
                        "Shadow Attachment ") {

        // Create a sampler for sampling the shadow map in the fragment shader

        shadowSampler = createSampler (device, "Shadow Sampler");

        shadowDescLayout =
        createDescriptorLayout (device, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                VK_SHADER_STAGE_FRAGMENT_BIT, 0);

        shadowDescPool =
        createDescriptorPool (device, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

        shadowDescSet = allocateDescriptorSet (device, shadowDescLayout, shadowDescPool,
                                               "Shadow Descriptor Set");

        writeImageSamplerInDescriptorSet (device, shadowAttachment.getImageView(), shadowSampler, shadowDescSet);
    }
    ~VulkanShadowView () {
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
        if (shadowDescLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout (pDevice.getDevice (), shadowDescLayout, nullptr);
            shadowDescLayout = VK_NULL_HANDLE;
        }

        if (shadowDescPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool (pDevice.getDevice (), shadowDescPool, nullptr);
            shadowDescPool = VK_NULL_HANDLE;
        }

        // Descriptor sets are freed implicitly when the pool is destroyed,
        // but you can explicitly free them if needed:
        if (shadowDescSet != VK_NULL_HANDLE && shadowDescPool != VK_NULL_HANDLE) {
            vkFreeDescriptorSets (pDevice.getDevice (), shadowDescPool, 1, &shadowDescSet);
            shadowDescSet = VK_NULL_HANDLE;
        }

        if (shadowSampler != VK_NULL_HANDLE) {
            vkDestroySampler (pDevice.getDevice (), shadowSampler, nullptr);
            shadowSampler = VK_NULL_HANDLE;
        }
        shadowAttachment.destroy();
    }
};