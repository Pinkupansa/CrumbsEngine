#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <string>

#include "vulkan_device.hpp"

class VulkanSyncObjects {
 private:
  const VulkanDevice& pDevice;

 public:
  std::vector<VkSemaphore> imageAvailableSemaphore;
  std::vector<VkSemaphore> renderFinishedSemaphore;
  std::vector<VkFence> inFlightFence;

  bool isSwapchain;  // hacky, to change
  int currentSyncIndex;
  uint32_t currentFrame;
  VkFence getFence(int syncIndex) const;
  bool hasWaitSemaphore(bool isFirstPass) const;

  void setCurrentFrame(uint32_t newFrame);
  void updateSyncIndex();

  uint32_t getCurrentFrame() const;

  int getSyncIndex() const;

  const VkSemaphore* getWaitSemaphore(int syncIndex, bool isFirstPass) const;
  bool hasSignalSemaphore(bool isFirstPass) const;
  const VkSemaphore* getSignalSemaphore(int syncIndex) const;
  VulkanSyncObjects(const VulkanDevice& device, int imageCount,
                    bool isSwapchain, std::string name = "Swapchain Sync ");

  ~VulkanSyncObjects();

  void destroy();
};