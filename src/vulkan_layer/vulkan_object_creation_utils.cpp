#include "vulkan_layer/vulkan_object_creation_utils.hpp"
#include "vulkan_layer/vulkan_constants.hpp"
#include "vulkan_layer/vulkan_surface.hpp"

#include <omp.h>
#define STB_IMAGE_IMPLEMENTATION
#include <chrono>
#include "vulkan_layer/stb/stb_image.h"

VkImage createImage(const VulkanDevice& device, VkExtent2D extent,
                    VkFormat format, VkImageUsageFlags imageUsage,
                    bool isSinglesampled, std::string name) {
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = extent.width;
  imageInfo.extent.height = extent.height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = format;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = imageUsage;
  imageInfo.samples = isSinglesampled ? VK_SAMPLE_COUNT_1_BIT : MSAA_LEVEL;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  VkImage image;
  if (vkCreateImage(device.getDevice(), &imageInfo, nullptr, &image) !=
      VK_SUCCESS) {  // depth image creation
    throw std::runtime_error("Failed to create depth image!");
  }

  device.nameObject((uint64_t)image, VK_OBJECT_TYPE_IMAGE, name);
  return image;
}

VkImage createColorImage(const VulkanDevice& device, VkExtent2D extent,
                         bool isSingleSampled, std::string name,
                         VkImageUsageFlags usage) {
  return createImage(device, extent, DEFAULT_COLOR_FORMAT, usage,
                     isSingleSampled, name);
}

VkImage createDepthImage(const VulkanDevice& device, VkExtent2D extent,
                         bool isSingleSampled, std::string name,
                         VkImageUsageFlags usage) {
  return createImage(device, extent, DEFAULT_DEPTH_FORMAT, usage,
                     isSingleSampled, name);
}

VkImageView createImageView(const VulkanDevice& device, const VkImage& image,
                            VkFormat format, VkImageAspectFlags aspectMask,
                            std::string name) {
  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;  // 2D texture
  viewInfo.format = format;                   // same as swapchain

  viewInfo.subresourceRange.aspectMask = aspectMask;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  VkImageView imageView;
  if (vkCreateImageView(device.getDevice(), &viewInfo, nullptr, &imageView) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create image view!");
  }

  device.nameObject((uint64_t)imageView, VK_OBJECT_TYPE_IMAGE_VIEW, name);

  return imageView;
}

VkImageView createColorImageView(const VulkanDevice& device,
                                 const VkImage& image, std::string name) {
  return createImageView(device, image, DEFAULT_COLOR_FORMAT,
                         VK_IMAGE_ASPECT_COLOR_BIT, name);
}

VkImageView createDepthImageView(const VulkanDevice& device,
                                 const VkImage& image, std::string name) {
  return createImageView(device, image, DEFAULT_DEPTH_FORMAT,
                         VK_IMAGE_ASPECT_DEPTH_BIT, name);
}

VkSwapchainKHR createTripleBufferingSwapchain(const VulkanDevice& device,
                                              const VulkanSurface& surface,
                                              std::string name) {
    VkSwapchainCreateInfoKHR swapchainInfo{};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.surface = surface.getSurface();
    swapchainInfo.minImageCount = 3;  // triple buffering
    swapchainInfo.imageFormat = DEFAULT_COLOR_FORMAT;  // pick first supported format
    swapchainInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    
    swapchainInfo.imageExtent = surface.getCapabilities().currentExtent;
    swapchainInfo.imageArrayLayers = 1;  // just means 2d image
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.preTransform = surface.getCapabilities().currentTransform;
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;  // no VSync
    swapchainInfo.clipped = VK_TRUE;
    swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

  VkSwapchainKHR swapchain;
  if (vkCreateSwapchainKHR(device.getDevice(), &swapchainInfo, nullptr,
                           &swapchain) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create swapchain!");
  }

  device.nameObject((uint64_t)swapchain, VK_OBJECT_TYPE_SWAPCHAIN_KHR, name);
  return swapchain;
}

VkMemoryAllocateInfo createMemoryAllocateInfo(
    const VulkanDevice& device, const VkMemoryRequirements& memReq,
    VkMemoryPropertyFlags properties) {
  // allocate memory for image on GPU
  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memReq.size;
  allocInfo.memoryTypeIndex =
      device.findMemoryType(memReq.memoryTypeBits, properties);
  return allocInfo;
}

VkDeviceMemory allocateMemory(const VulkanDevice& device,
                              VkMemoryRequirements memReq,
                              VkMemoryPropertyFlags properties) {
  VkMemoryAllocateInfo allocInfo =
      createMemoryAllocateInfo(device, memReq, properties);
  VkDeviceMemory memory;
  vkAllocateMemory(device.getDevice(), &allocInfo, nullptr, &memory);

  return memory;
}

