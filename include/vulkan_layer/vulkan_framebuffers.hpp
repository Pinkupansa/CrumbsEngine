#pragma once
#include "vulkan_device.hpp"
#include "vulkan_render_pass.hpp"

#include <vector>
#include <vulkan/vulkan.h>

class VulkanFramebuffers {
    private:
    const VulkanDevice& device;
    std::vector<VkFramebuffer> framebuffers;

    public:
    const std::vector<VkFramebuffer>& getFramebuffers () const {
        return framebuffers;
    }

    VulkanFramebuffers (const VulkanDevice& dev,
                        const VulkanRenderPass& rp,
                        const std::vector<std::vector<VulkanAttachment>>& attachmentsPerFramebuffers,
                        std::string name)
    : device (dev) {

        framebuffers.resize (attachmentsPerFramebuffers.size ());

        for (size_t i = 0; i < framebuffers.size (); ++i) {
            std::vector<VulkanAttachment> attachments = attachmentsPerFramebuffers[i];
            std::vector<VkImageView> imageViews; 
            for(auto& attachment : attachments){
                imageViews.push_back(attachment.getImageView());
            }
            //all attachments should have same extent
            framebuffers[i] =
            createFramebuffer (device, rp.getRenderPass (), imageViews, attachments[0].getExtent(), name);
        }
    }

    ~VulkanFramebuffers () {
        destroy ();
    }

    void destroy () {
        for (auto& fb : framebuffers) {
            if (fb != VK_NULL_HANDLE) {
                vkDestroyFramebuffer (device.getDevice (), fb, nullptr);
                fb = VK_NULL_HANDLE;
            }
        }
        framebuffers.clear ();
    }

    size_t size () const {
        return framebuffers.size ();
    }
};
