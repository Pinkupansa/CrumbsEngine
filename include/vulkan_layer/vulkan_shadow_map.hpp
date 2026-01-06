#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <functional>

#include "vulkan_attachment.hpp"
#include "vulkan_device.hpp"
#include "vulkan_sync_objects.hpp"
#include "vulkan_texture_descriptor.hpp"

class VulkanShadowMap {
 private:
  VulkanDevice& pDevice;

  VulkanAttachment shadowAttachment;
  VulkanTextureDescriptor shadowTexture;
  VulkanSyncObjects syncObjects;

 public:
  std::vector<std::vector<VulkanAttachment*>> getAttachmentsPerFrameBuffer();

  VulkanAttachment* getShadowAttachment();

  const VulkanTextureDescriptor& getTexture();

  VulkanShadowMap(VulkanDevice& device, uint width, uint height);

  ~VulkanShadowMap();
  const std::function<void()> getFenceResetCallback();
  const VulkanSyncObjects& getSyncObjects() const;
  void waitAndResetFences() const;

  void destroy();
};
