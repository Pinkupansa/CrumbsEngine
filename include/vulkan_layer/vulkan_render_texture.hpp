#pragma once
#include <vulkan/vulkan.h>
#include <functional>

#include "vulkan_attachment.hpp"
#include "vulkan_sync_objects.hpp"
#include "vulkan_texture_descriptor.hpp"

// A texture that can be renderered to and sampled in a shader
class VulkanRenderTexture {
 private:
  const VulkanDevice& device;
  VulkanAttachment colorAttachment;
  VulkanAttachment resolveAttachment;
  VulkanTextureDescriptor textureDescriptor;
  VulkanSyncObjects syncObjects;

 public:
  VulkanRenderTexture(const VulkanDevice& device, VkExtent2D extent,
                      std::string name);

  ~VulkanRenderTexture();
  std::function<void()> getFenceResetCallback() const;

  const VulkanSyncObjects* getSyncObjects() const;

  VulkanAttachment* getColorAttachment();
  VulkanAttachment* getResolveAttachment();
  VulkanTextureDescriptor& getTextureDescriptor();
  void waitAndResetFences() const;

  void destroy();
};
