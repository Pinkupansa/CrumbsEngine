#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <stdexcept>
#include "vulkan_device.hpp"
#include "vulkan_render_pass.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_framebuffers.hpp"
#include "vulkan_ub_descriptor.hpp"
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
                VkExtent2D extent,
                VulkanRenderPass& renderPass,
                VulkanFramebuffers& framebuffers,
                VulkanBuffer& vertexBuffer,
                VulkanBuffer& indexBuffer,
                VulkanUBDescriptor& sceneUBDescriptor,
                VulkanUBDescriptor& objectsUBDescriptor,
                VulkanShadowView& shadowView,
                VulkanPipeline& graphicsPipeline,
                std::vector<MeshDrawInfo> meshPool,
                std::vector<uint32_t> meshDrawIndices, 
                int commandBufferIndex
                )
    
    {
        vkResetCommandBuffer(commandBuffers[commandBufferIndex], 0);
        const auto& fbos = framebuffers.getFramebuffers();
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffers[commandBufferIndex], &beginInfo) != VK_SUCCESS)
            throw std::runtime_error("Failed to begin recording command buffer!");
        
        std::vector<VkClearValue> clearValues;
        if(renderPass.hasColorAttachment()){
            VkClearValue colorValue; 
            colorValue.color = {{0.1f, 0.1f, 0.1f, 1.0f}};
            clearValues.push_back(colorValue);
        }
        if(renderPass.hasDepthAttachment()){
            VkClearValue depthValue; 
            depthValue.depthStencil = {1.0f, 0};
            clearValues.push_back(depthValue);
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass.getRenderPass();
        renderPassInfo.framebuffer = framebuffers.getFramebuffers()[commandBufferIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = extent;
        renderPassInfo.clearValueCount = clearValues.size();
        renderPassInfo.pClearValues = clearValues.data();

        vkCmdBeginRenderPass(commandBuffers[commandBufferIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffers[commandBufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline.getPipeline());
        VkBuffer vertexBuffers[] = { vertexBuffer.getBuffer() };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffers[commandBufferIndex], 0, 1, vertexBuffers, offsets);
        
        vkCmdBindIndexBuffer(commandBuffers[commandBufferIndex], indexBuffer.getBuffer(), 0, VK_INDEX_TYPE_UINT32);

        VkDescriptorSet sets[] = {
            sceneUBDescriptor.getDescriptorSet(),  // set 0
            objectsUBDescriptor.getDescriptorSet(),// set 1
            shadowView.getDescSet()                // set 2
        };
        for (size_t j = 0; j < meshDrawIndices.size(); ++j) {
            uint32_t dynamicOffset = static_cast<uint32_t>(objectsUBDescriptor.getAlignedObjectSize() * j);

            
            vkCmdBindDescriptorSets(
                commandBuffers[commandBufferIndex],
                VK_PIPELINE_BIND_POINT_GRAPHICS,
                graphicsPipeline.getLayout(),
                0,          // first set
                3,          // number of sets
                sets,
                1,          // dynamic offsets count
                &dynamicOffset
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
