#include "vulkan_layer/vulkan_render_texture.hpp"

#include "vulkan_layer/vulkan_attachment.hpp"
#include "vulkan_layer/vulkan_device.hpp"
#include "vulkan_layer/vulkan_object_creation_utils.hpp"
#include "vulkan_layer/vulkan_sync_objects.hpp"
#include "vulkan_layer/vulkan_texture_descriptor.hpp"
#include <functional>

VulkanRenderTexture::VulkanRenderTexture (const VulkanDevice& device,
                                          VkExtent2D extent,
                                          int nColorAttachments,
                                          std::string name)
: device (device), depthAttachment (device,
                                    VulkanAttachmentType::Depth,
                                    extent,
                                    false,
                                    false,
                                    createDepthClearValue ({ 1.0f, 0 }),
                                    name + " Depth Attachment",
                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                    true),
  syncObjects (device, 1, false, name + " Sync") {

    depthTextureDescriptor =
    new VulkanTextureDescriptor (device, depthAttachment.getImageView (),
                                 name + " Depth Texture Descriptor");
    for (int i = 0; i < nColorAttachments; i++) {
        colorAttachments.push_back (
        new VulkanAttachment (device, VulkanAttachmentType::Color, extent, false, false,
                              createColorClearValue (VkClearColorValue({ 0, 0, 0, 0 })), name + " Attachment",
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true));
        resolveAttachments.push_back (
        new VulkanAttachment (device, VulkanAttachmentType::Color, extent, true, true,
                              createColorClearValue (VkClearColorValue({ 0, 0, 0, 0 })), name + " Resolve Attachment",
                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, true));
        resolveTextureDescriptors.push_back (
        new VulkanTextureDescriptor (device, resolveAttachments[i]->getImageView (),
                                     name + " Resolve Texture Descriptor"));
    }
}

std::vector<VulkanAttachment*> VulkanRenderTexture::getColorAttachments () const {
    return colorAttachments;
}

VulkanRenderTexture::~VulkanRenderTexture () {
    destroy ();
}
std::function<void ()> VulkanRenderTexture::getFenceResetCallback () const {
    return std::bind (&VulkanRenderTexture::waitAndResetFences, this);
}

const VulkanSyncObjects* VulkanRenderTexture::getSyncObjects () const {
    return &syncObjects;
}

VulkanAttachment* VulkanRenderTexture::getColorAttachment (int index) {
    return colorAttachments[index];
}
VulkanAttachment* VulkanRenderTexture::getResolveAttachment (int index) {
    return resolveAttachments[index];
}
VulkanAttachment* VulkanRenderTexture::getDepthAttachment () {
    return &depthAttachment;
}

VulkanTextureDescriptor* VulkanRenderTexture::getDepthTextureDescriptor () {
    return depthTextureDescriptor;
}
VulkanTextureDescriptor* VulkanRenderTexture::getResolveTextureDescriptor (int index) {
    return resolveTextureDescriptors[index];
}
std::vector<VulkanAttachment*> VulkanRenderTexture::getResolveAttachments () const {
    return resolveAttachments;
}
std::vector<VulkanTextureDescriptor*>
VulkanRenderTexture::getResolveTextureDescriptors () const {
    return resolveTextureDescriptors;
}
void VulkanRenderTexture::waitAndResetFences () const {
    vkWaitForFences (device.getDevice (), 1, &syncObjects.inFlightFence[0], VK_TRUE, UINT64_MAX);
    vkResetFences (device.getDevice (), 1, &syncObjects.inFlightFence[0]);
}

void VulkanRenderTexture::destroy () {
    syncObjects.destroy ();
    for (int i = 0; i < colorAttachments.size (); i++) {
        colorAttachments[i]->destroy ();

        delete colorAttachments[i];
    }
    colorAttachments.clear ();


    for (int i = 0; i < resolveAttachments.size (); i++) {
        resolveAttachments[i]->destroy ();
        resolveTextureDescriptors[i]->destroy ();
        delete resolveAttachments[i];
        delete resolveTextureDescriptors[i];
    }
    resolveAttachments.clear ();
    resolveTextureDescriptors.clear ();

    if (depthTextureDescriptor != nullptr) {
        depthTextureDescriptor->destroy ();
        delete depthTextureDescriptor;
        depthTextureDescriptor = nullptr;
    }
    depthAttachment.destroy ();
}
