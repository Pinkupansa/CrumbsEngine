#include "vulkan_layer/vulkan_render_pass.hpp"

#include "engine_layer/debug.hpp"
#include "vulkan_layer/vulkan_attachment.hpp"
#include "vulkan_layer/vulkan_constants.hpp"
#include "vulkan_layer/vulkan_device.hpp"
#include "vulkan_layer/vulkan_object_creation_utils.hpp"
#include <vulkan/vulkan.h>

std::vector<VkClearValue> VulkanRenderPass::getClearValues() const {
  return clearValues;
}
const VkRenderPass& VulkanRenderPass::getRenderPass() const {
  return renderPass;
}

const bool VulkanRenderPass::hasResolveAttachment() const {
  return hasResolve;
}  // used to set nRasterizationSamples in pipeline

VulkanRenderPass::VulkanRenderPass(
    const VulkanDevice& device,
    const std::vector<VulkanAttachment*>& attachments)
    : pDevice(device) {
  std::vector<VkAttachmentDescription> colorAttachmentDescs;
  std::vector<VkAttachmentDescription> depthAttachmentDescs;
  std::vector<VkAttachmentDescription> resolveAttachmentDescs;

  std::vector<VkClearValue> colorClearValues;
  std::vector<VkClearValue> depthClearValues;
  std::vector<VkClearValue> resolveClearValues;

  for (auto& attachment : attachments) {
    VkAttachmentDescription desc = attachment->createAttachmentDesc();
    switch (attachment->getType()) {
      case VulkanAttachmentType::Color:
        if (attachment->isColorResolveAttachment()) {
          resolveAttachmentDescs.push_back(desc);
          resolveClearValues.push_back(attachment->getClearValue());

        } else {
          colorAttachmentDescs.push_back(desc);
          colorClearValues.push_back(attachment->getClearValue());
        }
        break;
      case VulkanAttachmentType::Depth:
        depthAttachmentDescs.push_back(desc);
        depthClearValues.push_back(attachment->getClearValue());
        break;
      case VulkanAttachmentType::ShadowMap:
        depthAttachmentDescs.push_back(desc);
        depthClearValues.push_back(attachment->getClearValue());
        break;
    }
  }

  int attachmentCount = 0;
  hasResolve = resolveAttachmentDescs.size() > 0;
  for (int i = 0; i < colorAttachmentDescs.size(); i++) {
    clearValues.push_back(colorClearValues[i]);
  }

  for (int i = 0; i < depthAttachmentDescs.size(); i++) {
    clearValues.push_back(depthClearValues[i]);
  }
  for (int i = 0; i < resolveAttachmentDescs.size(); i++) {
    clearValues.push_back(resolveClearValues[i]);
  }
  std::vector<VkAttachmentReference> colorAttachmentsRefs;
  for (VkAttachmentDescription cAtt : colorAttachmentDescs) {
    colorAttachmentsRefs.push_back(createColorAttachmentRef(attachmentCount));
    attachmentCount++;
  }

  std::vector<VkAttachmentReference> depthAttachmentsRefs;
  for (VkAttachmentDescription dAtt : depthAttachmentDescs) {
    depthAttachmentsRefs.push_back(createDepthAttachmentRef(attachmentCount));
    attachmentCount++;
  }

  std::vector<VkAttachmentReference> resolveAttachmentsRefs;
  for (VkAttachmentDescription rAtt : depthAttachmentDescs) {
    resolveAttachmentsRefs.push_back(createColorAttachmentRef(attachmentCount));
    attachmentCount++;
  }
  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = colorAttachmentsRefs.size();
  subpass.pColorAttachments =
      colorAttachmentsRefs.size() > 0 ? colorAttachmentsRefs.data() : nullptr;
  subpass.pDepthStencilAttachment =
      depthAttachmentsRefs.size() > 0 ? depthAttachmentsRefs.data() : nullptr;
  Debug::Log(std::to_string(resolveAttachmentDescs.size()));
  subpass.pResolveAttachments = hasResolve ? resolveAttachmentsRefs.data() : nullptr;

  std::vector<VkAttachmentDescription> attachmentDescs = colorAttachmentDescs;
  attachmentDescs.insert(attachmentDescs.end(), depthAttachmentDescs.begin(),
                         depthAttachmentDescs.end());
  attachmentDescs.insert(attachmentDescs.end(), resolveAttachmentDescs.begin(),
                         resolveAttachmentDescs.end());

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = attachmentDescs.size();
  renderPassInfo.pAttachments = attachmentDescs.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;

  if (vkCreateRenderPass(device.getDevice(), &renderPassInfo, nullptr,
                         &renderPass) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create render pass!");
  }

  device.nameObject((uint64_t)renderPass, VK_OBJECT_TYPE_RENDER_PASS,
                    "RenderPass");
}

VulkanRenderPass::~VulkanRenderPass() { destroy(); }

void VulkanRenderPass::destroy() {
  if (renderPass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(pDevice.getDevice(), renderPass, nullptr);
    renderPass = VK_NULL_HANDLE;
  }
}
