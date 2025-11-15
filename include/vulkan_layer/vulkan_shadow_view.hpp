#pragma once 
#include <vulkan/vulkan.h> 
#include <vector> 
#include "vulkan_device.hpp"
class VulkanShadowView{
    private: 
        VulkanDevice& pDevice;
        VkImage shadowImage;
        VkImageView shadowImageView;
        VkExtent2D extent;
        VkDeviceMemory shadowMemory;
        VkSampler shadowSampler;
        VkDescriptorSetLayout shadowDescLayout;
        VkDescriptorPool shadowDescPool;
        VkDescriptorSet shadowDescSet; 

    public: 
        std::vector<std::vector<VkImageView>> getAttachmentsPerImage(){
            return {{shadowImageView}};
        }
        VkExtent2D getExtent(){
            return extent;
        }
        const VkDescriptorSet& getDescSet() const{
            return shadowDescSet;
        }
        const VkDescriptorSetLayout getLayout() const{
            return shadowDescLayout;
        }
        VulkanShadowView(VulkanDevice& device, uint width, uint height, VkFormat format): pDevice(device){
            extent = {width, height};
            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = width;   // shadow map resolution
            imageInfo.extent.height = height;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format = format;  // recommended for shadows
            imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | 
                  VK_IMAGE_USAGE_SAMPLED_BIT | 
                  VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

            imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            
            vkCreateImage(device.getDevice(), &imageInfo, nullptr, &shadowImage);
            
            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(device.getDevice(), shadowImage, &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = device.findMemoryType(memRequirements.memoryTypeBits, 
                                                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

            
            vkAllocateMemory(device.getDevice(), &allocInfo, nullptr, &shadowMemory);
            vkBindImageMemory(device.getDevice(), shadowImage, shadowMemory, 0);

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = shadowImage;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = format;
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;  // depth only!
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device.getDevice(), &viewInfo, nullptr, &shadowImageView) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create shadow image view!");
            }  
            
            // Create a sampler for sampling the shadow map in the fragment shader
            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = VK_FALSE;       
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

            vkCreateSampler(device.getDevice(), &samplerInfo, nullptr, &shadowSampler);
        
            VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

            VkDescriptorSetLayoutBinding sampLayout;
            sampLayout.binding = 0; 
            sampLayout.descriptorType = descriptorType; 
            sampLayout.descriptorCount = 1;
            sampLayout.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            sampLayout.pImmutableSamplers = nullptr; 

            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = 1;
            layoutInfo.pBindings = &sampLayout;
            
            if (vkCreateDescriptorSetLayout(device.getDevice(), &layoutInfo, nullptr, &shadowDescLayout) != VK_SUCCESS)
                throw std::runtime_error("Failed to create descriptor set layout!");

            VkDescriptorPoolSize poolSize{};
            poolSize.type = descriptorType;
            poolSize.descriptorCount = 1;

            VkDescriptorPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO; 
            poolInfo.poolSizeCount = 1; 
            poolInfo.pPoolSizes = &poolSize;
            poolInfo.maxSets = 1;

            if (vkCreateDescriptorPool(device.getDevice(), &poolInfo, nullptr, &shadowDescPool) != VK_SUCCESS)
                throw std::runtime_error("Failed to create descriptor pool!");
            
            VkDescriptorSetAllocateInfo setAllocInfo{};
            setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            setAllocInfo.descriptorPool = shadowDescPool;
            setAllocInfo.descriptorSetCount = 1;
            setAllocInfo.pSetLayouts = &shadowDescLayout;

            if (vkAllocateDescriptorSets(device.getDevice(), &setAllocInfo, &shadowDescSet) != VK_SUCCESS)
                throw std::runtime_error("Failed to allocate descriptor set!");

            VkDescriptorImageInfo shadowInfo{};
            shadowInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            shadowInfo.imageView = shadowImageView;  // your shadow depth view
            shadowInfo.sampler   = shadowSampler;    // sampler

            VkWriteDescriptorSet shadowWrite{};
            shadowWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            shadowWrite.dstSet = shadowDescSet;
            shadowWrite.dstBinding = 0;
            shadowWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            shadowWrite.descriptorCount = 1;
            shadowWrite.pImageInfo = &shadowInfo;

            vkUpdateDescriptorSets(device.getDevice(), 1, &shadowWrite, 0, nullptr);
        }
        ~VulkanShadowView(){
            destroy();
        }
        void destroy(){
            if (shadowDescLayout != VK_NULL_HANDLE) {
                vkDestroyDescriptorSetLayout(pDevice.getDevice(), shadowDescLayout, nullptr);
                shadowDescLayout = VK_NULL_HANDLE;
            }

            if (shadowDescPool != VK_NULL_HANDLE) {
                vkDestroyDescriptorPool(pDevice.getDevice(), shadowDescPool, nullptr);
                shadowDescPool = VK_NULL_HANDLE;
            }

            // Descriptor sets are freed implicitly when the pool is destroyed,
            // but you can explicitly free them if needed:
            if (shadowDescSet != VK_NULL_HANDLE && shadowDescPool != VK_NULL_HANDLE) {
                vkFreeDescriptorSets(pDevice.getDevice(), shadowDescPool, 1, &shadowDescSet);
                shadowDescSet = VK_NULL_HANDLE;
            }

            if(shadowSampler != VK_NULL_HANDLE){
                vkDestroySampler(pDevice.getDevice(), shadowSampler, nullptr);
                shadowSampler = VK_NULL_HANDLE;
            }
            if(shadowMemory != VK_NULL_HANDLE){
                vkFreeMemory(pDevice.getDevice(), shadowMemory, nullptr);
                shadowMemory = VK_NULL_HANDLE;
            }
            if(shadowImage != VK_NULL_HANDLE){
                vkDestroyImage(pDevice.getDevice(), shadowImage, nullptr);
                shadowImage = VK_NULL_HANDLE;
            }
            if(shadowImageView != VK_NULL_HANDLE){
                vkDestroyImageView(pDevice.getDevice(), shadowImageView, nullptr);
                shadowImageView = VK_NULL_HANDLE;
            }
            
        }
    
};