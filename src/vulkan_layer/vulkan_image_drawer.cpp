#include "vulkan_layer/vulkan_image_drawer.hpp"

#include "engine_layer/debug.hpp"
#include "vulkan_layer/vulkan_command_buffers.hpp"
#include "vulkan_layer/vulkan_descriptor_data.hpp"
#include "vulkan_layer/vulkan_device.hpp"
#include "vulkan_layer/vulkan_framebuffers.hpp"
#include "vulkan_layer/vulkan_mesh_draw_info.hpp"
#include "vulkan_layer/vulkan_pipeline.hpp"
#include "vulkan_layer/vulkan_render_pass.hpp"
#include "vulkan_layer/vulkan_shader_data.hpp"
#include "vulkan_layer/vulkan_sync_objects.hpp"
#include <functional>

using Clock = std::chrono::high_resolution_clock;

VulkanImageDrawer::VulkanImageDrawer (
const VulkanDevice& device,
VkExtent2D extent,
const std::vector<std::vector<VulkanAttachment*>>& attachmentsPerFramebuffer,
bool isFirstPass,
const std::vector<VulkanDescriptorData>& descriptors,
const VulkanSyncObjects& renderTargetSyncObjects,
std::function<void ()> renderTargetFenceResetCallback,
std::vector<std::string> vertShaderPaths,
std::vector<std::string> fragShaderPaths,
VkCullModeFlagBits cullMode,

bool enableDepthTest,
bool enableDepthWrite,
bool isFullScreenShader,
VulkanAlphaBlendMode alphaBlendMode,
std::string name)
: pDevice (device), renderTargetSyncObjects (renderTargetSyncObjects),
  renderTargetFenceResetCallback (renderTargetFenceResetCallback),
  descriptors (descriptors), isFirstPass (isFirstPass),
  renderPass (device, attachmentsPerFramebuffer[0]), imageExtent (extent),
  graphicsPipeline (device,
                    renderPass,
                    extent,
                    descriptors,
                    vertShaderPaths,
                    fragShaderPaths,
                    cullMode,
                    enableDepthTest,
                    enableDepthWrite,
                    alphaBlendMode,
                    isFullScreenShader,
                    name + " Pipeline"),
  framebuffers (device, renderPass, attachmentsPerFramebuffer, name + " Framebuffers"),
  commandBuffers (device, framebuffers), name (name),
  isFullScreenShader (isFullScreenShader),
  waitStages ({ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }) {
    Debug::Log (name);
}

void VulkanImageDrawer::draw (const VulkanBuffer& vertexBuffer,
                              const VulkanBuffer& indexBuffer,
                              const std::vector<MeshDrawInfo>& meshPool,
                              const std::vector<uint32_t>& drawCallMeshIndices) {
    int syncIndex       = renderTargetSyncObjects.getSyncIndex ();
    uint32_t imageIndex = renderTargetSyncObjects.getCurrentFrame ();
    renderTargetFenceResetCallback ();
    // record command buffer for this image
    //forced to re-record every frame in molten-vk
    commandBuffers.record (imageExtent, renderPass, framebuffers, vertexBuffer,
                               indexBuffer, descriptors, graphicsPipeline, meshPool,
                               drawCallMeshIndices, imageIndex, isFullScreenShader);
    // submit
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount =
    renderTargetSyncObjects.hasWaitSemaphore (isFirstPass) ? 1 : 0;
    submitInfo.pWaitSemaphores =
    renderTargetSyncObjects.getWaitSemaphore (syncIndex, isFirstPass);
    submitInfo.pWaitDstStageMask = waitStages.data ();
    submitInfo.signalSemaphoreCount =
    renderTargetSyncObjects.hasSignalSemaphore (isFirstPass) ? 1 : 0;
    submitInfo.pSignalSemaphores = renderTargetSyncObjects.getSignalSemaphore (syncIndex);
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers.getCommandBuffers ()[imageIndex];


    vkQueueSubmit (pDevice.getGraphicsQueue (), 1, &submitInfo,
                   *renderTargetSyncObjects.getFence (syncIndex));
}

void VulkanImageDrawer::destroy () {
    commandBuffers.destroy ();
    framebuffers.destroy ();
    graphicsPipeline.destroy ();
    renderPass.destroy ();
}
