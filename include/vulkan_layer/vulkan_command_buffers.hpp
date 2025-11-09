#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <stdexcept>
#include "vulkan_device.hpp"
#include "vulkan_swapchain.hpp"
#include "vulkan_render_pass.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_framebuffers.hpp"
#include "vulkan_descriptor.hpp"
#include "vulkan_pipeline.hpp"
#include "mesh_draw_info.hpp"

class VulkanCommandBuffers {
private: 
    VulkanDevice& pDevice;
public:
    std::vector<VkCommandBuffer> commandBuffers;

    VulkanCommandBuffers(VulkanDevice& device,
                         VulkanFramebuffers& framebuffers): pDevice(device)
    {
        commandBuffers.resize(framebuffers.getFramebuffers().size());

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = device.getCommandPool();
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

        if (vkAllocateCommandBuffers(device.getDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate command buffers!");
    }

    ~VulkanCommandBuffers(){
        destroy();
    }
    void destroy(){
       if (!commandBuffers.empty()) {
            
            vkFreeCommandBuffers(pDevice.getDevice(), pDevice.getCommandPool(), 
                                static_cast<uint32_t>(commandBuffers.size()), 
                                commandBuffers.data());
            commandBuffers.clear();
        }
    }

    void record(VulkanDevice& device,
                VulkanSwapchain& swapchain,
                VulkanRenderPass& renderPass,
                VulkanFramebuffers& framebuffers,
                VulkanBuffer& vertexBuffer,
                VulkanBuffer& indexBuffer,
                VulkanDescriptor& sceneUBDescriptor,
                VulkanDescriptor& staticObjectsUBDescriptor,
                std::vector<uint32_t> firstPosInIndexArrayStaticObjects,
                VulkanPipeline& graphicsPipeline
                
                )
    {
        const auto& fbos = framebuffers.getFramebuffers();
        auto extent = swapchain.getExtent();

        for (size_t i = 0; i < commandBuffers.size(); ++i) {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

            if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)
                throw std::runtime_error("Failed to begin recording command buffer!");
            
            VkClearValue clearValues[2];
            clearValues[0].color = {{0.1f, 0.1f, 0.1f, 1.0f}};  // color attachment
            clearValues[1].depthStencil = {1.0f, 0};            // depth attachment cleared to far plane

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = renderPass.getRenderPass();
            renderPassInfo.framebuffer = framebuffers.getFramebuffers()[i];       // framebuffer for this swapchain image
            renderPassInfo.renderArea.offset = {0, 0};
            renderPassInfo.renderArea.extent = swapchain.getExtent();
            renderPassInfo.clearValueCount = 2;
            renderPassInfo.pClearValues = clearValues;

            vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.getPipeline());
            VkBuffer vertexBuffers[] = { vertexBuffer.getBuffer() };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
            
            vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer.getBuffer(), 0, VK_INDEX_TYPE_UINT16);

            vkCmdBindDescriptorSets(
                commandBuffers[i],
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                graphicsPipeline.getLayout(),
                0,                       
                1,                        // number of descriptor sets
                &sceneUBDescriptor.getDescriptorSet(),
                0,                        // dynamic offset count (0 for static UBO)
                nullptr                   // dynamic offsets
            );

            for (size_t j = 0; j < firstPosInIndexArrayStaticObjects.size() - 1; ++j) {
                uint32_t dynamicOffset = static_cast<uint32_t>(staticObjectsUBDescriptor.getAlignedObjectSize() * j);

                vkCmdBindDescriptorSets(
                    commandBuffers[i],
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    graphicsPipeline.getLayout(),
                    1, 1, &staticObjectsUBDescriptor.getDescriptorSet(),
                    1, &dynamicOffset
                );

                vkCmdDrawIndexed(commandBuffers[i], firstPosInIndexArrayStaticObjects[j+1] - firstPosInIndexArrayStaticObjects[j], 1, firstPosInIndexArrayStaticObjects[j], 0, 0);
            }

            vkCmdEndRenderPass(commandBuffers[i]);

            if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
                throw std::runtime_error("Failed to record command buffer!");
        }
    }

    void record2(VulkanDevice& device,
                VulkanSwapchain& swapchain,
                VulkanRenderPass& renderPass,
                VulkanFramebuffers& framebuffers,
                VulkanBuffer& vertexBuffer,
                VulkanBuffer& indexBuffer,
                VulkanDescriptor& sceneUBDescriptor,
                VulkanDescriptor& objectsUBDescriptor,
                VulkanPipeline& graphicsPipeline,
                std::vector<MeshDrawInfo> meshPool,
                std::vector<uint32_t> meshDrawIndices, 
                int commandBufferIndex
                )
    
    {
        vkResetCommandBuffer(commandBuffers[commandBufferIndex], 0);
        const auto& fbos = framebuffers.getFramebuffers();
        auto extent = swapchain.getExtent();
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffers[commandBufferIndex], &beginInfo) != VK_SUCCESS)
            throw std::runtime_error("Failed to begin recording command buffer!");
        
        VkClearValue clearValues[2];
        clearValues[0].color = {{0.1f, 0.1f, 0.1f, 1.0f}};  // color attachment
        clearValues[1].depthStencil = {1.0f, 0};            // depth attachment cleared to far plane

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass.getRenderPass();
        renderPassInfo.framebuffer = framebuffers.getFramebuffers()[commandBufferIndex];       // framebuffer for this swapchain image
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapchain.getExtent();
        renderPassInfo.clearValueCount = 2;
        renderPassInfo.pClearValues = clearValues;

        vkCmdBeginRenderPass(commandBuffers[commandBufferIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffers[commandBufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.getPipeline());
        VkBuffer vertexBuffers[] = { vertexBuffer.getBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffers[commandBufferIndex], 0, 1, vertexBuffers, offsets);
        
        vkCmdBindIndexBuffer(commandBuffers[commandBufferIndex], indexBuffer.getBuffer(), 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(
            commandBuffers[commandBufferIndex],
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            graphicsPipeline.getLayout(),
            0,                       
            1,                        // number of descriptor sets
            &sceneUBDescriptor.getDescriptorSet(),
            0,                        // dynamic offset count (0 for static UBO)
            nullptr                   // dynamic offsets
        );

        for (size_t j = 0; j < meshDrawIndices.size(); ++j) {
            uint32_t dynamicOffset = static_cast<uint32_t>(objectsUBDescriptor.getAlignedObjectSize() * j);

            // Bind the descriptor set with the dynamic offset
            vkCmdBindDescriptorSets(
                commandBuffers[commandBufferIndex],
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                graphicsPipeline.getLayout(),
                1, 1, &objectsUBDescriptor.getDescriptorSet(),
                1, &dynamicOffset
            );

            // Draw using the information in MeshDrawInfo
            const MeshDrawInfo& drawInfo = meshPool[meshDrawIndices[j]];

            vkCmdDrawIndexed(
                commandBuffers[commandBufferIndex],
                drawInfo.indexCount,    // number of indices to draw
                1,                      // instance count
                drawInfo.indexOffset,    // first index
                drawInfo.vertexOffset,   // vertex offset
                0                       // first instance
            );
        }

        vkCmdEndRenderPass(commandBuffers[commandBufferIndex]);

        if (vkEndCommandBuffer(commandBuffers[commandBufferIndex]) != VK_SUCCESS)
            throw std::runtime_error("Failed to record command buffer!");
    
    }
    const std::vector<VkCommandBuffer>& getCommandBuffers() const {
        return commandBuffers;
    }
};
