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

    public:
    std::vector<VkClearValue> getClearValues() const {
        return clearValues;
    }
    const VkRenderPass& getRenderPass () const {
        return renderPass;
    }

    VulkanRenderPass (const VulkanDevice& device,
                      std::vector<VkAttachmentDescription> colorAttachments,
                      std::vector<VkClearValue> colorClearValues,
                      std::vector<VkAttachmentDescription> depthAttachments,
                      std::vector<VkClearValue> depthClearValues)
    : pDevice (device) {
        int attachmentCount = 0;
        clearValues = colorClearValues; 
        clearValues.insert(clearValues.end(), depthClearValues.begin(), depthClearValues.end());
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


        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount    = colorAttachmentsRefs.size ();
        subpass.pColorAttachments       = colorAttachmentsRefs.data ();
        subpass.pDepthStencilAttachment = depthAttachmentsRefs.data ();

        std::vector<VkAttachmentDescription> attachments = colorAttachments;
        attachments.insert (attachments.end (), depthAttachments.begin (),
                            depthAttachments.end ());

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
