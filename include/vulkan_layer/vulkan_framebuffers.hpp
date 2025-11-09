#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include "vulkan_device.hpp"
#include "vulkan_swapchain.hpp"
#include "vulkan_render_pass.hpp"

class VulkanFramebuffers {
private:
    VulkanDevice& device;
    VulkanSwapchain& swapchain;
    VulkanRenderPass& renderPass;

    std::vector<VkFramebuffer> framebuffers;
public:

    const std::vector<VkFramebuffer>& getFramebuffers() const{
        return framebuffers;
    }

    VulkanFramebuffers(VulkanDevice& dev, VulkanSwapchain& sc, VulkanRenderPass& rp)
        : device(dev), swapchain(sc), renderPass(rp) {
        
        framebuffers.resize(swapchain.getImageViews().size());
        
        for (size_t i = 0; i < swapchain.getImageViews().size(); ++i) {
            std::array<VkImageView, 2> attachments= {swapchain.getImageViews()[i], swapchain.getDepthView()};
            VkFramebufferCreateInfo fbInfo{};
            fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            fbInfo.renderPass = renderPass.getRenderPass();
            fbInfo.attachmentCount = attachments.size();
            fbInfo.pAttachments = attachments.data();
            fbInfo.width = swapchain.getExtent().width;
            fbInfo.height = swapchain.getExtent().height;
            fbInfo.layers = 1;

            if (vkCreateFramebuffer(device.getDevice(), &fbInfo, nullptr, &framebuffers[i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to create framebuffer!");
            
            device.nameObject((uint64_t) framebuffers[i], VK_OBJECT_TYPE_FRAMEBUFFER, "Framebuffer " + std::to_string(i));
        }
    
    }

    ~VulkanFramebuffers() {
        destroy();
    }

    void destroy(){
        for (auto& fb : framebuffers) {
            if (fb != VK_NULL_HANDLE) {
                vkDestroyFramebuffer(device.getDevice(), fb, nullptr);
                fb = VK_NULL_HANDLE;
            }
        }
        framebuffers.clear();
    }
    size_t size() const { return framebuffers.size(); }
 
};