VkDeviceMemory allocateAndBindImageMemory(const VulkanDevice& device,
                                          const VkImage& image) {
  VkMemoryRequirements memReq;
  vkGetImageMemoryRequirements(device.getDevice(), image, &memReq);

  VkDeviceMemory memory =
      allocateMemory(device, memReq, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  vkBindImageMemory(device.getDevice(), image, memory,
                    0);  // bind image to memory

  return memory;
}

VkDeviceMemory allocateAndBindBufferMemory(const VulkanDevice& device,
                                           const VkBuffer& buffer) {
  VkMemoryRequirements memReq;
  vkGetBufferMemoryRequirements(device.getDevice(), buffer, &memReq);

  VkDeviceMemory memory =
      allocateMemory(device, memReq, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

  vkBindBufferMemory(device.getDevice(), buffer, memory,
                     0);  // bind image to memory

  return memory;
}

VkAttachmentDescription createColorAttachment(
    bool isSingleSampled, VkAttachmentLoadOp loadOp, VkImageLayout initialLayout, VkImageLayout finalLayout) {
  VkAttachmentDescription colorAttachment;
  colorAttachment.format = DEFAULT_COLOR_FORMAT;  // same as swapchain
  colorAttachment.samples =
      isSingleSampled ? VK_SAMPLE_COUNT_1_BIT : MSAA_LEVEL;  // no MSAA for now
  colorAttachment.loadOp = loadOp;                           // clear at start
  colorAttachment.storeOp =
      VK_ATTACHMENT_STORE_OP_STORE;  // store result for presentation
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = initialLayout;
  colorAttachment.finalLayout = finalLayout;
  colorAttachment.flags = 0;
  return colorAttachment;
}

VkAttachmentReference createColorAttachmentRef(int attachmentNumber) {
  VkAttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment =
      attachmentNumber;  // index in the attachment array
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  return colorAttachmentRef;
}

VkAttachmentDescription createDepthAttachment(VkAttachmentLoadOp loadOp, VkImageLayout initialLayout, VkImageLayout finalLayout) {
  VkAttachmentDescription depthAttachment{};
  depthAttachment.format = DEFAULT_DEPTH_FORMAT;  // the same format as your depth image
  depthAttachment.samples = MSAA_LEVEL;
  depthAttachment.loadOp = loadOp;
  depthAttachment.storeOp =
      VK_ATTACHMENT_STORE_OP_STORE;  // depth must be preserved throughout passes
  depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout = initialLayout;
  depthAttachment.finalLayout = finalLayout;
  return depthAttachment;
}
VkAttachmentDescription createShadowDepthAttachment(VkAttachmentLoadOp loadOp) {
  VkAttachmentDescription depthAttachment{};
  depthAttachment.format = DEFAULT_DEPTH_FORMAT;  // the same format as your depth image
  depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAttachment.loadOp = loadOp;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout =
      loadOp == VK_ATTACHMENT_LOAD_OP_LOAD
          ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
          : VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  return depthAttachment;
}
VkAttachmentReference createDepthAttachmentRef(int attachmentNumber) {
  VkAttachmentReference depthAttachmentRef{};
  depthAttachmentRef.attachment =
      attachmentNumber;  // index of the depth attachment
  depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  return depthAttachmentRef;
}

VkBuffer createBuffer(const VulkanDevice& device, VulkanBufferType type,
                      VkDeviceSize bufferSize, std::string name) {
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = bufferSize;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  switch (type) {
    case VulkanBufferType::Vertex:
      bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
      break;
    case VulkanBufferType::Index:
      bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
      break;
    case VulkanBufferType::Uniform:
      bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
      break;
    case VulkanBufferType::Staging:
      bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
      break;
  }

  VkBuffer buffer;
  if (vkCreateBuffer(device.getDevice(), &bufferInfo, nullptr, &buffer) !=
      VK_SUCCESS)
    throw std::runtime_error("Failed to create buffer!");
  device.nameObject((uint64_t)buffer, VK_OBJECT_TYPE_BUFFER, name);
  return buffer;
}

VkDescriptorSetLayout createDescriptorLayout(
    const VulkanDevice& device, VkDescriptorType descriptorType,
    VkShaderStageFlags shaderStageFlags, uint32_t binding,
    uint32_t descriptorCount, VkDescriptorBindingFlags bindingFlags) {
  VkDescriptorSetLayoutBinding layoutBinding;
  layoutBinding.binding = binding;
  layoutBinding.descriptorType = descriptorType;
  layoutBinding.descriptorCount = descriptorCount;
  layoutBinding.stageFlags = shaderStageFlags;
  layoutBinding.pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{};
  if (bindingFlags != 0) {
    bindingFlagsInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    bindingFlagsInfo.bindingCount = 1;
    bindingFlagsInfo.pBindingFlags = &bindingFlags;
  }

  // layout from all the bindings created
  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = 1;
  layoutInfo.pBindings = &layoutBinding;
  if (bindingFlags != 0) {
    layoutInfo.pNext = &bindingFlagsInfo;
  }

  VkDescriptorSetLayout layout;
  if (vkCreateDescriptorSetLayout(device.getDevice(), &layoutInfo, nullptr,
                                  &layout) != VK_SUCCESS)
    throw std::runtime_error("Failed to create descriptor set layout!");
  return layout;
}

VkDescriptorPool createDescriptorPool(const VulkanDevice& device,
                                      VkDescriptorType descriptorType,
                                      uint32_t descriptorCount) {
  VkDescriptorPoolSize poolSize{};
  poolSize.type = descriptorType;
  poolSize.descriptorCount = descriptorCount;

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = 1;
  poolInfo.pPoolSizes = &poolSize;
  poolInfo.maxSets = 1;

  VkDescriptorPool pool;
  if (vkCreateDescriptorPool(device.getDevice(), &poolInfo, nullptr, &pool) !=
      VK_SUCCESS)
    throw std::runtime_error("Failed to create descriptor pool!");
  return pool;
}

VkDescriptorSet allocateDescriptorSet(const VulkanDevice& device,
                                      const VkDescriptorSetLayout& layout,
                                      const VkDescriptorPool& pool,
                                      std::string name, uint32_t actualCount,
                                      bool isVariableCount) {
  VkDescriptorSetVariableDescriptorCountAllocateInfo variableDescCountInfo{};
  if (isVariableCount) {
    variableDescCountInfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
    variableDescCountInfo.descriptorSetCount = 1;
    variableDescCountInfo.pDescriptorCounts = &actualCount;
  }
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = pool;
  allocInfo.descriptorSetCount = 1;
  allocInfo.pSetLayouts = &layout;
  if (isVariableCount) {
    allocInfo.pNext = &variableDescCountInfo;
  }
  VkDescriptorSet descriptorSet;
  if (vkAllocateDescriptorSets(device.getDevice(), &allocInfo, &descriptorSet) !=
      VK_SUCCESS)
    throw std::runtime_error("Failed to allocate descriptor set!");

  device.nameObject((uint64_t)descriptorSet, VK_OBJECT_TYPE_DESCRIPTOR_SET,
                    name);
  return descriptorSet;
}

void writeBufferInDescriptorSet(const VulkanDevice& device,
                                const VkBuffer& buffer,
                                const VkDescriptorSet& descriptorSet,
                                VkDescriptorType descriptorType,
                                VkDeviceSize unalignedObjectSize) {
  VkDescriptorBufferInfo bufferInfo{};
  bufferInfo.buffer = buffer;
  bufferInfo.offset = 0;
  bufferInfo.range = unalignedObjectSize;  // covers all UBOs in the buffer

  VkWriteDescriptorSet descriptorWrite{};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = descriptorSet;  // which descriptor set to update
  descriptorWrite.dstBinding = 0;          // binding number (matches shader)
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = descriptorType;  // ⚠ must match layout
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pBufferInfo = &bufferInfo;  // points to your buffer
  descriptorWrite.pImageInfo = nullptr;       // only used for samplers/images
  descriptorWrite.pTexelBufferView = nullptr;
  vkUpdateDescriptorSets(device.getDevice(), 1, &descriptorWrite, 0, nullptr);
}

void writeImageSamplerInDescriptorSet(const VulkanDevice& device,
                                      const VkImageView& imageView,
                                      const VkSampler& sampler,
                                      const VkDescriptorSet& descSet) {
  VkDescriptorImageInfo shadowInfo{};
  shadowInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  shadowInfo.imageView = imageView;  // your shadow depth view
  shadowInfo.sampler = sampler;      // sampler

  VkWriteDescriptorSet shadowWrite{};
  shadowWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  shadowWrite.dstSet = descSet;
  shadowWrite.dstBinding = 0;
  shadowWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  shadowWrite.descriptorCount = 1;
  shadowWrite.pImageInfo = &shadowInfo;

  vkUpdateDescriptorSets(device.getDevice(), 1, &shadowWrite, 0, nullptr);
}

void writeImageSamplerInDescriptorSetArray(const VulkanDevice& device,
                                           const VkImageView& imageView,
                                           const VkSampler& sampler,
                                           const VkDescriptorSet& descSet,
                                           uint32_t binding,
                                           uint32_t arrayElement) {
  VkDescriptorImageInfo textureInfo{};
  textureInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  textureInfo.imageView = imageView;  // your texture image view
  textureInfo.sampler = sampler;      // sampler

  VkWriteDescriptorSet textureWrite{};
  textureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  textureWrite.dstSet = descSet;
  textureWrite.dstBinding = binding;
  textureWrite.dstArrayElement = arrayElement;
  textureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  textureWrite.descriptorCount = 1;
  textureWrite.pImageInfo = &textureInfo;

  vkUpdateDescriptorSets(device.getDevice(), 1, &textureWrite, 0, nullptr);
}
VkShaderModule createShaderModule(std::vector<char> code,
                                  const VkDevice& device) {
  VkShaderModuleCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) !=
      VK_SUCCESS) {
    throw std::runtime_error("Failed to create shader module!");
  }

  return shaderModule;
}

