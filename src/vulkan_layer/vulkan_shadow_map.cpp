#include "vulkan_layer/vulkan_shadow_map.hpp"

#include "vulkan_layer/vulkan_attachment.hpp"
#include "vulkan_layer/vulkan_descriptor_data.hpp"
#include "vulkan_layer/vulkan_device.hpp"
#include "vulkan_layer/vulkan_object_creation_utils.hpp"
#include "vulkan_layer/vulkan_sync_objects.hpp"
#include "vulkan_layer/vulkan_texture_descriptor.hpp"
#include <functional>

std::vector<std::vector<VulkanAttachment*>>
VulkanShadowMap::getAttachmentsPerFrameBuffer() {
  return {{&shadowAttachment}};
}

VulkanAttachment* VulkanShadowMap::getShadowAttachment() {
  return &shadowAttachment;
}

const VulkanTextureDescriptor& VulkanShadowMap::getTexture() {
  return shadowTexture;
}

VulkanShadowMap::VulkanShadowMap(VulkanDevice& device, uint width, uint height,
                                 VkFormat format)
    : pDevice(device),
      syncObjects(device, 1, false, "Shadow Sync "),
      shadowAttachment(device, VulkanAttachmentType::ShadowMap,
                       {width, height}, true, true, createDepthClearValue({1.0f, 0}),
                       "Shadow Attachment "),
      shadowTexture(device, shadowAttachment.getImageView(),
                    "Shadow Texture Sampler ") {}
VulkanShadowMap::~VulkanShadowMap() { destroy(); }
const std::function<void()> VulkanShadowMap::getFenceResetCallback() {
  return std::bind(&VulkanShadowMap::waitAndResetFences, this);
}
const VulkanSyncObjects& VulkanShadowMap::getSyncObjects() const {
  return syncObjects;
}
void VulkanShadowMap::waitAndResetFences() const {
  vkWaitForFences(pDevice.getDevice(), 1, &syncObjects.inFlightFence[0],
                  VK_TRUE, UINT64_MAX);
  vkResetFences(pDevice.getDevice(), 1, &syncObjects.inFlightFence[0]);
}

void VulkanShadowMap::destroy() {
  syncObjects.destroy();
  shadowTexture.destroy();
  shadowAttachment.destroy();
}
