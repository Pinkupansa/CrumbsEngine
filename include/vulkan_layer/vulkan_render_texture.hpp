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

  std::vector<VulkanAttachment*> colorAttachments;
  VulkanAttachment depthAttachment;
  std::vector<VulkanAttachment*> resolveAttachments;
  VulkanTextureDescriptor* depthTextureDescriptor;
  std::vector<VulkanTextureDescriptor*> resolveTextureDescriptors;
  VulkanSyncObjects syncObjects;

 public:
  VulkanRenderTexture(const VulkanDevice& device, VkExtent2D extent, int nColorAttachments,
                      std::string name);

  ~VulkanRenderTexture();
  std::function<void()> getFenceResetCallback() const;

  const VulkanSyncObjects* getSyncObjects() const;

  VulkanAttachment* getColorAttachment(int index);
  VulkanAttachment* getResolveAttachment(int index);
  VulkanAttachment* getDepthAttachment();
  VulkanTextureDescriptor* getColorTextureDescriptor(int index);
  VulkanTextureDescriptor* getDepthTextureDescriptor();
  VulkanTextureDescriptor* getResolveTextureDescriptor(int index);
  
  std::vector<VulkanAttachment*> getColorAttachments() const;
  std::vector<VulkanAttachment*> getResolveAttachments() const;

  std::vector<VulkanTextureDescriptor*> getResolveTextureDescriptors() const;


  void waitAndResetFences() const;

  void destroy();
};
