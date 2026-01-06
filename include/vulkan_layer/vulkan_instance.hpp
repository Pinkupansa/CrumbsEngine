#pragma once
#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>
#include <string>
#include <vector>

VkApplicationInfo defaultAppInfo();

class VulkanInstance {
 private:
  VkInstance instance;
  VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

  static VKAPI_ATTR VkBool32 VKAPI_CALL
  debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                VkDebugUtilsMessageTypeFlagsEXT messageType,
                const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                void* pUserData);

  void setupDebugMessenger();

 public:
  VulkanInstance();

  ~VulkanInstance();

  void destroy();

  const VkInstance& getInstance() const;
};