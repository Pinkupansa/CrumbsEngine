#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.h>

struct VulkanUniformBufferObject {
    alignas(16) glm::mat4 model;                  // 16-byte aligned, occupies 64 byte
    bool castsShadows;
};
std::vector<uint8_t> padUBOData (std::vector<VulkanUniformBufferObject> ubos,
                                  VkDeviceSize alignedSize);