VkPipelineShaderStageCreateInfo
createVertexShaderStageCreateInfo(const VkShaderModule& vertexShaderModule) {
  VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
  vertShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = vertexShaderModule;
  vertShaderStageInfo.pName = "main";
  return vertShaderStageInfo;
}
void writeImageInDescriptorSet(const VulkanDevice& device,
                               const VkImageView& imageView,
                               const VkDescriptorSet& descSet,
                               uint32_t binding) {

  VkDescriptorImageInfo imageInfo{};
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo.imageView = imageView;
  imageInfo.sampler = VK_NULL_HANDLE; // MUST be null for SAMPLED_IMAGE

  VkWriteDescriptorSet write{};
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.dstSet = descSet;
  write.dstBinding = binding;
  write.dstArrayElement = 0;
  write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
  write.descriptorCount = 1;
  write.pImageInfo = &imageInfo;

  vkUpdateDescriptorSets(device.getDevice(), 1, &write, 0, nullptr);
}

VkPipelineShaderStageCreateInfo
createFragmentShaderStageCreateInfo(const VkShaderModule& fragmentShaderModule) {
  VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
  fragShaderStageInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = fragmentShaderModule;
  fragShaderStageInfo.pName = "main";
  return fragShaderStageInfo;
}

VkPipelineVertexInputStateCreateInfo createVertexInputInfo(
    const VkVertexInputBindingDescription& bindingDescription,
    const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions) {
  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
  vertexInputInfo.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attributeDescriptions.size());
  return vertexInputInfo;
}

