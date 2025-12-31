#include "vulkan_layer/vulkan_sync_objects.hpp"

#include "vulkan_layer/vulkan_device.hpp"
#include <stdexcept>

VkFence VulkanSyncObjects::getFence(int syncIndex) const {
  return inFlightFence[syncIndex];
}
bool VulkanSyncObjects::hasWaitSemaphore(bool isFirstPass) const {
  return !isFirstPass || isSwapchain;
}

void VulkanSyncObjects::setCurrentFrame(uint32_t newFrame) {
  currentFrame = newFrame;
}
void VulkanSyncObjects::updateSyncIndex() {
  currentSyncIndex = (currentSyncIndex + 1) % imageAvailableSemaphore.size();
}

uint32_t VulkanSyncObjects::getCurrentFrame() const { return currentFrame; }

int VulkanSyncObjects::getSyncIndex() const { return currentSyncIndex; }

const VkSemaphore* VulkanSyncObjects::getWaitSemaphore(int syncIndex,
                                                       bool isFirstPass) const {
  if (isFirstPass) {
    if (isSwapchain) {
      return &imageAvailableSemaphore[syncIndex];
    } else {
      return VK_NULL_HANDLE;
    }
  } else {
    return &renderFinishedSemaphore[syncIndex];
  }
}
bool VulkanSyncObjects::hasSignalSemaphore(bool isFirstPass) const {
  return isSwapchain;
}
const VkSemaphore* VulkanSyncObjects::getSignalSemaphore(int syncIndex) const {
  return &renderFinishedSemaphore[syncIndex];
}
VulkanSyncObjects::VulkanSyncObjects(const VulkanDevice& device,
                                     int imageCount, bool isSwapchain,
                                     std::string name)
    : pDevice(device),
      isSwapchain(isSwapchain),
      currentSyncIndex(0),
      currentFrame(0) {
  imageAvailableSemaphore.resize(imageCount);
  renderFinishedSemaphore.resize(imageCount);
  inFlightFence.resize(imageCount);

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags =
      VK_FENCE_CREATE_SIGNALED_BIT;  // Start signaled so first frame runs
                                     // immediately

  for (int i = 0; i < imageCount; i++) {
    if (vkCreateSemaphore(device.getDevice(), &semaphoreInfo, nullptr,
                          &imageAvailableSemaphore[i]) != VK_SUCCESS ||
        vkCreateSemaphore(device.getDevice(), &semaphoreInfo, nullptr,
                          &renderFinishedSemaphore[i]) != VK_SUCCESS ||
        vkCreateFence(device.getDevice(), &fenceInfo, nullptr,
                      &inFlightFence[i]) != VK_SUCCESS) {
      throw std::runtime_error("Failed to create synchronization objects!");
    }

    device.nameObject(
        (uint64_t)imageAvailableSemaphore[i], VK_OBJECT_TYPE_SEMAPHORE,
        name + " Image Available " + std::to_string(i));
    device.nameObject(
        (uint64_t)renderFinishedSemaphore[i], VK_OBJECT_TYPE_SEMAPHORE,
        name + " Render Finished " + std::to_string(i));
    device.nameObject((uint64_t)inFlightFence[i], VK_OBJECT_TYPE_FENCE,
                      name + " In Flight " + std::to_string(i));
  }
}

VulkanSyncObjects::~VulkanSyncObjects() { destroy(); }

void VulkanSyncObjects::destroy() {
  for (size_t i = 0; i < inFlightFence.size(); ++i) {
    if (inFlightFence[i] != VK_NULL_HANDLE) {
      vkDestroyFence(pDevice.getDevice(), inFlightFence[i], nullptr);
      inFlightFence[i] = VK_NULL_HANDLE;
    }
    if (renderFinishedSemaphore[i] != VK_NULL_HANDLE) {
      vkDestroySemaphore(pDevice.getDevice(), renderFinishedSemaphore[i],
                         nullptr);
      renderFinishedSemaphore[i] = VK_NULL_HANDLE;
    }
    if (imageAvailableSemaphore[i] != VK_NULL_HANDLE) {
      vkDestroySemaphore(pDevice.getDevice(), imageAvailableSemaphore[i],
                         nullptr);
      imageAvailableSemaphore[i] = VK_NULL_HANDLE;
    }
  }
}
