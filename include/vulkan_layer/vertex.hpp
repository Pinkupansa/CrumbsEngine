#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>  
struct Vertex {
    const glm::vec3 pos; 
    const glm::vec3 color; 
    const glm::vec3 normal;   // <-- new

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription binding{};
        binding.binding = 0;                       
        binding.stride = sizeof(Vertex);           
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return binding;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attribs{};
        
        // Position
        attribs[0].binding  = 0;
        attribs[0].location = 0;
        attribs[0].format   = VK_FORMAT_R32G32B32_SFLOAT; // vec3
        attribs[0].offset   = offsetof(Vertex, pos);

        // Color
        attribs[1].binding  = 0;
        attribs[1].location = 1;
        attribs[1].format   = VK_FORMAT_R32G32B32_SFLOAT; // vec3
        attribs[1].offset   = offsetof(Vertex, color);

        // Normal
        attribs[2].binding  = 0;
        attribs[2].location = 2;
        attribs[2].format   = VK_FORMAT_R32G32B32_SFLOAT; // vec3
        attribs[2].offset   = offsetof(Vertex, normal);

        return attribs;
    }
};