VkPipelineVertexInputStateCreateInfo createFullscreenVertexInputInfo() {
  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 0;
  vertexInputInfo.vertexAttributeDescriptionCount = 0;
  vertexInputInfo.pVertexBindingDescriptions = nullptr;
  vertexInputInfo.pVertexAttributeDescriptions = nullptr;
  return vertexInputInfo;
}
VkPipelineInputAssemblyStateCreateInfo createDefaultInputAssemblyInfo() {
  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;
  return inputAssembly;
}

VkViewport createViewport(VkExtent2D viewportExtent) {
  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float)viewportExtent.width;
  viewport.height = (float)viewportExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  return viewport;
}

VkRect2D createScissor(VkExtent2D viewportExtent) {
  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = viewportExtent;
  return scissor;
}

VkPipelineViewportStateCreateInfo
createViewportStateInfo(const VkViewport& viewport, const VkRect2D& scissor) {
  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;
  return viewportState;
}

VkPipelineRasterizationStateCreateInfo
createDefaultRasterizerInfo(VkCullModeFlagBits cullMode) {
  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = cullMode;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  return rasterizer;
}

VkPipelineMultisampleStateCreateInfo createMSAAInfo(bool hasResolve) {
  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples =
      hasResolve ? MSAA_LEVEL : VK_SAMPLE_COUNT_1_BIT;
  return multisampling;
}

VkPipelineColorBlendAttachmentState createFullColorBlendAttachment() {
  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;
  return colorBlendAttachment;
}
VkPipelineColorBlendAttachmentState createAdditiveAlphaBlendAttachment() {
  VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

  colorBlendAttachment.blendEnable = VK_TRUE;

  // Additive
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;

  // Usually same for alpha
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
  return colorBlendAttachment;
}
VkPipelineColorBlendAttachmentState createWeightedAlphaBlendAttachment() {
  VkPipelineColorBlendAttachmentState blend = {};
  blend.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  blend.blendEnable = VK_TRUE;
  blend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  blend.colorBlendOp = VK_BLEND_OP_ADD;

  blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  blend.alphaBlendOp = VK_BLEND_OP_ADD;
  return blend;
}
VkPipelineColorBlendStateCreateInfo createColorBlendStateInfo(const std::vector<VkPipelineColorBlendAttachmentState>& colorBlendAttachment) {
  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.attachmentCount = colorBlendAttachment.size();
  colorBlending.pAttachments = colorBlendAttachment.data();
  return colorBlending;
}

VkPipelineLayout createPipelineLayout(
    const VulkanDevice& device,
    const std::vector<VkDescriptorSetLayout>& descLayouts,
    const std::vector<VkPushConstantRange>& pushConstantRanges,
    std::string name) {
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount =
      descLayouts.size();  // number of descriptor set layouts
  pipelineLayoutInfo.pSetLayouts =
      descLayouts.data();  // pointer to your descriptor set layout
  pipelineLayoutInfo.pushConstantRangeCount = pushConstantRanges.size();
  pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

  VkPipelineLayout layout;
  if (vkCreatePipelineLayout(device.getDevice(), &pipelineLayoutInfo, nullptr,
                             &layout) != VK_SUCCESS) {
    throw std::runtime_error("Couldn't create pipeline layout " + name);
  }
  device.nameObject((uint64_t)layout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, name);
  return layout;
}

VkPipelineDepthStencilStateCreateInfo
createDepthStencilInfo(bool enableDepthTest, bool enableDepthWrite) {
  VkPipelineDepthStencilStateCreateInfo depthStencil{};
  depthStencil.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable = enableDepthTest;  // enable depth test
  depthStencil.depthWriteEnable =
      enableDepthWrite;  // enable writing to depth buffer
  depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;  // standard depth test
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.stencilTestEnable = VK_FALSE;
  return depthStencil;
}

VkPipeline createPipeline(
    const VulkanDevice& device,
    const std::vector<VkPipelineShaderStageCreateInfo>& shaderStagesInfos,
    const VkPipelineVertexInputStateCreateInfo& vertexInputInfo,
    const VkPipelineInputAssemblyStateCreateInfo& inputAssemblyInfo,
    const VkPipelineViewportStateCreateInfo& viewportStateInfo,
    const VkPipelineRasterizationStateCreateInfo& rasterizerInfo,
    const VkPipelineMultisampleStateCreateInfo& msaaInfo,
    const VkPipelineColorBlendStateCreateInfo& colorBlendingInfo,
    const VkPipelineLayout& pipelineLayout, const VkRenderPass& renderPass,
    const VkPipelineDepthStencilStateCreateInfo& depthStencil,
    std::string name) {
  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = shaderStagesInfos.size();
  pipelineInfo.pStages = shaderStagesInfos.data();
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
  pipelineInfo.pViewportState = &viewportStateInfo;
  pipelineInfo.pRasterizationState = &rasterizerInfo;
  pipelineInfo.pMultisampleState = &msaaInfo;
  pipelineInfo.pColorBlendState = &colorBlendingInfo;
  pipelineInfo.layout = pipelineLayout;
  pipelineInfo.renderPass = renderPass;  // your render pass
  pipelineInfo.subpass = 0;
  pipelineInfo.pDepthStencilState = &depthStencil;

  VkPipeline pipeline;
  if (vkCreateGraphicsPipelines(device.getDevice(), VK_NULL_HANDLE, 1,
                                &pipelineInfo, nullptr, &pipeline) !=
      VK_SUCCESS) {
    throw std::runtime_error("Couldn't create pipeline !");
  }
  device.nameObject((uint64_t)pipeline, VK_OBJECT_TYPE_PIPELINE, name);
  return pipeline;
}

