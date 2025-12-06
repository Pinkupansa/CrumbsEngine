#pragma once
#include "vulkan_ubo.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_device.hpp"
#include <vulkan/vulkan.h>
#include "vulkan_descriptor_data.hpp"
class VulkanUBDescriptor {
    private:
    VulkanDevice& device;
    VkDescriptorSetLayout layout{ VK_NULL_HANDLE };
    VkDescriptorPool pool{ VK_NULL_HANDLE };
    VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
    VkDeviceSize alignedObjectSize;
    bool dynamic;
    public:
    const VkDescriptorSetLayout& getLayout () const {
        return layout;
    }

    const VkDeviceSize& getAlignedObjectSize () const {
        return alignedObjectSize;
    }

    const bool isDynamic() const{
        return dynamic;
    }
    const VulkanDescriptorData getDescData(int binding, int set) const{
        return {descriptorSet, layout, pool, dynamic, alignedObjectSize, binding, set};
    }

    VulkanUBDescriptor (VulkanDevice& device,
                      VulkanBuffer& uniformBuffer,
                      VkShaderStageFlags stageFlags,
                      VkDeviceSize unalignedObjectSize,
                      std::string name)
    : device (device), dynamic(uniformBuffer.isDynamic()) {
        VkDescriptorType descriptorType = uniformBuffer.isDynamic () ?
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC :
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        alignedObjectSize               = uniformBuffer.getAlignedObjectSize ();
        // binding for the ubo
        layout = createDescriptorLayout (device, descriptorType, stageFlags, 0);

        pool = createDescriptorPool (device, descriptorType);

        descriptorSet = allocateDescriptorSet (device, layout, pool, name);
        writeBufferInDescriptorSet (device, uniformBuffer.getBuffer (), descriptorSet,
                                    descriptorType, unalignedObjectSize);
    }

    ~VulkanUBDescriptor () {
        destroy ();
    }

    void destroy () {
        if (pool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool (device.getDevice (), pool, nullptr);
            pool = VK_NULL_HANDLE;
        }
        if (layout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout (device.getDevice (), layout, nullptr);
            layout = VK_NULL_HANDLE;
        }
    }

    const VkDescriptorSet& getDescriptorSet () const {
        return descriptorSet;
    }
};
