#pragma once

#include "vulkan_command_buffers.hpp"
#include "vulkan_descriptor_data.hpp"
#include "vulkan_framebuffers.hpp"
#include "vulkan_pipeline.hpp"
#include "vulkan_render_pass.hpp"
#include "vulkan_sync_objects.hpp"
#include "vulkan_shader_data.hpp"
#include <functional>
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
    std::vector<VkPipelineStageFlags> waitStages{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    bool isFirstPass;
    bool isFullScreenShader;
    std::string name;
    public:
    VulkanImageDrawer (const VulkanDevice& device,
                       VkExtent2D extent,
                       const std::vector<std::vector<VulkanAttachment*>>& attachmentsPerFramebuffer,
                       bool isFirstPass,
                       const std::vector<VulkanDescriptorData>& descriptors,
                       const VulkanSyncObjects& renderTargetSyncObjects,
                       std::function<void()>  renderTargetFenceResetCallback,
                       std::vector<std::string> vertShaderPaths,
                       std::vector<std::string> fragShaderPaths,
                       VkCullModeFlagBits cullMode,
                       
                       bool enableDepthTest,
                       bool enableDepthWrite,
                       bool isFullScreenShader,
                       VulkanAlphaBlendMode alphaBlendMode,
                       std::string name)
    : pDevice (device), renderTargetSyncObjects(renderTargetSyncObjects), renderTargetFenceResetCallback(renderTargetFenceResetCallback), descriptors(descriptors), isFirstPass(isFirstPass),
      renderPass (device, attachmentsPerFramebuffer[0], isFirstPass),
      imageExtent (extent), graphicsPipeline (device,
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
      commandBuffers (device, framebuffers), name(name), isFullScreenShader(isFullScreenShader) {
        Debug::Log(name);
    }

    void draw (const VulkanBuffer& vertexBuffer,
               const VulkanBuffer& indexBuffer,
               const std::vector<MeshDrawInfo>& meshPool,
               const std::vector<uint32_t>& drawCallMeshIndices) {
        int syncIndex = renderTargetSyncObjects.getSyncIndex();
        uint32_t imageIndex = renderTargetSyncObjects.getCurrentFrame();
        renderTargetFenceResetCallback();
        // record command buffer for this image
        commandBuffers.record (imageExtent, renderPass, framebuffers, vertexBuffer,
                               indexBuffer, descriptors, graphicsPipeline,
                                meshPool, drawCallMeshIndices, imageIndex, isFullScreenShader);
        // submit
        VkSubmitInfo submitInfo{};
        submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = renderTargetSyncObjects.hasWaitSemaphore(isFirstPass)? 1 : 0;
        submitInfo.pWaitSemaphores = renderTargetSyncObjects.getWaitSemaphore(syncIndex, isFirstPass);
        submitInfo.pWaitDstStageMask    = waitStages.data ();
        submitInfo.signalSemaphoreCount = renderTargetSyncObjects.hasSignalSemaphore(isFirstPass)?1 :0;
        submitInfo.pSignalSemaphores = renderTargetSyncObjects.getSignalSemaphore(syncIndex);
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers.getCommandBuffers ()[imageIndex];

        vkQueueSubmit (pDevice.getGraphicsQueue (), 1, &submitInfo,
                       renderTargetSyncObjects.getFence(syncIndex));
    }

    void destroy () {
        commandBuffers.destroy ();
        framebuffers.destroy ();
        graphicsPipeline.destroy ();
        renderPass.destroy ();
    }
};