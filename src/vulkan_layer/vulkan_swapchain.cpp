#include "vulkan_layer/vulkan_swapchain.hpp"

#include "vulkan_layer/vulkan_attachment.hpp"
#include "vulkan_layer/vulkan_device.hpp"
#include "vulkan_layer/vulkan_image_drawer.hpp"
#include "vulkan_layer/vulkan_object_creation_utils.hpp"
#include "vulkan_layer/vulkan_surface.hpp"
#include "vulkan_layer/vulkan_sync_objects.hpp"
#include <functional>
#include <vector>

const VkSwapchainKHR& VulkanSwapchain::getSwapchain() const { return swapchain; }

const std::function<void()> VulkanSwapchain::getFenceResetCallback() {
  return std::bind(&VulkanSwapchain::waitAndResetFences, this);
}
VulkanAttachment* VulkanSwapchain::getDepthAttachment() const {
  return depthAttachment;
}

const VulkanSyncObjects& VulkanSwapchain::getSyncObjects() { return syncObjects; }

std::vector<std::vector<VulkanAttachment*>>
VulkanSwapchain::getAttachmentsPerFrameBuffer() {
  return {{msaaColorAttachments[0], depthAttachment, swapchainAttachments[0]},
          {msaaColorAttachments[1], depthAttachment, swapchainAttachments[1]},
          {msaaColorAttachments[2], depthAttachment, swapchainAttachments[2]}};
}
VulkanSwapchain::VulkanSwapchain(VulkanDevice& device, VulkanSurface& surface)
    : pDevice(device), syncObjects(device, 3, true) {
  swapchain = createTripleBufferingSwapchain(device, surface, "Main Swapchain");

  // get extent

  // 1. Get swapchain images (3 because triple buffering)
  uint32_t imageCount = 0;
  vkGetSwapchainImagesKHR(device.getDevice(), swapchain, &imageCount, nullptr);
  swapchainImages.resize(imageCount);

  vkGetSwapchainImagesKHR(device.getDevice(), swapchain, &imageCount,
                          swapchainImages.data());

  for (int i = 0; i < imageCount; i++) {
    VulkanAttachment* msaaColorAttachment = new VulkanAttachment(
        device, VulkanAttachmentType::Color, surface.getCapabilities().currentExtent,
        false, false, createColorClearValue({0, 0, 0, 0}),
        "MSAA Color Attachment " + std::to_string(i));
    msaaColorAttachments.push_back(msaaColorAttachment);
  }

  // 2. Create image views
  for (int i = 0; i < imageCount; i++) {
    VulkanAttachment* swapchainAttachment = new VulkanAttachment(
        device, VulkanAttachmentType::Color, swapchainImages[i],
        surface.getCapabilities().currentExtent, true, true,
        createColorClearValue({0, 0, 0, 0}),
        "Swapchain Attachment " + std::to_string(i));
    swapchainAttachments.push_back(swapchainAttachment);
  }

  depthAttachment = new VulkanAttachment(
      device, VulkanAttachmentType::Depth,
      surface.getCapabilities().currentExtent, false, false,
      createDepthClearValue({1.0f, 0}), "Main Depth Attachment ");
}

void VulkanSwapchain::present() {
  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores =
      &syncObjects.renderFinishedSemaphore[syncObjects.getSyncIndex()];
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = &swapchain;
  std::vector<uint32_t> imageIndices = {syncObjects.getCurrentFrame()};
  presentInfo.pImageIndices = imageIndices.data();
  vkQueuePresentKHR(pDevice.getGraphicsQueue(), &presentInfo);

  syncObjects.updateSyncIndex();
}

void VulkanSwapchain::updateFrameIndex() {
  uint32_t currentFrame = 0;
  vkAcquireNextImageKHR(
      pDevice.getDevice(), swapchain, UINT64_MAX,
      syncObjects.imageAvailableSemaphore[syncObjects.getSyncIndex()],
      VK_NULL_HANDLE, &currentFrame);
  syncObjects.setCurrentFrame(currentFrame);
}

void VulkanSwapchain::waitAndResetFences() const {
  vkWaitForFences(pDevice.getDevice(), 1,
                  &syncObjects.inFlightFence[syncObjects.currentSyncIndex],
                  VK_TRUE, UINT64_MAX);
  vkResetFences(pDevice.getDevice(), 1,
                &syncObjects.inFlightFence[syncObjects.currentSyncIndex]);
}

VulkanSwapchain::~VulkanSwapchain() { destroy(); }

void VulkanSwapchain::destroy() {
  syncObjects.destroy();

  for (int i = 0; i < swapchainImages.size(); i++) {
    swapchainAttachments[i]->destroy();
    delete swapchainAttachments[i];

    msaaColorAttachments[i]->destroy();
    delete msaaColorAttachments[i];
  }

  if (depthAttachment != nullptr) {
    depthAttachment->destroy();
    delete depthAttachment;
    depthAttachment = nullptr;
  }

  swapchainAttachments.clear();
  msaaColorAttachments.clear();
  swapchainImages.clear();
  if (swapchain != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(pDevice.getDevice(), swapchain, nullptr);
    swapchain = VK_NULL_HANDLE;
  }
}
