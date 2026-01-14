#pragma once

#include <functional>
#include <string>
#include <vector>

#include "vulkan_layer/vulkan_command_buffers.hpp"
#include "vulkan_layer/vulkan_descriptor_data.hpp"
#include "vulkan_layer/vulkan_device.hpp"
#include "vulkan_layer/vulkan_framebuffers.hpp"
#include "vulkan_layer/vulkan_pipeline.hpp"
#include "vulkan_layer/vulkan_render_pass.hpp"
#include "vulkan_layer/vulkan_shader_data.hpp"
#include "vulkan_layer/vulkan_sync_objects.hpp"

/*class creating a render pass, a pipeline and dedicated command buffers and
 to be able to render on any compatible image on command.

takes in the shaders and a vector of vectors of imageviews (attachments)
*/

class VulkanImageDrawer {
 private:
  const VulkanSyncObjects& renderTargetSyncObjects;
  std::function<void()> renderTargetFenceResetCallback;

  const VulkanDevice& pDevice;
  const std::vector<VulkanDescriptorData> descriptors;
  VkExtent2D imageExtent;

  VulkanRenderPass renderPass;
  // Pipeline and framebuffers
  VulkanPipeline graphicsPipeline;
  VulkanFramebuffers framebuffers;

  VulkanCommandBuffers commandBuffers;
  std::vector<VkPipelineStageFlags> waitStages;

  bool isFirstPass;
  bool isFullScreenShader;
  std::string name;

 public:
  VulkanImageDrawer(
      const VulkanDevice& device, VkExtent2D extent,
      const std::vector<std::vector<VulkanAttachment*>>&
          attachmentsPerFramebuffer,
      bool isFirstPass, const std::vector<VulkanDescriptorData>& descriptors,
      const VulkanSyncObjects& renderTargetSyncObjects,
      std::function<void()> renderTargetFenceResetCallback,
      std::vector<std::string> vertShaderPaths,
      std::vector<std::string> fragShaderPaths, VkCullModeFlagBits cullMode,

      bool enableDepthTest, bool enableDepthWrite, bool isFullScreenShader,
      VulkanAlphaBlendMode alphaBlendMode, std::string name);

  void draw(const VulkanBuffer& vertexBuffer, const VulkanBuffer& indexBuffer,
            const std::vector<MeshDrawInfo>& meshPool,
            const std::vector<uint32_t>& drawCallMeshIndices, ImDrawData* drawData = nullptr);

  void destroy();

  VulkanRenderPass& getRenderPass() { return renderPass; }
};
