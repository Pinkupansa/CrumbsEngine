#pragma once

#include <vulkan/vulkan.hpp>

struct VulkanDescriptorData {
    VkDescriptorSet descriptorSet;
    VkDescriptorSetLayout layout;
    VkDescriptorPool pool;
    bool isDynamicBuffer;
    VkDeviceSize alignedObjectSize; //used if dynamic
    int binding;
    int set;
};