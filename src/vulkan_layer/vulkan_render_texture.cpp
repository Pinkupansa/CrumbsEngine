#include "vulkan_layer/vulkan_render_texture.hpp"

#include "vulkan_layer/vulkan_attachment.hpp"
#include "vulkan_layer/vulkan_device.hpp"
#include "vulkan_layer/vulkan_object_creation_utils.hpp"
#include "vulkan_layer/vulkan_sync_objects.hpp"
#include "vulkan_layer/vulkan_texture_descriptor.hpp"
#include <functional>

VulkanRenderTexture::VulkanRenderTexture(const VulkanDevice& device,
                                         VkExtent2D extent, std::string name)
    : device(device),
      colorAttachment(
          device, VulkanAttachmentType::Color, extent, false, false,
          createColorClearValue({0, 0, 0, 0}), name + " Attachment", true,
          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
      resolveAttachment(
          device, VulkanAttachmentType::Color, extent, true, true,
          createColorClearValue({0, 0, 0, 0}), name + " Resolve Attachment",
          true, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
      textureDescriptor(device, resolveAttachment.getImageView(),
                        name + " Texture Descriptor"),
      syncObjects(device, 1, false, name + " Sync") {}

VulkanRenderTexture::~VulkanRenderTexture() { destroy(); }
std::function<void()> VulkanRenderTexture::getFenceResetCallback() const {
  return std::bind(&VulkanRenderTexture::waitAndResetFences, this);
}

const VulkanSyncObjects* VulkanRenderTexture::getSyncObjects() const {
  return &syncObjects;
}

VulkanAttachment* VulkanRenderTexture::getColorAttachment() {
  return &colorAttachment;
}
VulkanAttachment* VulkanRenderTexture::getResolveAttachment() {
  return &resolveAttachment;
}
VulkanTextureDescriptor& VulkanRenderTexture::getTextureDescriptor() {
  return textureDescriptor;
}
void VulkanRenderTexture::waitAndResetFences() const {
  vkWaitForFences(device.getDevice(), 1, &syncObjects.inFlightFence[0],
                  VK_TRUE, UINT64_MAX);
  vkResetFences(device.getDevice(), 1, &syncObjects.inFlightFence[0]);
}

void VulkanRenderTexture::destroy() {
  syncObjects.destroy();
  textureDescriptor.destroy();
  colorAttachment.destroy();
  resolveAttachment.destroy();
}
