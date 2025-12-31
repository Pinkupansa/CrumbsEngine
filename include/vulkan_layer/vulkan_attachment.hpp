#pragma once
#include <vulkan/vulkan.h>
#include "vulkan_device.hpp"

enum VulkanAttachmentType { Depth, Color, ShadowMap };

class VulkanAttachment {
 private:
  const VulkanDevice& device;

  VulkanAttachmentType type;
  VkImage image;
  VkImageView imageView;
  VkDeviceMemory memory;

  VkExtent2D extent;

  VkImageLayout finalLayout;  // layout after render pass

  bool isImageCreator;
  bool isSinglesampled;
  bool isResolve;

 public:
  VkClearValue getClearValue() const;

  VkImageView getImageView() const;

  VkImage getImage() const;

  VulkanAttachmentType getType() const;

  bool isColorResolveAttachment() const;

  VkClearValue clearValue;
  VkAttachmentDescription createAttachmentDesc();
  VkExtent2D getExtent() const;
  bool alreadyBound = false;
  VulkanAttachment(const VulkanDevice& device, VulkanAttachmentType type,
                   VkExtent2D extent, bool isResolve, bool isSinglesampled,
                   VkClearValue clearValue, std::string name,
                   bool isSampleable = false,
                   VkImageLayout finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  VulkanAttachment(const VulkanDevice& device, VulkanAttachmentType type,
                   VkImage image, VkExtent2D extent, bool isResolve,
                   bool isSinglesampled, VkClearValue clearValue,
                   std::string name,
                   VkImageLayout finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
  ~VulkanAttachment();

  void destroy();
};
