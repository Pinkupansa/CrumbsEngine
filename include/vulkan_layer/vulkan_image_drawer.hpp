#pragma once

#include "vulkan_command_buffers.hpp"
#include "vulkan_framebuffers.hpp"
#include "vulkan_pipeline.hpp"
#include "vulkan_render_pass.hpp"
#include "vulkan_sync_objects.hpp"
#include "vulkan_descriptor_data.hpp"
/*class creating a render pass, a pipeline and dedicated command buffers and
 to be able to render on any compatible image on command.

takes in the shaders and a vector of vectors of imageviews (attachments)
*/

class VulkanImageDrawer {
    private:
    const VulkanDevice& pDevice;
    const std::vector<VulkanDescriptorData> pDescriptors;
    VkExtent2D imageExtent;

    VulkanRenderPass renderPass;
    // Pipeline and framebuffers
    VulkanPipeline graphicsPipeline;
    VulkanFramebuffers framebuffers;

    VulkanCommandBuffers commandBuffers;
    VulkanSyncObjects syncObjects;
    uint32_t currentIndex;
    std::vector<VkPipelineStageFlags> waitStages{ VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    uint32_t nFrames;

    public:
    VulkanImageDrawer (const VulkanDevice& device,
                       VkExtent2D extent,
                       const std::vector<std::vector<VkImageView>>& attachments,
                       const std::vector<VulkanDescriptorData> descriptors,
                       std::vector<VkAttachmentDescription> colorAttachmentDescs,
                       std::vector<VkClearValue> colorClearValues,
                       std::vector<VkAttachmentDescription> depthAttachmentDescs,
                       std::vector<VkClearValue> depthClearValues,
                       std::vector<std::string> vertShaderPaths,
                       std::vector<std::string> fragShaderPaths,
                       VkCullModeFlagBits cullMode,
                       VkCompareOp depthCompareOp,
                       std::string name)
    : pDevice (device), pDescriptors (descriptors),
      renderPass (device, colorAttachmentDescs, colorClearValues, depthAttachmentDescs, depthClearValues),
      imageExtent (extent),
      graphicsPipeline (device, renderPass, extent, descriptors, vertShaderPaths, fragShaderPaths, cullMode, depthCompareOp, name + " Pipeline"),
      framebuffers (device, renderPass, attachments, extent, name + " Framebuffers"),
      commandBuffers (device, framebuffers),
      syncObjects (device, attachments.size ()), currentIndex (0),
      nFrames (attachments.size ()) {
    }
    uint32_t getCurrentFrame () const {
        return currentIndex;
    }

    const VulkanSyncObjects& getSyncObjects () const {
        return syncObjects;
    }

    void waitAndReset () const {
        vkWaitForFences (pDevice.getDevice (), 1,
                         &syncObjects.inFlightFence[currentIndex], VK_TRUE, UINT64_MAX);
        vkResetFences (pDevice.getDevice (), 1, &syncObjects.inFlightFence[currentIndex]);
    }
    void draw (const VulkanBuffer& vertexBuffer,
               const VulkanBuffer& indexBuffer,
               const std::vector<MeshDrawInfo>& meshPool,

               const std::vector<uint32_t>& drawCallMeshIndices,
               int waitSemCount,
               int sigSemCount,
               uint32_t imageIndex) {
        // record command buffer for this image
        commandBuffers.record (imageExtent, renderPass, framebuffers, vertexBuffer,
                               indexBuffer, pDescriptors, graphicsPipeline,
                               meshPool, drawCallMeshIndices, imageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = waitSemCount;
        submitInfo.pWaitSemaphores = &syncObjects.imageAvailableSemaphore[currentIndex];
        submitInfo.pWaitDstStageMask    = waitStages.data ();
        submitInfo.signalSemaphoreCount = sigSemCount;
        submitInfo.pSignalSemaphores = &syncObjects.renderFinishedSemaphore[currentIndex];
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers.getCommandBuffers ()[imageIndex];

        vkQueueSubmit (pDevice.getGraphicsQueue (), 1, &submitInfo,
                       syncObjects.inFlightFence[currentIndex]);
        currentIndex = (currentIndex + 1) % nFrames;
    }

    void destroy () {
        syncObjects.destroy ();
        commandBuffers.destroy ();
        framebuffers.destroy ();
        graphicsPipeline.destroy ();
        renderPass.destroy ();
    }
};