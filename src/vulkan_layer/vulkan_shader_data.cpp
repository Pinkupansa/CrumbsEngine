#include "vulkan_layer/vulkan_shader_data.hpp"

#include "vulkan_layer/vulkan_attachment.hpp"
#include "vulkan_layer/vulkan_render_texture.hpp"
#include "vulkan_layer/vulkan_texture_descriptor.hpp"
#include <functional>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

VulkanShaderData::VulkanShaderData(
    std::string filePath, bool enableDepthTest, bool enableDepthWrite,
    VulkanAlphaBlendMode alphaBlendMode, bool isFullScreenShader,
    std::vector<VulkanAttachment*> customColorAttachments,
    VulkanAttachment* customDepthAttachment,
    std::vector<VulkanAttachment*> customResolveAttachments,
    VulkanSyncObjects* customRenderTargetSyncObjects,
    VkCullModeFlagBits customCullMode,
    std::function<void()> customRenderTargetFenceResetCallback,
    std::vector<VulkanTextureDescriptor*> customTextureDescriptors)
    : filePath(filePath),
      enableDepthTest(enableDepthTest),
      enableDepthWrite(enableDepthWrite),
      colorAttachments(customColorAttachments),
      depthAttachment(customDepthAttachment),
      resolveAttachments(customResolveAttachments),
      renderTargetSyncObjects(customRenderTargetSyncObjects),
      renderTargetFenceResetCallback(customRenderTargetFenceResetCallback),
      textureDescriptors(customTextureDescriptors),
      alphaBlendMode(alphaBlendMode),
      isFullScreenShader(isFullScreenShader),
      cullMode(customCullMode) {}

void VulkanShaderData::bindTargetRenderTexture(
    VulkanRenderTexture& renderTexture) {
  colorAttachments = renderTexture.getColorAttachments();
  depthAttachment = renderTexture.getDepthAttachment();
  resolveAttachments = renderTexture.getResolveAttachments();
  renderTargetSyncObjects = renderTexture.getSyncObjects();
  renderTargetFenceResetCallback = renderTexture.getFenceResetCallback();
}

void VulkanShaderData::bindSourceRenderTexture(
    VulkanRenderTexture& renderTexture) {
  textureDescriptors = renderTexture.getResolveTextureDescriptors();
  //textureDescriptors.push_back(renderTexture.getDepthTextureDescriptor());
  //insert resolvedescriptors
}
