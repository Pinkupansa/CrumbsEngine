#pragma once
#include <glm/glm.hpp>

struct VulkanUniformBufferObject {
    glm::mat4 model;
    int textureIndex = -1;
};
