#pragma once
#include <vector>
#include <vulkan/vulkan.h>

#include "vulkan_attachment.hpp"
#include "vulkan_device.hpp"

class VulkanRenderPass {
    private:
    const VulkanDevice& pDevice;
    VkRenderPass renderPass;

    std::vector<VkClearValue> clearValues;
    bool hasResolve;
    bool isFirstPass;

    public:
    std::vector<VkClearValue> getClearValues () const;
    const VkRenderPass& getRenderPass () const;

    const bool hasResolveAttachment () const; // used to set nRasterizationSamples
                                              // in pipeline

    VkAttachmentLoadOp getLoadOp ();
    VulkanRenderPass (const VulkanDevice& device,
                      const std::vector<VulkanAttachment*>& attachments);
    ~VulkanRenderPass ();

    void destroy ();
};