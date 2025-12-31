#include "vulkan_layer/vulkan_descriptor.hpp"

#include "vulkan_layer/vulkan_buffer.hpp"
#include "vulkan_layer/vulkan_device.hpp"
#include "vulkan_layer/vulkan_ubo.hpp"
#include "vulkan_layer/vulkan_object_creation_utils.hpp"
#include <stdexcept>

const VkDescriptorSetLayout& VulkanUBDescriptor::getLayout() const {
  return layout;
}

const VkDeviceSize& VulkanUBDescriptor::getAlignedObjectSize() const {
  return alignedObjectSize;
}

const bool VulkanUBDescriptor::isDynamic() const { return dynamic; }
const VulkanDescriptorData VulkanUBDescriptor::getDescData(int binding,
                                                           int set) const {
  return {descriptorSet, layout, pool, dynamic, alignedObjectSize, binding, set};
}

VulkanUBDescriptor::VulkanUBDescriptor(VulkanDevice& device,
                                       VulkanBuffer& uniformBuffer,
                                       VkShaderStageFlags stageFlags,
                                       VkDeviceSize unalignedObjectSize,
                                       std::string name)
    : device(device), dynamic(uniformBuffer.isDynamic()) {
  VkDescriptorType descriptorType = uniformBuffer.isDynamic()
                                        ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
                                        : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  alignedObjectSize = uniformBuffer.getAlignedObjectSize();
  // binding for the ubo
  layout = createDescriptorLayout(device, descriptorType, stageFlags, 0);

  pool = createDescriptorPool(device, descriptorType);

  descriptorSet = allocateDescriptorSet(device, layout, pool, name);
  writeBufferInDescriptorSet(device, uniformBuffer.getBuffer(), descriptorSet,
                             descriptorType, unalignedObjectSize);
}

VulkanUBDescriptor::~VulkanUBDescriptor() { destroy(); }

void VulkanUBDescriptor::destroy() {
  if (pool != VK_NULL_HANDLE) {
    vkDestroyDescriptorPool(device.getDevice(), pool, nullptr);
    pool = VK_NULL_HANDLE;
  }
  if (layout != VK_NULL_HANDLE) {
    vkDestroyDescriptorSetLayout(device.getDevice(), layout, nullptr);
    layout = VK_NULL_HANDLE;
  }
}

const VkDescriptorSet& VulkanUBDescriptor::getDescriptorSet() const {
  return descriptorSet;
}
