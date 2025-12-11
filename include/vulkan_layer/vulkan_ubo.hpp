#pragma once
#include <glm/glm.hpp>

struct VulkanUniformBufferObject {
    glm::mat4 model;
    glm::vec2 atlasOffset;
    glm::vec2 textureSize;
    glm::vec2 tilingFactor; 
};
