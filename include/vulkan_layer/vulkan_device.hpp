#pragma once
#include <vulkan/vulkan.h>
#include <string>

class VulkanInstance;

class VulkanDevice {
 private:
  VkPhysicalDevice physicalDevice;
  VkPhysicalDeviceProperties properties;
  VkDevice device;
  VkQueue graphicsQueue;
  uint32_t graphicsFamilyIndex;
  VkSurfaceCapabilitiesKHR surfaceCapabilities;

  VkCommandPool commandPool;

  PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = nullptr;
  PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT = nullptr;
  PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT = nullptr;
  PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT = nullptr;

 public:
  const VkDevice& getDevice() const;

  const VkPhysicalDevice& getPhysicalDevice() const;

  const VkPhysicalDeviceProperties& getProperties() const;

  const VkCommandPool& getCommandPool() const;

  const VkQueue& getGraphicsQueue() const;

  VulkanDevice(VulkanInstance& instance);

  ~VulkanDevice();

  void destroy();

  uint32_t findMemoryType(uint32_t typeFilter,
                          VkMemoryPropertyFlags properties) const;

  void nameObject(uint64_t vulkanObject, VkObjectType type, std::string name) const;
};