VkFramebuffer createFramebuffer(const VulkanDevice& device,
                                 const VkRenderPass& rp,
                                 const std::vector<VkImageView> attachments,
                                 VkExtent2D extent, std::string name) {
  VkFramebufferCreateInfo fbInfo{};
  fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  fbInfo.renderPass = rp;
  fbInfo.attachmentCount = attachments.size();
  fbInfo.pAttachments = attachments.data();
  fbInfo.width = extent.width;
  fbInfo.height = extent.height;
  fbInfo.layers = 1;
  VkFramebuffer framebuffer;
  if (vkCreateFramebuffer(device.getDevice(), &fbInfo, nullptr, &framebuffer) !=
      VK_SUCCESS)
    throw std::runtime_error("Failed to create framebuffer!");
  device.nameObject((uint64_t)framebuffer, VK_OBJECT_TYPE_FRAMEBUFFER, name);
  return framebuffer;
}

VkRenderPassBeginInfo createRenderPassBeginInfo(
    const VkRenderPass& rp, const VkFramebuffer& framebuffer,
    const std::vector<VkClearValue>& clearValues, VkExtent2D renderAreaExtent) {
  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = rp;
  renderPassInfo.framebuffer =
      framebuffer;  // framebuffer for this swapchain image
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = renderAreaExtent;
  renderPassInfo.clearValueCount = clearValues.size();
  renderPassInfo.pClearValues = clearValues.data();
  return renderPassInfo;
}

VkClearValue createColorClearValue(VkClearColorValue color) {
  VkClearValue clearValue;
  clearValue.color = color;
  return clearValue;
}

VkClearValue createDepthClearValue(VkClearDepthStencilValue depthValue) {
  VkClearValue clearValue;
  clearValue.depthStencil = depthValue;
  return clearValue;
}

VkSampler createSampler(const VulkanDevice& device, std::string name) {
  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  /*samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
  samplerInfo.anisotropyEnable = VK_TRUE;

  //obtain physical device properties to set maxAnisotropy
  VkPhysicalDeviceProperties properties{};
  vkGetPhysicalDeviceProperties (device.getPhysicalDevice (), &properties);
  samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;*/

  VkSampler sampler;
  vkCreateSampler(device.getDevice(), &samplerInfo, nullptr, &sampler);

  device.nameObject((uint64_t)sampler, VK_OBJECT_TYPE_SAMPLER, name);
  return sampler;
}

void createBufferWithData(const VulkanDevice& device, VulkanBufferType type,
                          VkDeviceSize bufferSize, const void* data,
                          VkBuffer& buffer, VkDeviceMemory& bufferMemory,
                          std::string name) {
  buffer = createBuffer(device, type, bufferSize, name + " Buffer");
  bufferMemory = allocateAndBindBufferMemory(device, buffer);

  // map memory and copy data
  void* mappedData;

  vkMapMemory(device.getDevice(), bufferMemory, 0, bufferSize, 0, &mappedData);
  memcpy(mappedData, data, static_cast<size_t>(bufferSize));
  vkUnmapMemory(device.getDevice(), bufferMemory);

  device.nameObject((uint64_t)bufferMemory, VK_OBJECT_TYPE_DEVICE_MEMORY,
                    name + " Buffer Memory");
}
VkCommandBuffer beginSingleTimeCommands(VulkanDevice const& device,
                                        std::string name) {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = device.getCommandPool();
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(device.getDevice(), &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  device.nameObject((uint64_t)commandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER,
                    name);
  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}

void endSingleTimeCommands(const VulkanDevice& device,
                           VkCommandBuffer commandBuffer) {
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(device.getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(device.getGraphicsQueue());

  vkFreeCommandBuffers(device.getDevice(), device.getCommandPool(), 1,
                       &commandBuffer);
}

void transitionImageLayout(const VulkanDevice& device, VkImage image,
                           VkFormat format, VkImageLayout oldLayout,
                           VkImageLayout newLayout) {
  VkCommandBuffer commandBuffer =
      beginSingleTimeCommands(device, "Image Layout Transition");
  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
             newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
        format == VK_FORMAT_D24_UNORM_S8_UINT) {
      barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
             newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = 0;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  } else {
    throw std::invalid_argument("Unsupported layout transition!");
  }

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0,
                       nullptr, 0, nullptr, 1, &barrier);

  endSingleTimeCommands(device, commandBuffer);
}

