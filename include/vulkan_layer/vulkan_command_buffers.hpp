#pragma once
#include <vulkan/vulkan.h>
#include <vector>

class VulkanDevice;
class VulkanFramebuffers;
class VulkanBuffer;
class VulkanDescriptorData;
class VulkanPipeline;
class MeshDrawInfo;
class VulkanRenderPass;

class VulkanCommandBuffers {
 private:
  const VulkanDevice& pDevice;

 public:
  std::vector<VkCommandBuffer> commandBuffers;

  VulkanCommandBuffers(const VulkanDevice& device,
                       const VulkanFramebuffers& framebuffers);

  ~VulkanCommandBuffers();

  void destroy();

  void record(VkExtent2D renderAreaExtent,
              const VulkanRenderPass& renderPass,
              const VulkanFramebuffers& framebuffers,
              const VulkanBuffer& vertexBuffer, const VulkanBuffer& indexBuffer,
              const std::vector<VulkanDescriptorData>& descriptors,
              const VulkanPipeline& graphicsPipeline,
              std::vector<MeshDrawInfo> meshPool,
              std::vector<uint32_t> meshDrawIndices, int commandBufferIndex,
              bool isFullscreenShader) const;

  const std::vector<VkCommandBuffer>& getCommandBuffers() const;
};