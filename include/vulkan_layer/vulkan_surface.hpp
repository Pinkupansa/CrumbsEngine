#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "vulkan_device.hpp"
#include "vulkan_instance.hpp"

class VulkanSurface {
 private:
  const VulkanInstance& pInstance;

  VkSurfaceKHR surface;
  VkSurfaceCapabilitiesKHR surfaceCapabilities;

 public:
  const VkSurfaceKHR& getSurface() const;

  const VkSurfaceCapabilitiesKHR& getCapabilities() const;

  VulkanSurface(const VulkanInstance& instance, const VulkanDevice& device,
                GLFWwindow* window);

  ~VulkanSurface();
  void destroy();
};
