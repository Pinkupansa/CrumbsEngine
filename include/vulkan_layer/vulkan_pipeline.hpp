#pragma once
#include "vulkan_vertex.hpp"
#include "vulkan_device.hpp"
#include "vulkan_object_creation_utils.hpp"
#include "vulkan_render_pass.hpp"
#include <fstream>
#include <string>
#include <vulkan/vulkan.h>
#include "vulkan_descriptor_data.hpp"
std::vector<char> readFile (const std::string& filename) {
    std::ifstream file (filename, std::ios::ate | std::ios::binary);

    if (!file.is_open ()) {
        throw std::runtime_error ("Failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg ();
    std::vector<char> buffer (fileSize);

    file.seekg (0);
    file.read (buffer.data (), fileSize);

    file.close ();
    return buffer;
}

class VulkanPipeline {
    private:
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    std::vector<VkShaderModule> vertShaderModules;
    std::vector<VkShaderModule> fragShaderModules;
    const VulkanDevice& pDevice;

    public:
    VkPipeline getPipeline () const{
        return pipeline;
    }

    VkPipelineLayout getLayout () const {
        return pipelineLayout;
    }

    VulkanPipeline (const VulkanDevice& device,
                    const VulkanRenderPass& renderPass,
                    VkExtent2D viewportExtent,
                    const std::vector<VulkanDescriptorData>& descriptors,
                    const std::vector<std::string>& vertPaths,
                    const std::vector<std::string>& fragPaths,
                    VkCullModeFlagBits cullMode,
                    bool enableDepthTest,
                    bool enableDepthWrite,
                    std::string name)
    : pDevice (device) {

        
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
        for(std::string path : vertPaths){
            auto vertShaderCode = readFile(path);
            VkShaderModule vertShaderModule = createShaderModule(vertShaderCode, device.getDevice());
            vertShaderModules.push_back(vertShaderModule);
            shaderStages.push_back(createVertexShaderStageCreateInfo(vertShaderModule));
        }
        for(std::string path : fragPaths){
            auto vertShaderCode = readFile(path);
            VkShaderModule fragShaderModule = createShaderModule(vertShaderCode, device.getDevice());
            fragShaderModules.push_back(fragShaderModule);
            shaderStages.push_back(createFragmentShaderStageCreateInfo(fragShaderModule));
        }

        auto bindingDescription    = Vertex::getBindingDescription ();
        auto attributeDescriptions = Vertex::getAttributeDescriptions ();

        VkPipelineVertexInputStateCreateInfo vertexInputInfo =
        createVertexInputInfo (bindingDescription, attributeDescriptions);

        VkPipelineInputAssemblyStateCreateInfo inputAssembly =
        createDefaultInputAssemblyInfo ();

        VkViewport viewport = createViewport (viewportExtent);

        VkRect2D scissor = createScissor (viewportExtent);

        VkPipelineViewportStateCreateInfo viewportStateInfo =
        createViewportStateInfo (viewport, scissor);

        VkPipelineRasterizationStateCreateInfo rasterizerInfo =
        createDefaultRasterizerInfo (cullMode);

        VkPipelineMultisampleStateCreateInfo multisamplingInfo = createMSAAInfo (renderPass.hasResolveAttachment());

        VkPipelineColorBlendAttachmentState colorBlendAttachment =
        createFullColorBlendAttachment ();

        VkPipelineColorBlendStateCreateInfo colorBlendingInfo =
        createColorBlendStateInfo (colorBlendAttachment);

        std::vector<VkDescriptorSetLayout> descLayouts(descriptors.size());
        
        for(auto desc: descriptors){
            if(descLayouts[desc.set] != nullptr){
                
                throw std::runtime_error("ERROR : Two descriptor sets have the same set number !");
            }
            descLayouts[desc.set] = desc.layout;
        }

        pipelineLayout = createPipelineLayout (device, descLayouts, {}, name + " Layout");

        VkPipelineDepthStencilStateCreateInfo depthStencil =
        createDepthStencilInfo (enableDepthTest, enableDepthWrite);

        pipeline = createPipeline (device, shaderStages, vertexInputInfo,
                                   inputAssembly, viewportStateInfo, rasterizerInfo,
                                   multisamplingInfo, colorBlendingInfo, pipelineLayout,
                                   renderPass.getRenderPass (), depthStencil, name);
    }

    ~VulkanPipeline () {
        destroy ();
    }

    void destroy () {
        for(int i = 0; i < vertShaderModules.size(); i++){
            VkShaderModule vertShaderModule = vertShaderModules[i];
            if (vertShaderModule != VK_NULL_HANDLE) {
                vkDestroyShaderModule (pDevice.getDevice (), vertShaderModule, nullptr);
                vertShaderModule = VK_NULL_HANDLE;
            }
        }
        for(int i = 0; i < fragShaderModules.size(); i++){
            VkShaderModule fragShaderModule = fragShaderModules[i];
            if (fragShaderModule != VK_NULL_HANDLE) {
                vkDestroyShaderModule (pDevice.getDevice (), fragShaderModule, nullptr);
                fragShaderModule = VK_NULL_HANDLE;
            }
        }
        if (pipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout (pDevice.getDevice (), pipelineLayout, nullptr);
            pipelineLayout = VK_NULL_HANDLE;
        }
        if (pipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline (pDevice.getDevice (), pipeline, nullptr);
            pipeline = VK_NULL_HANDLE;
        }
        vertShaderModules.clear();
        fragShaderModules.clear();
    }
};
