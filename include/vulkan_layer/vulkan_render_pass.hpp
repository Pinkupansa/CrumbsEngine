#pragma once
#include "vulkan_constants.hpp"
#include "vulkan_device.hpp"
#include "vulkan_object_creation_utils.hpp"
#include <vulkan/vulkan.h>
class VulkanRenderPass {
    private:
    const VulkanDevice& pDevice;
    VkRenderPass renderPass;

    std::vector<VkClearValue> clearValues;
    bool hasResolve;
    public:
    std::vector<VkClearValue> getClearValues () const {
        return clearValues;
    }
    const VkRenderPass& getRenderPass () const {
        return renderPass;
    }

    const bool hasResolveAttachment() const{ //used to set nRasterizationSamples in pipeline
        return hasResolve;
    }
    VulkanRenderPass (const VulkanDevice& device,
                      std::vector<VkAttachmentDescription> colorAttachments,
                      std::vector<VkAttachmentDescription> depthAttachments,
                      std::vector<VkAttachmentDescription> resolveAttachments)
    : pDevice (device) {
        int attachmentCount = 0;
        hasResolve = resolveAttachments.size() > 0;
        for(int i = 0; i < colorAttachments.size(); i++){
            clearValues.push_back(createColorClearValue({0.1f, 0.1f, 0.1f, 0.1f}));
        }

        for(int i = 0; i < depthAttachments.size(); i++){
            clearValues.push_back(createDepthClearValue({1.0f, 0}));
        }
        for(int i = 0; i < resolveAttachments.size(); i++){
            clearValues.push_back(createColorClearValue({0.1f, 0.1f, 0.1f, 0.1f}));
        }
        std::vector<VkAttachmentReference> colorAttachmentsRefs;
        for (VkAttachmentDescription cAtt : colorAttachments) {
            colorAttachmentsRefs.push_back (createColorAttachmentRef (attachmentCount));
            attachmentCount++;
        }

        std::vector<VkAttachmentReference> depthAttachmentsRefs;
        for (VkAttachmentDescription dAtt : depthAttachments) {
            depthAttachmentsRefs.push_back (createDepthAttachmentRef (attachmentCount));
            attachmentCount++;
        }

        std::vector<VkAttachmentReference> resolveAttachmentsRefs;
        for (VkAttachmentDescription rAtt : depthAttachments) {
            resolveAttachmentsRefs.push_back (createColorAttachmentRef (attachmentCount));
            attachmentCount++;
        }
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount    = colorAttachmentsRefs.size ();
        subpass.pColorAttachments       = colorAttachmentsRefs.data ();
        subpass.pDepthStencilAttachment = depthAttachmentsRefs.data ();
        subpass.pResolveAttachments     = resolveAttachmentsRefs.data ();

        std::vector<VkAttachmentDescription> attachments = colorAttachments;
        attachments.insert (attachments.end (), depthAttachments.begin (),
                            depthAttachments.end ());
        attachments.insert (attachments.end (), resolveAttachments.begin (),
                            resolveAttachments.end ());

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = attachments.size ();
        renderPassInfo.pAttachments    = attachments.data ();
        renderPassInfo.subpassCount    = 1;
        renderPassInfo.pSubpasses      = &subpass;

        if (vkCreateRenderPass (device.getDevice (), &renderPassInfo, nullptr,
                                &renderPass) != VK_SUCCESS) {
            throw std::runtime_error ("Failed to create render pass!");
        }

        device.nameObject ((uint64_t)renderPass, VK_OBJECT_TYPE_RENDER_PASS, "RenderPass");
    }

    ~VulkanRenderPass () {
        destroy ();
    }

    void destroy () {
        if (renderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass (pDevice.getDevice (), renderPass, nullptr);
            renderPass = VK_NULL_HANDLE;
        }
    }
};
