#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "vulkan_device.hpp"
#include "vulkan_swapchain.hpp"
#include "vulkan_render_pass.hpp"

class VulkanFramebuffers {
private:
    VulkanDevice& pDevice;

    std::vector<VkFramebuffer> framebuffers;
public:

    const std::vector<VkFramebuffer>& getFramebuffers() const{
        return framebuffers;
    }

    VulkanFramebuffers(VulkanDevice& dev, VulkanRenderPass& rp, int nFramebuffers, std::vector<std::vector<VkImageView>> attachmentsPerFramebuffer, VkExtent2D extent): pDevice(dev) {
        
        if(attachmentsPerFramebuffer.size() != nFramebuffers){
            throw std::runtime_error("Attachment array size not matching number of framebuffers !");
        }

        framebuffers.resize(nFramebuffers);
        
        for (size_t i = 0; i < nFramebuffers; ++i) {
            std::vector<VkImageView> attachments = attachmentsPerFramebuffer[i];
            VkFramebufferCreateInfo fbInfo{};
            fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fbInfo.renderPass = rp.getRenderPass();
            fbInfo.attachmentCount = attachments.size();
            fbInfo.pAttachments = attachments.data();
            fbInfo.width = extent.width;
            fbInfo.height = extent.height;
            fbInfo.layers = 1;

            if (vkCreateFramebuffer(pDevice.getDevice(), &fbInfo, nullptr, &framebuffers[i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to create framebuffer!");
            
            pDevice.nameObject((uint64_t) framebuffers[i], VK_OBJECT_TYPE_FRAMEBUFFER, "Framebuffer " + std::to_string(i));
        }
    
    }

    ~VulkanFramebuffers() {
        destroy();
    }

    void destroy(){
        for (auto& fb : framebuffers) {
            if (fb != VK_NULL_HANDLE) {
                vkDestroyFramebuffer(pDevice.getDevice(), fb, nullptr);
                fb = VK_NULL_HANDLE;
            }
        }
        framebuffers.clear();
    }
    size_t size() const { return framebuffers.size(); }
 
};
