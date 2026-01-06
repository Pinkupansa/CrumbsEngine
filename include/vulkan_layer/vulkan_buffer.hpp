#pragma once
#include <vulkan/vulkan.h>

#include <string>

enum class VulkanBufferType { Vertex, Index, Uniform, Staging };

class VulkanBuffer {
 private:
  VkBuffer buffer{VK_NULL_HANDLE};
  VkDeviceMemory memory{VK_NULL_HANDLE};

  VkDeviceSize alignedObjectSize;
  bool dynamic;

 public:
  const VkBuffer& getBuffer() const;

  bool isDynamic() const;

  VkDeviceSize getAlignedObjectSize() const;

  VulkanBuffer(const class VulkanDevice& deviceRef, VulkanBufferType type,
               VkDeviceSize size, const void* data = nullptr,
               bool dynamic = false, VkDeviceSize alignedObjectSize = 0,
               std::string name = "Buffer");

  ~VulkanBuffer();

  void destroy();

  void update(const void* data, VkDeviceSize size, VkDeviceSize offset);

 private:
  const class VulkanDevice& device;
  VulkanBufferType type;
};