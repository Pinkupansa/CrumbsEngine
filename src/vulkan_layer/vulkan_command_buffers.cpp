#include "vulkan_layer/vulkan_command_buffers.hpp"
#include "vulkan_layer/vulkan_object_creation_utils.hpp"
#include "engine_layer/debug.hpp"
#include "vulkan_layer/vulkan_buffer.hpp"
#include "vulkan_layer/vulkan_descriptor.hpp"
#include "vulkan_layer/vulkan_descriptor_data.hpp"
#include "vulkan_layer/vulkan_device.hpp"
#include "vulkan_layer/vulkan_framebuffers.hpp"
#include "vulkan_layer/vulkan_mesh_draw_info.hpp"
#include "vulkan_layer/vulkan_pipeline.hpp"
#include "vulkan_layer/vulkan_render_pass.hpp"
#include <stdexcept>
#include <vector>
#include "backends/imgui_impl_vulkan.h"
using Clock = std::chrono::high_resolution_clock;


VulkanCommandBuffers::VulkanCommandBuffers(
    const VulkanDevice& device, const VulkanFramebuffers& framebuffers)
    : pDevice(device) {
  commandBuffers.resize(framebuffers.getFramebuffers().size());

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = device.getCommandPool();
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount =
      static_cast<uint32_t>(commandBuffers.size());

  if (vkAllocateCommandBuffers(device.getDevice(), &allocInfo,
                               commandBuffers.data()) != VK_SUCCESS)
    throw std::runtime_error("Failed to allocate command buffers!");
}

VulkanCommandBuffers::~VulkanCommandBuffers() { destroy(); }

void VulkanCommandBuffers::destroy() {
  if (!commandBuffers.empty()) {
    vkFreeCommandBuffers(pDevice.getDevice(), pDevice.getCommandPool(),
                         static_cast<uint32_t>(commandBuffers.size()),
                         commandBuffers.data());
    commandBuffers.clear();
  }
}

void VulkanCommandBuffers::record(
    VkExtent2D renderAreaExtent, const VulkanRenderPass& renderPass,
    const VulkanFramebuffers& framebuffers, const VulkanBuffer& vertexBuffer,
    const VulkanBuffer& indexBuffer,
    const std::vector<VulkanDescriptorData>& descriptors,
    const VulkanPipeline& graphicsPipeline,
    const std::vector<MeshDrawInfo>& meshPool,
    const std::vector<uint32_t>& meshDrawIndices, int commandBufferIndex,
    bool isFullscreenShader, ImDrawData* drawData) const {
  // split descriptors in dynamic and non-dynamic

  std::vector<VulkanDescriptorData> dynamicDescriptors;
  std::vector<VulkanDescriptorData> staticDescriptors;
  for (VulkanDescriptorData desc : descriptors) {
    if (desc.isDynamicBuffer) {
      dynamicDescriptors.push_back(desc);
    } else {
      staticDescriptors.push_back(desc);
    }
  }

  vkResetCommandBuffer(commandBuffers[commandBufferIndex], 0);
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  if (vkBeginCommandBuffer(commandBuffers[commandBufferIndex], &beginInfo) !=
      VK_SUCCESS)
    throw std::runtime_error("Failed to begin recording command buffer!");

  std::vector<VkClearValue> clearValues = renderPass.getClearValues();

  VkRenderPassBeginInfo renderPassInfo = createRenderPassBeginInfo(
      renderPass.getRenderPass(), framebuffers.getFramebuffers()[commandBufferIndex],
      clearValues, renderAreaExtent);

  vkCmdBeginRenderPass(commandBuffers[commandBufferIndex], &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);
  vkCmdBindPipeline(commandBuffers[commandBufferIndex],
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    graphicsPipeline.getPipeline());
  for (VulkanDescriptorData desc : staticDescriptors) {
    vkCmdBindDescriptorSets(commandBuffers[commandBufferIndex],
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            graphicsPipeline.getLayout(), desc.set,
                            1,           // number of descriptor sets
                            &desc.descriptorSet,
                            0,  // dynamic offset count (0 for static UBO)
                            nullptr  // dynamic offsets
    );
  }
  if (isFullscreenShader) {
    vkCmdDraw(commandBuffers[commandBufferIndex], 3, 1, 0, 0);
  } else {
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(commandBuffers[commandBufferIndex], 0, 1,
                           &vertexBuffer.getBuffer(), &offset);

    vkCmdBindIndexBuffer(commandBuffers[commandBufferIndex],
                         indexBuffer.getBuffer(), 0, VK_INDEX_TYPE_UINT32);

    // TODO : split in dynamic and non dynamic descriptors and bind dynamic in
    // loop
    
    for (size_t j = 0; j < meshDrawIndices.size(); ++j) {
      auto start = Clock::now();
     
      for (VulkanDescriptorData desc : dynamicDescriptors) {
        uint32_t dynamicOffset =
            static_cast<uint32_t>(desc.alignedObjectSize * j);

        // Bind the descriptor set with the dynamic offset
        vkCmdBindDescriptorSets(commandBuffers[commandBufferIndex],
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                graphicsPipeline.getLayout(), desc.set, 1,
                                &desc.descriptorSet, 1, &dynamicOffset);
      }

      // Draw using the information in MeshDrawInfo
      const MeshDrawInfo& drawInfo = meshPool[meshDrawIndices[j]];

      vkCmdDrawIndexed(commandBuffers[commandBufferIndex],
                       drawInfo.indexCount,  // number of indices to draw
                       1,                    // instance count
                       drawInfo.indexOffset,   // first index
                       drawInfo.vertexOffset,  // vertex offset
                       0                     // first instance
      );
    }

  }

  if (drawData) {
      ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffers[commandBufferIndex]);
  }

  vkCmdEndRenderPass(commandBuffers[commandBufferIndex]);

  if (vkEndCommandBuffer(commandBuffers[commandBufferIndex]) != VK_SUCCESS)
    throw std::runtime_error("Failed to record command buffer!");
}

const std::vector<VkCommandBuffer>& VulkanCommandBuffers::getCommandBuffers()
    const {
  return commandBuffers;
}
