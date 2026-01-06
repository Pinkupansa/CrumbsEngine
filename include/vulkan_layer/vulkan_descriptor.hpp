#pragma once
#include <vulkan/vulkan.h>

#include <string>

#include "vulkan_descriptor_data.hpp"

class VulkanDevice;
class VulkanBuffer;

class VulkanUBDescriptor {
 private:
  VulkanDevice& device;
  VkDescriptorSetLayout layout{VK_NULL_HANDLE};
  VkDescriptorPool pool{VK_NULL_HANDLE};
  VkDescriptorSet descriptorSet{VK_NULL_HANDLE};
  VkDeviceSize alignedObjectSize;
  bool dynamic;

 public:
  const VkDescriptorSetLayout& getLayout() const;

  const VkDeviceSize& getAlignedObjectSize() const;

  bool isDynamic() const;
  const VulkanDescriptorData getDescData(int binding, int set) const;

  VulkanUBDescriptor(VulkanDevice& device, VulkanBuffer& uniformBuffer,
                     VkShaderStageFlags stageFlags,
                     VkDeviceSize unalignedObjectSize, std::string name);

  ~VulkanUBDescriptor();

  void destroy();

  const VkDescriptorSet& getDescriptorSet() const;
};