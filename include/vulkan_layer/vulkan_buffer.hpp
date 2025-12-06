#pragma once
#include "vulkan_vertex.hpp"
#include "vulkan_device.hpp"
#include <cstring>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.h>

enum class VulkanBufferType {
    Vertex,
    Index,
    Uniform,
    Staging
};
#include "vulkan_object_creation_utils.hpp"

class VulkanBuffer {

private:
    VkBuffer buffer{VK_NULL_HANDLE};
    VkDeviceMemory memory{VK_NULL_HANDLE};
    VkDeviceSize size;
    VkDeviceSize alignedObjectSize;
    bool dynamic;

public:
    const VkBuffer &getBuffer() const {
        return buffer;
    }

    const bool isDynamic() const {
        return dynamic;
    }

    const VkDeviceSize getAlignedObjectSize() const {
        return alignedObjectSize;
    }

    VulkanBuffer(VulkanDevice &deviceRef, VulkanBufferType type, VkDeviceSize size, const void *data = nullptr, bool dynamic = false, VkDeviceSize alignedObjectSize = 0, std::string name = "Buffer")
        : device(deviceRef), type(type), size(size), dynamic(dynamic), alignedObjectSize(alignedObjectSize) {

        buffer = createBuffer(device, type, size, name);

        memory = allocateAndBindBufferMemory(device, buffer);

        if (data)
            update(data, size, 0);
    }

    ~VulkanBuffer() {
        destroy();
    }

    void destroy() {
        if (buffer != VK_NULL_HANDLE)
            vkDestroyBuffer(device.getDevice(), buffer, nullptr);
        if (memory != VK_NULL_HANDLE)
            vkFreeMemory(device.getDevice(), memory, nullptr);
        buffer = VK_NULL_HANDLE;
        memory = VK_NULL_HANDLE;
    }

    void update(const void *data, VkDeviceSize size, VkDeviceSize offset) {
        void *mapped;
        vkMapMemory(device.getDevice(), memory, offset, size, 0, &mapped);
        std::memcpy(mapped, data, static_cast<size_t>(size));
        vkUnmapMemory(device.getDevice(), memory);
    }

    const VkDeviceSize getSize() const {
        return size;
    }

private:
    VulkanDevice &device;
    VulkanBufferType type;
};
