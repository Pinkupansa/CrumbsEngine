#pragma once
#include "vulkan_attachment.hpp"
#include "vulkan_render_texture.hpp"
#include "vulkan_texture_descriptor.hpp"
#include <string>


enum VulkanAlphaBlendMode { None, Additive, Weighted };
class VulkanShaderData {
    public:
    std::string filePath;
    bool enableDepthTest;
    bool enableDepthWrite;
    VulkanAttachment* colorAttachment = nullptr; // leave unchanged to render on swapchain
    VulkanAttachment* resolveAttachment = nullptr;
    const VulkanSyncObjects* renderTargetSyncObjects;
    std::function<void ()> renderTargetFenceResetCallback;
    std::vector<VulkanTextureDescriptor*> textureDescriptors; // extra textures to sample
    VulkanAlphaBlendMode alphaBlendMode;
    VkCullModeFlagBits cullMode;
    bool isFullScreenShader;
    VulkanShaderData (std::string filePath,
                      bool enableDepthTest,
                      bool enableDepthWrite,
                      VulkanAlphaBlendMode alphaBlendMode,
                      bool isFullScreenShader                   = false,
                      VulkanAttachment* customColorAttachment   = nullptr,
                      VulkanAttachment* customResolveAttachment = nullptr,
                      VulkanSyncObjects* customRenderTargetSyncObjects = nullptr,
                      VkCullModeFlagBits customCullMode = VK_CULL_MODE_BACK_BIT,
                      std::function<void ()> customRenderTargetFenceResetCallback = nullptr,
                      std::vector<VulkanTextureDescriptor*> customTextureDescriptors = {})
    : filePath (filePath), enableDepthTest (enableDepthTest),
      enableDepthWrite (enableDepthWrite), colorAttachment (customColorAttachment),
      resolveAttachment (customResolveAttachment),
      renderTargetSyncObjects (customRenderTargetSyncObjects),
      renderTargetFenceResetCallback (customRenderTargetFenceResetCallback),
      textureDescriptors (customTextureDescriptors), alphaBlendMode (alphaBlendMode),
      isFullScreenShader (isFullScreenShader), cullMode (customCullMode) {
    }

    void bindTargetRenderTexture (VulkanRenderTexture& renderTexture) {
        colorAttachment                = renderTexture.getColorAttachment ();
        resolveAttachment              = renderTexture.getResolveAttachment ();
        renderTargetSyncObjects        = renderTexture.getSyncObjects ();
        renderTargetFenceResetCallback = renderTexture.getFenceResetCallback ();
    }

    void bindSourceRenderTexture (VulkanRenderTexture& renderTexture) {
        textureDescriptors.push_back (&(renderTexture.getTextureDescriptor ()));
    }
};