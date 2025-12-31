#include "vulkan_layer/vulkan_framebuffers.hpp"

#include "vulkan_layer/vulkan_device.hpp"
#include "vulkan_layer/vulkan_object_creation_utils.hpp"
#include "vulkan_layer/vulkan_render_pass.hpp"

#include <vector>

const std::vector<VkFramebuffer>& VulkanFramebuffers::getFramebuffers() const {
  return framebuffers;
}

VulkanFramebuffers::VulkanFramebuffers(
    const VulkanDevice& dev, const VulkanRenderPass& rp,
    const std::vector<std::vector<VulkanAttachment*>>&
        attachmentsPerFramebuffers,
    std::string name)
    : device(dev) {
  framebuffers.resize(attachmentsPerFramebuffers.size());

  for (size_t i = 0; i < framebuffers.size(); ++i) {
    std::vector<VulkanAttachment*> attachments = attachmentsPerFramebuffers[i];
    std::vector<VkImageView> imageViews;
    for (auto& attachment : attachments) {
      imageViews.push_back(attachment->getImageView());
    }
    // all attachments should have same extent
    framebuffers[i] = createFramebuffer(
        device, rp.getRenderPass(), imageViews, attachments[0]->getExtent(), name);
  }
}

VulkanFramebuffers::~VulkanFramebuffers() { destroy(); }

void VulkanFramebuffers::destroy() {
  for (auto& fb : framebuffers) {
    if (fb != VK_NULL_HANDLE) {
      vkDestroyFramebuffer(device.getDevice(), fb, nullptr);
      fb = VK_NULL_HANDLE;
    }
  }
  framebuffers.clear();
}

size_t VulkanFramebuffers::size() const { return framebuffers.size(); }