void copyBufferToImage(const VulkanDevice& device, VkBuffer buffer,
                       VkImage image, uint32_t width, uint32_t height) {
  VkCommandBuffer commandBuffer =
      beginSingleTimeCommands(device, "Copy Buffer To Image");

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;

  region.imageOffset = {0, 0, 0};
  region.imageExtent = {width, height, 1};

  vkCmdCopyBufferToImage(commandBuffer, buffer, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  endSingleTimeCommands(device, commandBuffer);
}

using Clock = std::chrono::high_resolution_clock;
// Pads an RGBA8 image by extruding edge pixels.
// - srcPixels: output of stbi_load (RGBA8)
// - width, height: original image size
// - paddingPx: padding size in pixels
// - outWidth, outHeight: padded image size
// Returns: newly allocated RGBA8 buffer (caller owns it)
std::vector<uint8_t> padImageRGBA(const std::vector<uint8_t>& srcPixels,
                                  int width, int height, int paddingPx,
                                  int& outWidth, int& outHeight) {
  auto time = Clock::now();
  // Compute padded dimensions
  outWidth = width + paddingPx * 2;
  outHeight = height + paddingPx * 2;

  std::vector<uint8_t> out(outWidth * outHeight * 4, 0);

  // Lambda to access source pixels
  auto src = [&](int x, int y) {
    return srcPixels.data() + (y * width + x) * 4;
  };

  // Lambda to access destination pixels
  auto dst = [&](int x, int y) {
    return out.data() + (y * outWidth + x) * 4;
  };

  // 1️⃣ Copy original image into center
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      std::memcpy(dst(x + paddingPx, y + paddingPx), src(x, y), 4);
    }
  }

  // 2️⃣ Extrude left & right edges
  for (int y = 0; y < height; ++y) {
    for (int p = 0; p < paddingPx; ++p) {
      std::memcpy(dst(p, y + paddingPx), src(0, y), 4);  // left
      std::memcpy(dst(outWidth - 1 - p, y + paddingPx), src(width - 1, y),
                  4);  // right
    }
  }

  // 3️⃣ Extrude top & bottom edges (including corners)
  for (int x = 0; x < outWidth; ++x) {
    for (int p = 0; p < paddingPx; ++p) {
      std::memcpy(dst(x, p), dst(x, paddingPx), 4);  // top
      std::memcpy(dst(x, outHeight - 1 - p), dst(x, outHeight - 1 - paddingPx),
                  4);  // bottom
    }
  }

  return out;
}
std::vector<glm::vec4> convertU8ToVec4(const std::vector<uint8_t>& u8Data,
                                       bool srgb) {
  // Ensure the input size is a multiple of 3 (RGB)
  assert(u8Data.size() % 4 == 0);

  std::vector<glm::vec4> out(u8Data.size() / 4);

#pragma omp parallel for
  for (size_t i = 0; i < u8Data.size(); i += 4) {
    float r = u8Data[i] / 255.0f;
    float g = u8Data[i + 1] / 255.0f;
    float b = u8Data[i + 2] / 255.0f;
    float a = u8Data[i + 3] / 255.0f;
    if (srgb) {
      auto toLinear = [](float c) {
        if (c <= 0.04045f)
          return c / 12.92f;
        else
          return powf((c + 0.055f) / 1.055f, 2.4f);
      };
      r = toLinear(r);
      g = toLinear(g);
      b = toLinear(b);
      a = toLinear(a);
    }

    out[i / 4] = {r, g, b, a};
  }

  return out;
}

std::vector<uint8_t> convertVec4ToU8(const std::vector<glm::vec4>& floatData,
                                     bool srgb) {
  std::vector<uint8_t> out(floatData.size() * 4);

#pragma omp parallel for
  for (int i = 0; i < floatData.size(); i++) {
    glm::vec4 c = floatData[i];
    float r = c.r;
    float g = c.g;
    float b = c.b;
    float a = c.a;

    if (srgb) {
      auto toSrgb = [](float x) -> float {
        if (x <= 0.0031308f)
          return 12.92f * x;
        else
          return 1.055f * powf(x, 1.0f / 2.4f) - 0.055f;
      };
      r = toSrgb(r);
      g = toSrgb(g);
      b = toSrgb(b);
      a = toSrgb(a);
    }

    // Clamp to [0,1] and convert to uint8
    int startInd = 4 * i;
    out[startInd] =
        static_cast<uint8_t>(std::round(std::clamp(r, 0.0f, 1.0f) * 255.0f));
    out[startInd + 1] =
        static_cast<uint8_t>(std::round(std::clamp(g, 0.0f, 1.0f) * 255.0f));
    out[startInd + 2] =
        static_cast<uint8_t>(std::round(std::clamp(b, 0.0f, 1.0f) * 255.0f));
    out[startInd + 3] =
        static_cast<uint8_t>(std::round(std::clamp(a, 0.0f, 1.0f) * 255.0f));
  }

  return out;
}

std::vector<float> makeGaussianKernel(float sigma) {
  float radius = std::ceil(3 * sigma);
  int kernelSize = 2 * radius + 1;
  std::vector<float> kernel(kernelSize);
  float sum = 0;
  for (int i = -radius; i <= radius; i++) {
    kernel[i + radius] = exp(-(i * i) / (2 * sigma * sigma));
    sum += kernel[i + radius];
  }
  for (int i = 0; i < kernelSize; i++) {
    kernel[i] /= sum;
  }
  return kernel;
}

