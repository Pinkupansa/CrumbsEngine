#pragma once
#include <vulkan/vulkan.h>
#include <string>
#include <fstream>
#include "vertex.hpp"
#include "vulkan_device.hpp"
#include "vulkan_render_pass.hpp"
#include "vulkan_shadow_view.hpp"
VkShaderModule createShaderModule(std::vector<char> code, const VkDevice &device) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module!");
    }

    return shaderModule;
}

std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file!");
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();
    return buffer;
}

class VulkanPipeline {
private:
    VkPipeline pipeline;
    VkPipelineLayout layout;
    VkShaderModule vertShaderModule; 
    VkShaderModule fragShaderModule; 
    VulkanDevice& pDevice;
public:
    
    VkPipeline getPipeline(){return pipeline;}
    VkPipelineLayout getLayout(){return layout;}

    VulkanPipeline(VulkanDevice& device, VulkanRenderPass& renderPass, VkExtent2D extent, VulkanUBDescriptor& sceneDataUBDescriptor, VulkanUBDescriptor& objectsUBDescriptor,
                   VulkanShadowView& shadowView, bool hasFrag, const std::string& vertPath, const std::string& fragPath): pDevice(device){
        
        auto vertShaderCode = readFile(vertPath);
        auto fragShaderCode = readFile(vertPath);
        if(hasFrag)
            fragShaderCode = readFile(fragPath);

        vertShaderModule = createShaderModule(vertShaderCode, device.getDevice());
        if(hasFrag)
            fragShaderModule = createShaderModule(fragShaderCode, device.getDevice());

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName  = "main";
                
        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        
        if(hasFrag){
            fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
            fragShaderStageInfo.module = fragShaderModule;
            fragShaderStageInfo.pName  = "main";
        }

        std::vector<VkPipelineShaderStageCreateInfo> shaderStages = {vertShaderStageInfo};
        if(hasFrag)
            shaderStages.push_back(fragShaderStageInfo);

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();
        
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f; viewport.y = 0.0f;
        viewport.width  = (float) extent.width;
        viewport.height = (float) extent.height;
        viewport.minDepth = 0.0f; viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = extent;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        if(hasFrag){
            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable = VK_FALSE;
        }
        VkPipelineColorBlendStateCreateInfo colorBlending{};
        if(hasFrag){
            colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlending.logicOpEnable = VK_FALSE;
            colorBlending.attachmentCount = 1;
            colorBlending.pAttachments = &colorBlendAttachment;
        }

        std::vector<VkDescriptorSetLayout> descLayouts{sceneDataUBDescriptor.getLayout(), objectsUBDescriptor.getLayout(), shadowView.getLayout()};
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = descLayouts.size();                          // number of descriptor set layouts
        pipelineLayoutInfo.pSetLayouts = descLayouts.data();      // pointer to your descriptor set layout
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;
        vkCreatePipelineLayout(device.getDevice(), &pipelineLayoutInfo, nullptr, &layout);
        
        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;               // enable depth test
        depthStencil.depthWriteEnable = VK_TRUE;              // enable writing to depth buffer
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;     // standard depth test
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;
       


        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = shaderStages.size();
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        if(hasFrag)
            pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = layout;
        pipelineInfo.renderPass = renderPass.getRenderPass(); // your render pass
        pipelineInfo.subpass = 0;
        pipelineInfo.pDepthStencilState = &depthStencil;       
             
        if(vkCreateGraphicsPipelines(device.getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &pipeline) != VK_SUCCESS){
            throw std::runtime_error("Couldn't create pipeline !");
        }
        device.nameObject((uint64_t)pipeline, VK_OBJECT_TYPE_PIPELINE, "Graphics Pipeline");
    }
    ~VulkanPipeline(){
        destroy();
    }
    void destroy() {
        if (vertShaderModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(pDevice.getDevice(), vertShaderModule, nullptr);
            vertShaderModule = VK_NULL_HANDLE;
        }
        if (fragShaderModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule(pDevice.getDevice(), fragShaderModule, nullptr);
            fragShaderModule = VK_NULL_HANDLE;
        }
        if (layout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(pDevice.getDevice(), layout, nullptr);
            layout = VK_NULL_HANDLE;
        }
        if (pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(pDevice.getDevice(), pipeline, nullptr);
            pipeline = VK_NULL_HANDLE;
        }
    }
};
