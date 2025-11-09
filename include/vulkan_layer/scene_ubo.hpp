#pragma once
#include <glm/glm.hpp>

struct SceneUBO {
    glm::mat4 view;
    glm::mat4 proj; 
    glm::vec3 lightDir;
    alignas(16) glm::vec3 lightColor; // Vulkan requires 16-byte alignment for vec3
};