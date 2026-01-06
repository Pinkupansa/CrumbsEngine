#include "vulkan_layer/vulkan_buffer.hpp"

#include "vulkan_layer/vulkan_device.hpp"
#include "vulkan_layer/vulkan_object_creation_utils.hpp"
#include "vulkan_layer/vulkan_vertex.hpp"
#include <cstring>
#include <stdexcept>
#include <vector>

const VkBuffer& VulkanBuffer::getBuffer() const { return buffer; }

bool VulkanBuffer::isDynamic() const { return dynamic; }

VkDeviceSize VulkanBuffer::getAlignedObjectSize() const {
  return alignedObjectSize;
}

VulkanBuffer::VulkanBuffer(const VulkanDevice& deviceRef, VulkanBufferType type,
                           VkDeviceSize size, const void* data, bool dynamic,
                           VkDeviceSize alignedObjectSize, std::string name)
    : device(deviceRef),
      type(type),
      dynamic(dynamic),
      alignedObjectSize(alignedObjectSize) {
  buffer = createBuffer(device, type, size, name);

  memory = allocateAndBindBufferMemory(device, buffer);

  if (data) update(data, size, 0);
}

VulkanBuffer::~VulkanBuffer() { destroy(); }

void VulkanBuffer::destroy() {
  if (buffer != VK_NULL_HANDLE)
    vkDestroyBuffer(device.getDevice(), buffer, nullptr);
  if (memory != VK_NULL_HANDLE)
    vkFreeMemory(device.getDevice(), memory, nullptr);
  buffer = VK_NULL_HANDLE;
  memory = VK_NULL_HANDLE;
}

void VulkanBuffer::update(const void* data, VkDeviceSize size,
                          VkDeviceSize offset) {
  void* mapped;
  vkMapMemory(device.getDevice(), memory, offset, size, 0, &mapped);
  std::memcpy(mapped, data, static_cast<size_t>(size));
  vkUnmapMemory(device.getDevice(), memory);
}


