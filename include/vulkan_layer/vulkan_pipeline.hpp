#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <vector>

#include "vulkan_descriptor_data.hpp"
#include "vulkan_device.hpp"
#include "vulkan_render_pass.hpp"
#include "vulkan_shader_data.hpp"
#include "vulkan_vertex.hpp"

std::vector<char> readFile(const std::string& filename);

class VulkanPipeline {
 private:
  VkPipeline pipeline;
  VkPipelineLayout pipelineLayout;
  std::vector<VkShaderModule> vertShaderModules;
  std::vector<VkShaderModule> fragShaderModules;
  const VulkanDevice& pDevice;

 public:
  VkPipeline getPipeline() const;

  VkPipelineLayout getLayout() const;

  VulkanPipeline(const VulkanDevice& device, const VulkanRenderPass& renderPass,
                 VkExtent2D viewportExtent,
                 const std::vector<VulkanDescriptorData>& descriptors,
                 const std::vector<std::string>& vertPaths,
                 const std::vector<std::string>& fragPaths,
                 VkCullModeFlagBits cullMode, bool enableDepthTest,
                 bool enableDepthWrite, VulkanAlphaBlendMode alphaBlendMode,
                 bool isFullscreen, std::string name);

  ~VulkanPipeline();

  void destroy();
};