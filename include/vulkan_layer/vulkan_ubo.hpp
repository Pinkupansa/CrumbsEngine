#pragma once
#include <glm/glm.hpp>

struct VulkanUniformBufferObject {
    glm::mat4 model;                  // 16-byte aligned, occupies 64 bytes
    glm::vec2 atlasOffset;            // offset: 64, size: 8 → next member must start at 16-byte boundary
    glm::vec2 textureSize;            // offset: 72
    glm::vec2 normalmapAtlasOffset;   // offset: 80
    glm::vec2 normalmapTextureSize;   // offset: 88
    glm::vec2 tilingFactor;           // offset: 96

    alignas(4) uint32_t castsShadows; // use uint32_t instead of bool → offset: 104
    alignas(4) uint32_t isLit;        // offset: 108

    // Total size will be rounded up to 16-byte multiple → 112 bytes
};
