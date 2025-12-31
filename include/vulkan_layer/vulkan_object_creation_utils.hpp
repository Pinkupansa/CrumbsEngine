#pragma once
#include <vector>
#include <vulkan/vulkan.h>

#include "vulkan_buffer.hpp"
#include "vulkan_device.hpp"
#include "vulkan_surface.hpp"
#include <glm/glm.hpp>

// Forward declaration of VulkanDevice

VkImage createImage (const VulkanDevice& device,
                     VkExtent2D extent,
                     VkFormat format,
                     VkImageUsageFlags imageUsage,
                     bool isSinglesampled,
                     std::string name);

VkImage createColorImage (const VulkanDevice& device,
                          VkExtent2D extent,
                          bool isSingleSampled,
                          std::string name,
                          VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

VkImage createDepthImage (const VulkanDevice& device,
                          VkExtent2D extent,
                          bool isSingleSampled,
                          std::string name,
                          VkImageUsageFlags usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

VkImageView createImageView (const VulkanDevice& device,
                             const VkImage& image,
                             VkFormat format,
                             VkImageAspectFlags aspectMask,
                             std::string name);

VkImageView
createColorImageView (const VulkanDevice& device, const VkImage& image, std::string name);

VkImageView
createDepthImageView (const VulkanDevice& device, const VkImage& image, std::string name);

VkSwapchainKHR createTripleBufferingSwapchain (const VulkanDevice& device,
                                               const VulkanSurface& surface,
                                               std::string name);

VkMemoryAllocateInfo createMemoryAllocateInfo (const VulkanDevice& device,
                                               const VkMemoryRequirements& memReq,
                                               VkMemoryPropertyFlags properties);

VkDeviceMemory allocateMemory (const VulkanDevice& device,
                               VkMemoryRequirements memReq,
                               VkMemoryPropertyFlags properties);

VkDeviceMemory allocateAndBindImageMemory (const VulkanDevice& device, const VkImage& image);

VkDeviceMemory
allocateAndBindBufferMemory (const VulkanDevice& device, const VkBuffer& buffer);

VkAttachmentDescription
createColorAttachment (bool isSingleSampled,
                       VkAttachmentLoadOp loadOp,
                       VkImageLayout finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

VkAttachmentReference createColorAttachmentRef (int attachmentNumber);

VkAttachmentDescription createDepthAttachment (VkAttachmentLoadOp loadOp);
VkAttachmentDescription createShadowDepthAttachment (VkAttachmentLoadOp loadOp);
VkAttachmentReference createDepthAttachmentRef (int attachmentNumber);

VkBuffer createBuffer (const VulkanDevice& device,
                       VulkanBufferType type,
                       VkDeviceSize bufferSize,
                       std::string name);

VkDescriptorSetLayout createDescriptorLayout (const VulkanDevice& device,
                                              VkDescriptorType descriptorType,
                                              VkShaderStageFlags shaderStageFlags,
                                              uint32_t binding,
                                              uint32_t descriptorCount = 1,
                                              VkDescriptorBindingFlags bindingFlags = 0);

VkDescriptorPool createDescriptorPool (const VulkanDevice& device,
                                       VkDescriptorType descriptorType,
                                       uint32_t descriptorCount = 1);

VkDescriptorSet allocateDescriptorSet (const VulkanDevice& device,
                                       const VkDescriptorSetLayout& layout,
                                       const VkDescriptorPool& pool,
                                       std::string name,
                                       uint32_t actualCount = 0,
                                       bool isVariableCount = false);

void writeBufferInDescriptorSet (const VulkanDevice& device,
                                 const VkBuffer& buffer,
                                 const VkDescriptorSet& descriptorSet,
                                 VkDescriptorType descriptorType,
                                 VkDeviceSize unalignedObjectSize);

void writeImageSamplerInDescriptorSet (const VulkanDevice& device,
                                       const VkImageView& imageView,
                                       const VkSampler& sampler,
                                       const VkDescriptorSet& descSet);

void writeImageSamplerInDescriptorSetArray (const VulkanDevice& device,
                                            const VkImageView& imageView,
                                            const VkSampler& sampler,
                                            const VkDescriptorSet& descSet,
                                            uint32_t binding,
                                            uint32_t arrayElement);

VkShaderModule createShaderModule (std::vector<char> code, const VkDevice& device);

VkPipelineShaderStageCreateInfo
createVertexShaderStageCreateInfo (const VkShaderModule& vertexShaderModule);

VkPipelineShaderStageCreateInfo
createFragmentShaderStageCreateInfo (const VkShaderModule& fragmentShaderModule);

VkPipelineVertexInputStateCreateInfo
createVertexInputInfo (const VkVertexInputBindingDescription& bindingDescription,
                       const std::vector<VkVertexInputAttributeDescription>& attributeDescriptions);

VkPipelineVertexInputStateCreateInfo createFullscreenVertexInputInfo ();
VkPipelineInputAssemblyStateCreateInfo createDefaultInputAssemblyInfo ();

VkViewport createViewport (VkExtent2D viewportExtent);

VkRect2D createScissor (VkExtent2D viewportExtent);

VkPipelineViewportStateCreateInfo
createViewportStateInfo (const VkViewport& viewport, const VkRect2D& scissor);

VkPipelineRasterizationStateCreateInfo createDefaultRasterizerInfo (VkCullModeFlagBits cullMode);

VkPipelineMultisampleStateCreateInfo createMSAAInfo (bool hasResolve);

VkPipelineColorBlendAttachmentState createFullColorBlendAttachment ();
VkPipelineColorBlendAttachmentState createAdditiveAlphaBlendAttachment ();
VkPipelineColorBlendAttachmentState createWeightedAlphaBlendAttachment ();
VkPipelineColorBlendStateCreateInfo
createColorBlendStateInfo (const VkPipelineColorBlendAttachmentState& colorBlendAttachment);

VkPipelineLayout
createPipelineLayout (const VulkanDevice& device,
                      const std::vector<VkDescriptorSetLayout>& descLayouts,
                      const std::vector<VkPushConstantRange>& pushConstantRanges,
                      std::string name);

VkPipelineDepthStencilStateCreateInfo
createDepthStencilInfo (bool enableDepthTest, bool enableDepthWrite);

VkPipeline
createPipeline (const VulkanDevice& device,
                const std::vector<VkPipelineShaderStageCreateInfo>& shaderStagesInfos,
                const VkPipelineVertexInputStateCreateInfo& vertexInputInfo,
                const VkPipelineInputAssemblyStateCreateInfo& inputAssemblyInfo,
                const VkPipelineViewportStateCreateInfo& viewportStateInfo,
                const VkPipelineRasterizationStateCreateInfo& rasterizerInfo,
                const VkPipelineMultisampleStateCreateInfo& msaaInfo,
                const VkPipelineColorBlendStateCreateInfo& colorBlendingInfo,
                const VkPipelineLayout& pipelineLayout,
                const VkRenderPass& renderPass,
                const VkPipelineDepthStencilStateCreateInfo& depthStencil,
                std::string name);

VkFramebuffer createFramebuffer (const VulkanDevice& device,
                                 const VkRenderPass& rp,
                                 const std::vector<VkImageView> attachments,
                                 VkExtent2D extent,
                                 std::string name);

VkRenderPassBeginInfo createRenderPassBeginInfo (const VkRenderPass& rp,
                                                 const VkFramebuffer& framebuffer,
                                                 const std::vector<VkClearValue>& clearValues,
                                                 VkExtent2D renderAreaExtent);

VkClearValue createColorClearValue (VkClearColorValue color);

VkClearValue createDepthClearValue (VkClearDepthStencilValue depthValue);

VkSampler createSampler (const VulkanDevice& device, std::string name);

void createBufferWithData (const VulkanDevice& device,
                           VulkanBufferType type,
                           VkDeviceSize bufferSize,
                           const void* data,
                           VkBuffer& buffer,
                           VkDeviceMemory& bufferMemory,
                           std::string name);
VkCommandBuffer beginSingleTimeCommands (VulkanDevice const& device, std::string name);

void endSingleTimeCommands (const VulkanDevice& device, VkCommandBuffer commandBuffer);

void transitionImageLayout (const VulkanDevice& device,
                            VkImage image,
                            VkFormat format,
                            VkImageLayout oldLayout,
                            VkImageLayout newLayout);

void copyBufferToImage (const VulkanDevice& device,
                        VkBuffer buffer,
                        VkImage image,
                        uint32_t width,
                        uint32_t height);

std::vector<uint8_t> padImageRGBA (const std::vector<uint8_t>& srcPixels,
                                   int width,
                                   int height,
                                   int paddingPx,
                                   int& outWidth,
                                   int& outHeight);
std::vector<glm::vec4> convertU8ToVec4(const std::vector<uint8_t>& u8Data,
                                       bool srgb);

std::vector<uint8_t>
convertVec4ToU8 (const std::vector<glm::vec4>& floatData, bool srgb = false);

std::vector<float> makeGaussianKernel (float sigma);

int mod (int x, int m);

std::vector<glm::vec4> applyHorizontalBlur (const std::vector<glm::vec4>& pixels,
                                            int texWidth,
                                            int texHeight,
                                            const std::vector<float>& kernel);

std::vector<glm::vec4> applyVerticalBlurAndDownscale (const std::vector<glm::vec4>& srcPixels,
                                                      int texWidth,
                                                      int texHeight,
                                                      const std::vector<float>& kernel);
std::vector<uint8_t>
applyGaussianKernel (const std::vector<uint8_t>& pixels, int texWidth, int texHeight, float sigma);

VkImage createImageFromPixelArray (const VulkanDevice& device,
                                   const std::vector<uint8_t>& pixels,
                                   VkDeviceMemory& imageMemory,
                                   VkFormat format,
                                   VkImageUsageFlags usage,
                                   std::string name,
                                   int texWidth,
                                   int texHeight);

void createImageAndMipmapsFromFile (const VulkanDevice& device,
                                    const std::string& filename,
                                    VkFormat format,
                                    VkImageUsageFlags usage,
                                    std::vector<VkImage>& mipmaps,
                                    std::vector<VkDeviceMemory>& mipmapsMemories,
                                    int nMipmaps,
                                    std::string name,
                                    std::vector<int>& mipmapsWidths,
                                    std::vector<int>& mipmapsHeights,
                                    int padding);

VkImage createImageFromFile (const VulkanDevice& device,
                             const std::string& filename,
                             VkFormat format,
                             VkImageUsageFlags usage,
                             VkDeviceMemory& imageMemory,
                             std::string name,
                             int& texWidth,
                             int& texHeight,
                             int padding);
void copyImage (const VulkanDevice& device, VkImage src, VkImage dst, VkExtent3D extent, VkOffset3D offset);

VkImage blitDownsizedImage (const VulkanDevice& device,
                            VkImage src,
                            VkFormat format,
                            uint32_t srcWidth,
                            uint32_t srcHeight,
                            uint32_t dstWidth,
                            uint32_t dstHeight,
                            VkDeviceMemory& imageMemory,
                            const char* name);