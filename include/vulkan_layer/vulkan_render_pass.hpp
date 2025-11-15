#pragma once
#include <vulkan/vulkan.h>
#include "vulkan_device.hpp"
#include "vulkan_swapchain.hpp"

class VulkanRenderPass {
private: 
    VulkanDevice* pDevice;
    VkRenderPass renderPass;
    bool colorAttachment;
    bool depthAttachment;
public:
    
    VkRenderPass getRenderPass(){
        return renderPass;
    }

    bool hasColorAttachment(){
        return colorAttachment;
    }

    bool hasDepthAttachment(){
        return depthAttachment;
    }


    VulkanRenderPass(VulkanDevice& device, VkFormat colorFormat, VkFormat depthFormat,
                 bool _hasColorAttachment, bool _hasDepthAttachment,
                 VkImageLayout depthFinalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    
    colorAttachment = _hasColorAttachment;
    depthAttachment = _hasDepthAttachment;
    pDevice = &device;

    std::vector<VkAttachmentDescription> attachments;
    VkAttachmentReference colorAttachmentRef{};
    VkAttachmentReference depthAttachmentRef{};

    if(_hasColorAttachment){
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = colorFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        colorAttachment.flags = 0;

        colorAttachmentRef.attachment = static_cast<uint32_t>(attachments.size()); // will be 0
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        attachments.push_back(colorAttachment);
    }

    if(_hasDepthAttachment){
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = depthFormat;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;

        depthAttachment.storeOp = (depthFinalLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
                                     ? VK_ATTACHMENT_STORE_OP_STORE
                                     : VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = depthFinalLayout;

        depthAttachmentRef.attachment = static_cast<uint32_t>(attachments.size()); // either 0 or 1
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        attachments.push_back(depthAttachment);
    }

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    if(_hasColorAttachment){
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
    } else {
        subpass.colorAttachmentCount = 0;
        subpass.pColorAttachments = nullptr;
    }

    if(_hasDepthAttachment)
        subpass.pDepthStencilAttachment = &depthAttachmentRef;
    else
        subpass.pDepthStencilAttachment = nullptr;

    // Add a simple external dependency to handle layout transitions between passes
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
    dependency.dependencyFlags = 0;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device.getDevice(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass!");
    }

    device.nameObject((uint64_t)renderPass, VK_OBJECT_TYPE_RENDER_PASS, "RenderPass");
}

    ~VulkanRenderPass(){
        destroy();
    }

    void destroy(){
        if(renderPass != VK_NULL_HANDLE){
            vkDestroyRenderPass(pDevice->getDevice(), renderPass, nullptr);
            renderPass = VK_NULL_HANDLE;
        }
    }

};
