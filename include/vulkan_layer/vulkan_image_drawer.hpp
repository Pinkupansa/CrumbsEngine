#pragma once

#include "vulkan_command_buffers.hpp"
#include "vulkan_descriptor_data.hpp"
#include "vulkan_framebuffers.hpp"
#include "vulkan_pipeline.hpp"
#include "vulkan_render_pass.hpp"
#include "vulkan_sync_objects.hpp"
/*class creating a render pass, a pipeline and dedicated command buffers and
 to be able to render on any compatible image on command.

takes in the shaders and a vector of vectors of imageviews (attachments)
*/

class VulkanImageDrawer {
    private:
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
   
    public:
    VulkanImageDrawer (const VulkanDevice& device,
                       VkExtent2D extent,
                       const std::vector<std::vector<VulkanAttachment*>>& attachmentsPerFramebuffer,
                       bool isFirstPass,
                       const std::vector<VulkanDescriptorData>& descriptors,
                       std::vector<std::string> vertShaderPaths,
                       std::vector<std::string> fragShaderPaths,
                       VkCullModeFlagBits cullMode,
                       
                       bool enableDepthTest,
                       bool enableDepthWrite,
                       std::string name)
    : pDevice (device), descriptors (descriptors), isFirstPass(isFirstPass),
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
                                              name + " Pipeline"),
      framebuffers (device, renderPass, attachmentsPerFramebuffer, name + " Framebuffers"),
      commandBuffers (device, framebuffers) {
    }

    void draw (const VulkanBuffer& vertexBuffer,
               const VulkanBuffer& indexBuffer,
               const std::vector<MeshDrawInfo>& meshPool,

               const std::vector<uint32_t>& drawCallMeshIndices,
               const VulkanSyncObjects& syncObjects,
               bool useWaitSem,
               bool useSigSem,
               int syncIndex,
               uint32_t imageIndex) {
        // record command buffer for this image
        commandBuffers.record (imageExtent, renderPass, framebuffers, vertexBuffer,
                               indexBuffer, descriptors, graphicsPipeline,
                               meshPool, drawCallMeshIndices, imageIndex);

        // submit
        VkSubmitInfo submitInfo{};
        submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = useWaitSem ? 1 : 0;
        submitInfo.pWaitSemaphores = isFirstPass? &syncObjects.imageAvailableSemaphore[syncIndex] : &syncObjects.renderFinishedSemaphore[syncIndex];
        submitInfo.pWaitDstStageMask    = waitStages.data ();
        submitInfo.signalSemaphoreCount = useSigSem ? 1 : 0;
        submitInfo.pSignalSemaphores = &syncObjects.renderFinishedSemaphore[syncIndex];
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers.getCommandBuffers ()[imageIndex];

        vkQueueSubmit (pDevice.getGraphicsQueue (), 1, &submitInfo,
                       syncObjects.inFlightFence[syncIndex]);
    }

    void destroy () {
        commandBuffers.destroy ();
        framebuffers.destroy ();
        graphicsPipeline.destroy ();
        renderPass.destroy ();
    }
};