inline int mod(int x, int m) { return (x % m + m) % m; }

std::vector<glm::vec4>
applyHorizontalBlur(const std::vector<glm::vec4>& pixels, int texWidth,
                    int texHeight, const std::vector<float>& kernel) {
  assert(pixels.size() == texWidth * texHeight);
  int radius = kernel.size() / 2;

  std::vector<glm::vec4> destination(pixels.size());
  const glm::vec4* src = pixels.data();      // pointer to input
  glm::vec4* dst = destination.data();  // pointer to output
  int effectiveRad = std::min(radius, texWidth);

#pragma omp parallel for
  for (int y = 0; y < texHeight; ++y) {
    const glm::vec4* row = src + y * texWidth;  // start of row y
    glm::vec4* outRow = dst + y * texWidth;

    for (int x = 0; x < texWidth; ++x) {
      glm::vec4 sum(0.0f);

      for (int k = -effectiveRad; k <= effectiveRad; ++k) {
        sum += kernel[k + radius] * row[(x + k + texWidth) % texWidth];
      }

      outRow[x] = sum;
    }
  }

  return destination;
}

std::vector<glm::vec4>
applyVerticalBlurAndDownscale(const std::vector<glm::vec4>& srcPixels,
                              int texWidth, int texHeight,
                              const std::vector<float>& kernel) {
  int newTexWidth = texWidth / 2;
  int newTexHeight = texHeight / 2;
  int radius = kernel.size() / 2;
  int effectiveRad = std::min(radius, texHeight);
  std::vector<glm::vec4> dstPixels(newTexWidth * newTexHeight);

#pragma omp parallel for
  for (int x = 0; x < newTexWidth; ++x) {
    for (int y = 0; y < newTexHeight; ++y) {
      glm::vec4 sum(0.0f);

      // Vertical blur with wrap-around
      for (int k = -effectiveRad; k <= effectiveRad; ++k) {
        int srcY = 2 * y + k;
        // wrap-around vertically
        srcY = (srcY + texHeight) % texHeight;

        sum += kernel[k + radius] * srcPixels[srcY * texWidth + 2 * x];
      }

      dstPixels[y * newTexWidth + x] = sum;
    }
  }

  return dstPixels;
}
std::vector<uint8_t>
applyGaussianKernel(const std::vector<uint8_t>& pixels, int texWidth,
                    int texHeight, float sigma) {
  std::vector<float> kernel = makeGaussianKernel(sigma);
  std::vector<glm::vec4> rgbFormat = convertU8ToVec4(pixels, false);

  std::vector<glm::vec4> hblurred =
      applyHorizontalBlur(rgbFormat, texWidth, texHeight, kernel);

  std::vector<glm::vec4> vblurred =
      applyVerticalBlurAndDownscale(hblurred, texWidth, texHeight, kernel);

  std::vector<glm::uint8_t> output = convertVec4ToU8(vblurred);

  return output;
}

