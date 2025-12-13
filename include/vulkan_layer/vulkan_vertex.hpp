#pragma once
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vector>

struct Vertex {
    const glm::vec3 pos;
    const glm::vec3 color;
    const glm::vec3 normal; // <-- new
    const glm::vec3 tangent;
    const glm::vec3 bitangent;
    const glm::vec2 uv;
    

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription binding{};
        binding.binding = 0;
        binding.stride = sizeof(Vertex);
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return binding;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attribs;
        attribs.resize(6);
        // Position
        attribs[0].binding = 0;
        attribs[0].location = 0;
        attribs[0].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
        attribs[0].offset = offsetof(Vertex, pos);


        // Color
        attribs[1].binding = 0;
        attribs[1].location = 1;
        attribs[1].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
        attribs[1].offset = offsetof(Vertex, color);

        // Normal
        attribs[2].binding = 0;
        attribs[2].location = 2;
        attribs[2].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3
        attribs[2].offset = offsetof(Vertex, normal);

        //Tangent
        attribs[3].binding = 0;
        attribs[3].location = 3;
        attribs[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        attribs[3].offset = offsetof(Vertex, tangent);

        //Bitangent
        attribs[4].binding = 0;
        attribs[4].location = 4;
        attribs[4].format = VK_FORMAT_R32G32B32_SFLOAT;


        // UV
        attribs[5].binding = 0;
        attribs[5].location = 5;
        attribs[5].format = VK_FORMAT_R32G32_SFLOAT; // vec2
        attribs[5].offset = offsetof(Vertex, uv);

        

        return attribs;
    }
};
