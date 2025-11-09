#pragma once
#include <vulkan/vulkan.h>

struct MeshDrawInfo{
    uint32_t vertexOffset;
    uint32_t indexOffset;
    uint32_t indexCount;
};