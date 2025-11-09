#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <cstring>
#include <stdexcept>
#include "vertex.hpp"
#include "vulkan_device.hpp"

enum class VulkanBufferType {
    Vertex,
    Index,
    Uniform
};

class VulkanBuffer {

private:
    
    VkBuffer buffer{ VK_NULL_HANDLE };
    VkDeviceMemory memory{ VK_NULL_HANDLE };
    VkDeviceSize size; 
    VkDeviceSize alignedObjectSize;
    bool dynamic;
public:
    
    const VkBuffer& getBuffer() const{
        return buffer;
    }

    const bool isDynamic() const{
        return dynamic;
    }

    const VkDeviceSize getAlignedObjectSize() const{
        return alignedObjectSize;
    }
    
    VulkanBuffer(VulkanDevice& deviceRef, VulkanBufferType type, VkDeviceSize size, const void* data = nullptr, bool dynamic = false, VkDeviceSize alignedObjectSize = 0, std::string name = "Buffer")
        : device(deviceRef), type(type), size(size), dynamic(dynamic), alignedObjectSize(alignedObjectSize)
    {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        switch (type) {
            case VulkanBufferType::Vertex:
                bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
                break;
            case VulkanBufferType::Index:
                bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
                break;
            case VulkanBufferType::Uniform:
                bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
                break;
        }

        if (vkCreateBuffer(device.getDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
            throw std::runtime_error("Failed to create buffer!");

        deviceRef.nameObject((uint64_t)buffer, VK_OBJECT_TYPE_BUFFER, name);
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device.getDevice(), buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = device.findMemoryType(
            memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
        );

        if (vkAllocateMemory(device.getDevice(), &allocInfo, nullptr, &memory) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate buffer memory!");

        vkBindBufferMemory(device.getDevice(), buffer, memory, 0);

        
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

    void update(const void* data, VkDeviceSize size, VkDeviceSize offset) {
        void* mapped;
        vkMapMemory(device.getDevice(), memory, offset, size, 0, &mapped);
        std::memcpy(mapped, data, static_cast<size_t>(size));
        vkUnmapMemory(device.getDevice(), memory);
    }
    const VkDeviceSize getSize() const{
            return size;
    }
private:
    VulkanDevice& device;
    VulkanBufferType type;
};