VkImage createImageFromPixelArray(const VulkanDevice& device,
                                  const std::vector<uint8_t>& pixels,
                                  VkDeviceMemory& imageMemory, VkFormat format,
                                  VkImageUsageFlags usage, std::string name,
                                  int texWidth, int texHeight) {
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;
  VkDeviceSize imageSize = texWidth * texHeight * 4;

  createBufferWithData(device, VulkanBufferType::Staging, imageSize,
                       pixels.data(), stagingBuffer, stagingBufferMemory,
                       "Texture Staging Buffer");

  // Ensure the image is created with TRANSFER_DST and TRANSFER_SRC usage so
  // we can transition it to TRANSFER_SRC_OPTIMAL later when copying from it
  // into an atlas or another image. Also include any usage flags requested
  // by the caller (for example VK_IMAGE_USAGE_SAMPLED_BIT).
  VkImage textureImage =
      createImage(device, {static_cast<uint32_t>(texWidth),
                            static_cast<uint32_t>(texHeight)},
                  format, usage, true, name);

  imageMemory = allocateAndBindImageMemory(device, textureImage);

  transitionImageLayout(device, textureImage, format, VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  copyBufferToImage(device, stagingBuffer, textureImage,
                    static_cast<uint32_t>(texWidth),
                    static_cast<uint32_t>(texHeight));

  transitionImageLayout(device, textureImage, format,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

  vkDestroyBuffer(device.getDevice(), stagingBuffer, nullptr);
  vkFreeMemory(device.getDevice(), stagingBufferMemory, nullptr);
  return textureImage;
}

void createImageAndMipmapsFromFile(
    const VulkanDevice& device, const std::string& filename, VkFormat format,
    VkImageUsageFlags usage, std::vector<VkImage>& mipmaps,
    std::vector<VkDeviceMemory>& mipmapsMemories, int nMipmaps,
    std::string name, std::vector<int>& mipmapsWidths,
    std::vector<int>& mipmapsHeights, int padding) {
  mipmaps.resize(nMipmaps);
  mipmapsMemories.resize(nMipmaps);

  mipmapsWidths.resize(nMipmaps);
  mipmapsHeights.resize(nMipmaps);

  const float SIGMA_TRANSITION =
      0.71f;  // best results combined with textureLod sampling in the shader
  int texChannels;
  stbi_set_flip_vertically_on_load(true);
  stbi_uc* rawPixels =
      stbi_load(filename.c_str(), &mipmapsWidths[0], &mipmapsHeights[0],
                &texChannels, STBI_rgb_alpha);
  if (!rawPixels) {
    throw std::runtime_error("Failed to load texture image!");
  }
  std::vector<uint8_t> rawPixelsArray(
      rawPixels, rawPixels + 4 * mipmapsWidths[0] * mipmapsHeights[0]);

  std::vector<std::vector<uint8_t>> mipmapsPixelArrays(nMipmaps);
  mipmapsPixelArrays[0] = rawPixelsArray;

  for (int i = 1; i < nMipmaps; i++) {
    if (mipmapsWidths[i - 1] > 1) {
      mipmapsPixelArrays[i] =
          applyGaussianKernel(mipmapsPixelArrays[i - 1], mipmapsWidths[i - 1],
                              mipmapsHeights[i - 1], SIGMA_TRANSITION);
      mipmapsWidths[i] = mipmapsWidths[i - 1] / 2;
      mipmapsHeights[i] = mipmapsHeights[i - 1] / 2;
    } else {
      mipmapsPixelArrays[i] = mipmapsPixelArrays[i - 1];
      mipmapsWidths[i] = 1;

      mipmapsHeights[i] = 1;
    }
  }

  for (int i = 0; i < nMipmaps; i++) {
    mipmapsPixelArrays[i] = padImageRGBA(
        mipmapsPixelArrays[i], mipmapsWidths[i], mipmapsHeights[i], padding,
        mipmapsWidths[i], mipmapsHeights[i]);
    mipmaps[i] = createImageFromPixelArray(
        device, mipmapsPixelArrays[i], mipmapsMemories[i], format, usage,
        name + " mipmap " + std::to_string(i), mipmapsWidths[i],
        mipmapsHeights[i]);
  }
  stbi_image_free(rawPixels);
}

VkImage createImageFromFile(const VulkanDevice& device,
                            const std::string& filename, VkFormat format,
                            VkImageUsageFlags usage,
                            VkDeviceMemory& imageMemory, std::string name,
                            int& texWidth, int& texHeight, int padding) {
  int texChannels;
  stbi_set_flip_vertically_on_load(true);
  stbi_uc* rawPixels =
      stbi_load(filename.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
  if (!rawPixels) {
    throw std::runtime_error("Failed to load texture image!");
  }
  std::vector<uint8_t> rawPixelsArray(rawPixels,
                                      rawPixels + 4 * texWidth * texHeight);

  std::vector<uint8_t> pixels =
      padImageRGBA(rawPixelsArray, texWidth, texHeight, padding, texWidth, texHeight);

  VkImage textureImage = createImageFromPixelArray(
      device, pixels, imageMemory, format, usage, name, texWidth, texHeight);
  stbi_image_free(rawPixels);
  return textureImage;
}
void copyImage(const VulkanDevice& device, VkImage src, VkImage dst,
               VkExtent3D extent, VkOffset3D offset) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands(device, "Copy Image");

  VkImageSubresourceLayers subResource{};
  subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  subResource.baseArrayLayer = 0;
  subResource.mipLevel = 0;
  subResource.layerCount = 1;

  VkImageCopy copyRegion{};
  copyRegion.srcSubresource = subResource;
  copyRegion.dstSubresource = subResource;
  copyRegion.extent = extent;
  copyRegion.dstOffset = offset;

  transitionImageLayout(device, dst, DEFAULT_COLOR_FORMAT,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  vkCmdCopyImage(commandBuffer, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst,
                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

  endSingleTimeCommands(device, commandBuffer);

  transitionImageLayout(device, dst, DEFAULT_COLOR_FORMAT,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

VkImage blitDownsizedImage(const VulkanDevice& device, VkImage src,
                           VkFormat format, uint32_t srcWidth,
                           uint32_t srcHeight, uint32_t dstWidth,
                           uint32_t dstHeight, VkDeviceMemory& imageMemory,
                           const char* name) {
  // --- 1. Create destination image ---
  VkExtent2D dstExtent = {dstWidth, dstHeight};

  VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                            VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                            VK_IMAGE_USAGE_SAMPLED_BIT;

  VkImage dst = createImage(device, dstExtent, format, usage, true, name);

  imageMemory = allocateAndBindImageMemory(device, dst);

  // --- 2. Transition ONLY the destination image ---
  transitionImageLayout(device, dst, format,
                        VK_IMAGE_LAYOUT_UNDEFINED,  // assumed initial
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  // Source stays in TRANSFER_SRC_OPTIMAL the entire time.

  // --- 3. Perform the blit ---
  VkCommandBuffer cmd = beginSingleTimeCommands(device, "Blit Image");

  VkImageBlit region{};
  region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.srcSubresource.layerCount = 1;
  region.srcOffsets[1] = {(int)srcWidth, (int)srcHeight, 1};

  region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.dstSubresource.layerCount = 1;
  region.dstOffsets[1] = {(int)dstWidth, (int)dstHeight, 1};

  vkCmdBlitImage(cmd, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst,
                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region,
                 VK_FILTER_LINEAR);

  endSingleTimeCommands(device, cmd);

  // --- 4. Transition destination to shader-read layout ---
  transitionImageLayout(device, dst, format, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

  // Source stays unchanged: still TRANSFER_SRC_OPTIMAL

  return dst;
}
