#pragma once
#include <vulkan/vulkan.h>
#include "vulkan_device.hpp"
#include "vulkan_swapchain.hpp"

class VulkanRenderPass {
private: 
    VulkanDevice* pDevice;
    VkRenderPass renderPass;
public:
    
    VkRenderPass getRenderPass(){
        return renderPass;
    }

    VulkanRenderPass(VulkanDevice& device, VulkanSwapchain& swapchain){
        pDevice = &device;
        VkAttachmentDescription colorAttachment;
        colorAttachment.format = swapchain.getFormat();       // same as swapchain
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;         // no MSAA for now
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;    // clear at start
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;  // store result for presentation
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // ready for presentation
        colorAttachment.flags = 0;
        
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;                        // index in the attachment array
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = swapchain.getDepthFormat(); // the same format as your depth image
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // depth not presented
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        

        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 1; // index of the depth attachment
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;

        std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = attachments.size();
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

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
