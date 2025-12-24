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
    bool isFirstPass;
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

    VkAttachmentLoadOp getLoadOp(){
        return isFirstPass? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    }
    VulkanRenderPass (const VulkanDevice& device,
                      const std::vector<VulkanAttachment>& attachments, 
                      bool isFirstPass)
    : pDevice (device) {

        std::vector<VkAttachmentDescription> colorAttachmentDescs;
        std::vector<VkAttachmentDescription> depthAttachmentDescs;
        std::vector<VkAttachmentDescription> resolveAttachmentDescs;

        for(auto& attachment : attachments){
            VkAttachmentDescription desc = attachment.createAttachmentDesc(getLoadOp());
            switch(attachment.getType()){
                case VulkanAttachmentType::Color:
                    if(attachment.isColorResolveAttachment()){
                        resolveAttachmentDescs.push_back(desc);
                    }
                    else{
                        colorAttachmentDescs.push_back(desc);
                    }
                    break;
                case VulkanAttachmentType::Depth:
                    depthAttachmentDescs.push_back(desc);
                    break;
            }
        }

        int attachmentCount = 0;
        hasResolve = resolveAttachmentDescs.size() > 0;
        for(int i = 0; i < colorAttachmentDescs.size(); i++){
            clearValues.push_back(createColorClearValue({0.1f, 0.1f, 0.1f, 0.1f}));
        }

        for(int i = 0; i < depthAttachmentDescs.size(); i++){
            clearValues.push_back(createDepthClearValue({1.0f, 0}));
        }
        for(int i = 0; i < resolveAttachmentDescs.size(); i++){
            clearValues.push_back(createColorClearValue({0.1f, 0.1f, 0.1f, 0.1f}));
        }
        std::vector<VkAttachmentReference> colorAttachmentsRefs;
        for (VkAttachmentDescription cAtt : colorAttachmentDescs) {
            colorAttachmentsRefs.push_back (createColorAttachmentRef (attachmentCount));
            attachmentCount++;
        }

        std::vector<VkAttachmentReference> depthAttachmentsRefs;
        for (VkAttachmentDescription dAtt : depthAttachmentDescs) {
            depthAttachmentsRefs.push_back (createDepthAttachmentRef (attachmentCount));
            attachmentCount++;
        }

        std::vector<VkAttachmentReference> resolveAttachmentsRefs;
        for (VkAttachmentDescription rAtt : depthAttachmentDescs) {
            resolveAttachmentsRefs.push_back (createColorAttachmentRef (attachmentCount));
            attachmentCount++;
        }
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount    = colorAttachmentsRefs.size ();
        subpass.pColorAttachments       = colorAttachmentsRefs.data ();
        subpass.pDepthStencilAttachment = depthAttachmentsRefs.data ();
        subpass.pResolveAttachments     = resolveAttachmentsRefs.data ();

        std::vector<VkAttachmentDescription> attachmentDescs = colorAttachmentDescs;
        attachmentDescs.insert (attachmentDescs.end (), depthAttachmentDescs.begin (),
                            depthAttachmentDescs.end ());
        attachmentDescs.insert (attachmentDescs.end (), resolveAttachmentDescs.begin (),
                            resolveAttachmentDescs.end ());

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = attachmentDescs.size ();
        renderPassInfo.pAttachments    = attachmentDescs.data ();
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
