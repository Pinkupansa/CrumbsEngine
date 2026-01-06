#pragma once
#include <string>
#include <vector>
#include <functional>

#include "vulkan_attachment.hpp"
#include "vulkan_render_texture.hpp"
#include "vulkan_texture_descriptor.hpp"
#include <vulkan/vulkan.h>


enum VulkanAlphaBlendMode { None, Additive, Weighted };
class VulkanShaderData {
    public:
    std::string filePath;
    bool enableDepthTest;
    bool enableDepthWrite;
    std::vector<VulkanAttachment*> colorAttachments; // leave unchanged to render on swapchain
    VulkanAttachment* depthAttachment = nullptr;
    std::vector<VulkanAttachment*> resolveAttachments;
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
                      std::vector<VulkanAttachment*> customColorAttachment   = {},
                      VulkanAttachment* customDepthAttachment  = nullptr,
                      std::vector<VulkanAttachment*> customResolveAttachment = {},
                      VulkanSyncObjects* customRenderTargetSyncObjects = nullptr,
                      VkCullModeFlagBits customCullMode = VK_CULL_MODE_BACK_BIT,
                      std::function<void ()> customRenderTargetFenceResetCallback = nullptr,
                      std::vector<VulkanTextureDescriptor*> customTextureDescriptors = {});

    void bindTargetRenderTexture (VulkanRenderTexture& renderTexture);

    void bindSourceRenderTexture (VulkanRenderTexture& renderTexture);
};
