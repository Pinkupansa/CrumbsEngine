#pragma once
#include <vulkan/vulkan.h>

#include <string>
#include <vector>

#include "vulkan_attachment.hpp"
#include "vulkan_device.hpp"
#include "vulkan_render_pass.hpp"

class VulkanFramebuffers {
 private:
  const VulkanDevice& device;
  std::vector<VkFramebuffer> framebuffers;

 public:
  const std::vector<VkFramebuffer>& getFramebuffers() const;

  VulkanFramebuffers(
      const VulkanDevice& dev, const VulkanRenderPass& rp,
      const std::vector<std::vector<VulkanAttachment*>>&
          attachmentsPerFramebuffers,
      std::string name);

  ~VulkanFramebuffers();

  void destroy();

  size_t size() const;
};