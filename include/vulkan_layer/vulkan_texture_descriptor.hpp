#pragma once
#include <vulkan/vulkan.h>
#include <string>

#include "vulkan_descriptor_data.hpp"
#include "vulkan_device.hpp"

// Holds necessary info to attach a texture to a shader.
// TODO : add option to create image from file ?
class VulkanTextureDescriptor {
 private:
  const VulkanDevice& device;
  VkSampler sampler;
  VkDescriptorSetLayout descLayout;
  VkDescriptorPool descPool;
  VkDescriptorSet descSet;

 public:
  const VulkanDescriptorData getDescData(int binding, int set) const;
  VulkanTextureDescriptor(const VulkanDevice& device, VkImageView imageView,
                          std::string name);

  ~VulkanTextureDescriptor();
  void destroy();
};
