#pragma once 
#include <vulkan/vulkan.h>
#include "vulkan_device.hpp"
#include "vulkan_object_creation_utils.hpp"
#include "vulkan_descriptor_data.hpp"

//Holds necessary info to attach a texture to a shader.
//TODO : add option to create image from file ? 
class VulkanTextureDescriptor{
    private: 
        const VulkanDevice& device;
        VkSampler sampler;
        VkDescriptorSetLayout descLayout;
        VkDescriptorPool descPool;
        VkDescriptorSet descSet;
    
    public: 
        const VulkanDescriptorData getDescData (int binding, int set) const {
        return { descSet,
                 descLayout,
                 descPool,
                 false,
                 0,
                 binding,
                 set };
    }
    VulkanTextureDescriptor(const VulkanDevice& device, VkImageView imageView, std::string name) : device(device){
        sampler = createSampler(device, name + " Sampler");
        descLayout = createDescriptorLayout(device, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                VK_SHADER_STAGE_FRAGMENT_BIT, 0);
        descPool = createDescriptorPool(device, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        
        descSet = allocateDescriptorSet(device, descLayout, descPool, name + " Descriptor Set");
        
        writeImageSamplerInDescriptorSet(device, imageView, sampler, descSet);
    }

    ~VulkanTextureDescriptor () {
        destroy ();
    }
    void destroy(){
        if (descLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout (device.getDevice (), descLayout, nullptr);
            descLayout = VK_NULL_HANDLE;
        }

        if (descPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool (device.getDevice (), descPool, nullptr);
            descPool = VK_NULL_HANDLE;
        }

        // Descriptor sets are freed implicitly when the pool is destroyed,
        // but you can explicitly free them if needed:
        if (descSet != VK_NULL_HANDLE && descPool != VK_NULL_HANDLE) {
            vkFreeDescriptorSets (device.getDevice (), descPool, 1, &descSet);
            descSet = VK_NULL_HANDLE;
        }

        if (sampler != VK_NULL_HANDLE) {
            vkDestroySampler (device.getDevice (), sampler, nullptr);
            sampler = VK_NULL_HANDLE;
        }
    }

};