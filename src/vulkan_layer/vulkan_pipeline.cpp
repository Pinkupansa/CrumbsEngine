#include "vulkan_layer/vulkan_pipeline.hpp"

#include "vulkan_layer/vulkan_descriptor_data.hpp"
#include "vulkan_layer/vulkan_device.hpp"
#include "vulkan_layer/vulkan_object_creation_utils.hpp"
#include "vulkan_layer/vulkan_render_pass.hpp"
#include "vulkan_layer/vulkan_shader_data.hpp"
#include "vulkan_layer/vulkan_vertex.hpp"
#include <fstream>
#include <string>
#include <vector>
#include "engine_layer/debug.hpp"

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

VkPipeline VulkanPipeline::getPipeline () const {
    return pipeline;
}

VkPipelineLayout VulkanPipeline::getLayout () const {
    return pipelineLayout;
}

VulkanPipeline::VulkanPipeline (const VulkanDevice& device,
                                const VulkanRenderPass& renderPass,
                                VkExtent2D viewportExtent,
                                const std::vector<VulkanDescriptorData>& descriptors,
                                const std::vector<std::string>& vertPaths,
                                const std::vector<std::string>& fragPaths,
                                VkCullModeFlagBits cullMode,
                                bool enableDepthTest,
                                bool enableDepthWrite,
                                VulkanAlphaBlendMode alphaBlendMode,
                                bool isFullscreen,
                                std::string name)
: pDevice (device) {
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    for (std::string path : vertPaths) {
        auto vertShaderCode = readFile (path);
        VkShaderModule vertShaderModule =
        createShaderModule (vertShaderCode, device.getDevice ());
        vertShaderModules.push_back (vertShaderModule);
        shaderStages.push_back (createVertexShaderStageCreateInfo (vertShaderModule));
    }
    for (std::string path : fragPaths) {
        auto fragShaderCode = readFile (path);
        VkShaderModule fragShaderModule =
        createShaderModule (fragShaderCode, device.getDevice ());
        fragShaderModules.push_back (fragShaderModule);
        shaderStages.push_back (createFragmentShaderStageCreateInfo (fragShaderModule));
    }

    auto bindingDescription    = Vertex::getBindingDescription ();
    auto attributeDescriptions = Vertex::getAttributeDescriptions ();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = isFullscreen ?
    createFullscreenVertexInputInfo () :
    createVertexInputInfo (bindingDescription, attributeDescriptions);

    VkPipelineInputAssemblyStateCreateInfo inputAssembly =
    createDefaultInputAssemblyInfo ();

    VkViewport viewport = createViewport (viewportExtent);

    VkRect2D scissor = createScissor (viewportExtent);

    VkPipelineViewportStateCreateInfo viewportStateInfo =
    createViewportStateInfo (viewport, scissor);

    VkPipelineRasterizationStateCreateInfo rasterizerInfo =
    createDefaultRasterizerInfo (cullMode);


    Debug::Log("pipeline has resolve " + std::to_string(renderPass.hasResolveAttachment()));
    VkPipelineMultisampleStateCreateInfo multisamplingInfo =
    createMSAAInfo (renderPass.hasResolveAttachment ());

    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments (
    renderPass.getNColorAttachments ());

    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    switch (alphaBlendMode) {
    case VulkanAlphaBlendMode::None:
        colorBlendAttachment = createFullColorBlendAttachment ();
        break;
    case VulkanAlphaBlendMode::Additive:
        colorBlendAttachment = createAdditiveAlphaBlendAttachment ();
        break;
    case VulkanAlphaBlendMode::Weighted:
        colorBlendAttachment = createWeightedAlphaBlendAttachment ();
        break;
    }

    for (int i = 0; i < colorBlendAttachments.size (); i++) {
        colorBlendAttachments[i] = colorBlendAttachment;
    }

    VkPipelineColorBlendStateCreateInfo colorBlendingInfo =
    createColorBlendStateInfo (colorBlendAttachments);

    std::vector<VkDescriptorSetLayout> descLayouts (descriptors.size ());

    for (auto desc : descriptors) {
        if (descLayouts[desc.set] != nullptr) {
            throw std::runtime_error (
            "ERROR : Two descriptor sets have the same set number !");
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

VulkanPipeline::~VulkanPipeline () {
    destroy ();
}

void VulkanPipeline::destroy () {
    for (int i = 0; i < vertShaderModules.size (); i++) {
        VkShaderModule vertShaderModule = vertShaderModules[i];
        if (vertShaderModule != VK_NULL_HANDLE) {
            vkDestroyShaderModule (pDevice.getDevice (), vertShaderModule, nullptr);
            vertShaderModule = VK_NULL_HANDLE;
        }
    }
    for (int i = 0; i < fragShaderModules.size (); i++) {
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
    vertShaderModules.clear ();
    fragShaderModules.clear ();
}
