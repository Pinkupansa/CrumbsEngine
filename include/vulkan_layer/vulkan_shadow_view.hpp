#pragma once
#include "vulkan_descriptor_data.hpp"
#include "vulkan_device.hpp"
#include "vulkan_object_creation_utils.hpp"
#include <vector>
#include <vulkan/vulkan.h>
class VulkanShadowView {
    private:
    VulkanDevice& pDevice;
    VkImage shadowImage;
    VkImageView shadowImageView;
    VkExtent2D extent;
    VkDeviceMemory shadowMemory;
    VkSampler shadowSampler;
    VkDescriptorSetLayout shadowDescLayout;
    VkDescriptorPool shadowDescPool;
    VkDescriptorSet shadowDescSet;
    VulkanSyncObjects syncObjects;

    public:
    VkImage getShadowImage () {
        return shadowImage;
    }
    std::vector<std::vector<VkImageView>> getAttachmentsPerImage () {
        return { { shadowImageView } };
    }
    VkExtent2D getExtent () {
        return extent;
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
    : pDevice (device), syncObjects (device, 1, "Shadow Sync ") {
        extent      = { width, height };
        shadowImage = createImage (device, extent, format,
                                   VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                                   VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                   true, "Shadow Image");

        shadowMemory = allocateAndBindImageMemory (device, shadowImage);

        shadowImageView = createImageView (device, shadowImage, format,
                                           VK_IMAGE_ASPECT_DEPTH_BIT, "Shadow Image View");

        // Create a sampler for sampling the shadow map in the fragment shader

        shadowSampler = createSampler (device, "Shadow Sampler");

        shadowDescLayout =
        createDescriptorLayout (device, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                VK_SHADER_STAGE_FRAGMENT_BIT, 0);

        shadowDescPool =
        createDescriptorPool (device, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

        shadowDescSet = allocateDescriptorSet (device, shadowDescLayout, shadowDescPool,
                                               "Shadow Descriptor Set");

        writeImageSamplerInDescriptorSet (device, shadowImageView, shadowSampler, shadowDescSet);
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
        imageDrawer.draw (vertexBuffer, indexBuffer, meshPool,
                          drawCallMeshIndices, syncObjects, false, false, true, 0, 0);
    }
    void destroy () {
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
        if (shadowMemory != VK_NULL_HANDLE) {
            vkFreeMemory (pDevice.getDevice (), shadowMemory, nullptr);
            shadowMemory = VK_NULL_HANDLE;
        }
        if (shadowImage != VK_NULL_HANDLE) {
            vkDestroyImage (pDevice.getDevice (), shadowImage, nullptr);
            shadowImage = VK_NULL_HANDLE;
        }
        if (shadowImageView != VK_NULL_HANDLE) {
            vkDestroyImageView (pDevice.getDevice (), shadowImageView, nullptr);
            shadowImageView = VK_NULL_HANDLE;
        }
    }
};