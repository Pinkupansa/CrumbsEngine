#pragma once
#include <vulkan/vulkan.h>

#include <string>
#include <vector>

#include "fsb_object.hpp"

class VulkanDevice;
class VulkanBuffer;

std::vector<uint8_t> padFSBData(std::vector<FSBObject> ubos,
                                VkDeviceSize alignedSize, VkDeviceSize size);
class VulkanFSB {
 private:
  FSBObject prototype;
  std::vector<FSBObject> objects;

  VulkanBuffer* buffer;
  VkDeviceSize alignment;
  VkDeviceSize objectSize;
  VkDeviceSize objectAlignedSize;

 public:
  const FSBObject getDefaultObject() const;
  VulkanFSB(const VulkanDevice& device, std::string jsonFile, int uboSetNumber,
            VkDeviceSize alignment, std::string name);
  void computeObjectSize();
  int addObject();

  int addObject(const FSBObject& object);
  void setObjectAttribute(int objectIndex, const std::string& attributeName,
                          const UniformVariant& value);

  void pushToGPU();

  void clear();
  void destroy();
  VkDeviceSize getObjectSize();
  VulkanBuffer& getBuffer() const;

  void debugPrintPrototype() const;

  void debugPrintObject(int index) const;
  bool isEmpty();
};
