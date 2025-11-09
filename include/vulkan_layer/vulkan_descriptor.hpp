
#include <vulkan/vulkan.h>
#include "vulkan_device.hpp"
#include "vulkan_buffer.hpp"
#include "ubo.hpp"
class VulkanDescriptor {
private:
    VulkanDevice& device;
    VkDescriptorSetLayout layout{ VK_NULL_HANDLE };
    VkDescriptorPool pool{ VK_NULL_HANDLE };
    VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
    VkDeviceSize alignedObjectSize;
public:

    const VkDescriptorSetLayout& getLayout() const{
        return layout;
    }

    const VkDeviceSize& getAlignedObjectSize() const{
        return alignedObjectSize;
    }
    VulkanDescriptor(VulkanDevice& device, VulkanBuffer& uniformBuffer, VkShaderStageFlags stageFlags, VkDeviceSize unalignedObjectSize): device(device){
        VkDescriptorType descriptorType = uniformBuffer.isDynamic()? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        alignedObjectSize = uniformBuffer.getAlignedObjectSize();
        //binding for the ubo 
        VkDescriptorSetLayoutBinding uboLayout;
        uboLayout.binding = 0;
        uboLayout.descriptorType = descriptorType;
        uboLayout.descriptorCount = 1;
        uboLayout.stageFlags = stageFlags;
        uboLayout.pImmutableSamplers = nullptr;

        //second binding will be texture to sample 

        //layout from all the bindings created
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &uboLayout;

        if (vkCreateDescriptorSetLayout(device.getDevice(), &layoutInfo, nullptr, &layout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create descriptor set layout!");

        VkDescriptorPoolSize poolSize{};
        poolSize.type = descriptorType;
        poolSize.descriptorCount = 1;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = 1;

        if (vkCreateDescriptorPool(device.getDevice(), &poolInfo, nullptr, &pool) != VK_SUCCESS)
            throw std::runtime_error("Failed to create descriptor pool!");

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = pool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &layout;

        if (vkAllocateDescriptorSets(device.getDevice(), &allocInfo, &descriptorSet) != VK_SUCCESS)
            throw std::runtime_error("Failed to allocate descriptor set!");

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffer.getBuffer();
        bufferInfo.offset = 0;
        bufferInfo.range = unalignedObjectSize; // covers all UBOs in the buffer

        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSet;                 // which descriptor set to update
        descriptorWrite.dstBinding = 0;                         // binding number (matches shader)
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = descriptorType; // ⚠ must match layout
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;              // points to your buffer
        descriptorWrite.pImageInfo = nullptr;                   // only used for samplers/images
        descriptorWrite.pTexelBufferView = nullptr;
        vkUpdateDescriptorSets(device.getDevice(), 1, &descriptorWrite, 0, nullptr);
        
    }
    ~VulkanDescriptor() {
        destroy();
    }

    void destroy(){
        if (pool != VK_NULL_HANDLE){
            vkDestroyDescriptorPool(device.getDevice(), pool, nullptr);
            pool = VK_NULL_HANDLE;
        }
        if (layout != VK_NULL_HANDLE){
            vkDestroyDescriptorSetLayout(device.getDevice(), layout, nullptr);
            layout = VK_NULL_HANDLE;
        }
    }
    const VkDescriptorSet& getDescriptorSet() const { return descriptorSet; }

};
