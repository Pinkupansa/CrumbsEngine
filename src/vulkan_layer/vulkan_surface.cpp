#include "vulkan_layer/vulkan_surface.hpp"

#include "vulkan_layer/vulkan_device.hpp"
#include "vulkan_layer/vulkan_instance.hpp"

const VkSurfaceKHR& VulkanSurface::getSurface() const { return surface; }

const VkSurfaceCapabilitiesKHR& VulkanSurface::getCapabilities() const {
  return surfaceCapabilities;
}

VulkanSurface::VulkanSurface(const VulkanInstance& instance,
                             const VulkanDevice& device, GLFWwindow* window)
    : pInstance(instance) {
  // Creation of the window tied to the instance
  if (glfwCreateWindowSurface(instance.getInstance(), window, nullptr, &surface) !=
      VK_SUCCESS)
    throw std::runtime_error("Failed to create window surface!");

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.getPhysicalDevice(), surface,
                                            &surfaceCapabilities);
  device.nameObject((uint64_t)surface, VK_OBJECT_TYPE_SURFACE_KHR, "Main Surface");
}

VulkanSurface::~VulkanSurface() { destroy(); }
void VulkanSurface::destroy() {
  if (surface != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(pInstance.getInstance(), surface, nullptr);
    surface = VK_NULL_HANDLE;
  }
}
