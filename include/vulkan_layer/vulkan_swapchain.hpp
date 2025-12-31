#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <functional>

#include "vulkan_attachment.hpp"
#include "vulkan_device.hpp"
#include "vulkan_surface.hpp"
#include "vulkan_sync_objects.hpp"

class VulkanSwapchain {
 private:
  VulkanDevice& pDevice;
  VkSwapchainKHR swapchain;

  std::vector<VkImage> swapchainImages;
  std::vector<VulkanAttachment*> swapchainAttachments;

  std::vector<VulkanAttachment*> msaaColorAttachments;

  VulkanAttachment* depthAttachment;
  VulkanSyncObjects syncObjects;

 public:
  const VkSwapchainKHR& getSwapchain() const;

  const std::function<void()> getFenceResetCallback();
  VulkanAttachment* getDepthAttachment() const;

  const VulkanSyncObjects& getSyncObjects();

  std::vector<std::vector<VulkanAttachment*>> getAttachmentsPerFrameBuffer();
  VulkanSwapchain(VulkanDevice& device, VulkanSurface& surface);

  void present();

  void updateFrameIndex();

  void waitAndResetFences() const;

  ~VulkanSwapchain();

  void destroy();
};