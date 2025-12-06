#pragma once 

#include <vulkan/vulkan.h>
#include "vulkan_device.hpp"

#include "vulkan_object_creation_utils.hpp"
#include "vulkan_descriptor_data.hpp"
#include <algorithm>

class VulkanTextureBundle {
private:
    /*VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;*/

    std::vector<VkImage> textureImages;
    std::vector<VkDeviceMemory> textureImageMemories;
    std::vector<VkImageView> textureImageViews;
    std::vector<VkSampler> textureSamplers;
    std::vector<bool> slotOccupied; // 0 = free, 1 = occupied


    VulkanDevice& device;
    uint32_t capacity = 0;

    VkDescriptorSetLayout textureDescLayout;
    VkDescriptorPool textureDescPool;
    VkDescriptorSet textureDescSet;

    // Resize descriptor set/pool/layout to a new capacity. Recreates layout/pool/set and
    // re-writes existing occupied descriptors into the new set. Throws on failure.
    void resizeCapacity(uint32_t newCapacity) {
        if (newCapacity <= capacity) return;

        // clamp to device limits
        const auto& limits = device.getProperties().limits;
        uint32_t samplerLimit = static_cast<uint32_t>(std::min(limits.maxPerStageDescriptorSamplers,
                                                               limits.maxDescriptorSetSamplers));
        if (newCapacity > samplerLimit) newCapacity = samplerLimit;

        // create new layout
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = 0;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding.descriptorCount = newCapacity;
        binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        binding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &binding;
        VkDescriptorSetLayout newLayout = VK_NULL_HANDLE;
        if (vkCreateDescriptorSetLayout(device.getDevice(), &layoutInfo, nullptr, &newLayout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create resized descriptor set layout");

        // create new pool
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = newCapacity;

        VkDescriptorPoolCreateInfo poolInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = 1;
        VkDescriptorPool newPool = VK_NULL_HANDLE;
        if (vkCreateDescriptorPool(device.getDevice(), &poolInfo, nullptr, &newPool) != VK_SUCCESS) {
            vkDestroyDescriptorSetLayout(device.getDevice(), newLayout, nullptr);
            throw std::runtime_error("Failed to create resized descriptor pool");
        }

        // allocate new set
        VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocInfo.descriptorPool = newPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &newLayout;
        VkDescriptorSet newSet = VK_NULL_HANDLE;
        if (vkAllocateDescriptorSets(device.getDevice(), &allocInfo, &newSet) != VK_SUCCESS) {
            vkDestroyDescriptorPool(device.getDevice(), newPool, nullptr);
            vkDestroyDescriptorSetLayout(device.getDevice(), newLayout, nullptr);
            throw std::runtime_error("Failed to allocate resized descriptor set");
        }

        // write existing occupied entries into new set (per-slot writes)
        for (uint32_t i = 0; i < capacity; ++i) {
            if (!slotOccupied[i]) continue;
            VkDescriptorImageInfo imageInfo{};
            imageInfo.sampler = textureSamplers[i];
            imageInfo.imageView = textureImageViews[i];
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkWriteDescriptorSet write{};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = newSet;
            write.dstBinding = 0;
            write.dstArrayElement = i;
            write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.descriptorCount = 1;
            write.pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(device.getDevice(), 1, &write, 0, nullptr);
        }

        // swap in new resources, destroy old ones
        VkDescriptorSetLayout oldLayout = textureDescLayout;
        VkDescriptorPool oldPool = textureDescPool;
        VkDescriptorSet oldSet = textureDescSet;

        textureDescLayout = newLayout;
        textureDescPool = newPool;
        textureDescSet = newSet;
        capacity = newCapacity;

        if (oldSet != VK_NULL_HANDLE) {
            // oldSet will be implicitly freed when pool is destroyed; destroy old pool and layout
            if (oldPool != VK_NULL_HANDLE) vkDestroyDescriptorPool(device.getDevice(), oldPool, nullptr);
            if (oldLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device.getDevice(), oldLayout, nullptr);
        }
        // resize internal vectors to newCapacity, preserving existing entries
        textureImages.resize(capacity, VK_NULL_HANDLE);
        textureImageMemories.resize(capacity, VK_NULL_HANDLE);
        textureImageViews.resize(capacity, VK_NULL_HANDLE);
        textureSamplers.resize(capacity, VK_NULL_HANDLE);
        slotOccupied.resize(capacity, 0);
    }

public:
    VulkanTextureBundle(VulkanDevice& deviceRef, const std::vector<std::string>& filenames, uint32_t preallocate = 8)
    : device(deviceRef) {
        // determine capacity (at least initial count)
        uint32_t initialCount = static_cast<uint32_t>(filenames.size());
        uint32_t requested = preallocate < 1 ? 1 : preallocate;

        // clamp requested capacity to device limits (sampler-related limits)
        const auto& limits = device.getProperties().limits;
        uint32_t samplerLimit = static_cast<uint32_t>(std::min(limits.maxPerStageDescriptorSamplers,
                                                               limits.maxDescriptorSetSamplers));
        if (samplerLimit == 0) {
            throw std::runtime_error("Device reports zero sampler descriptor limit");
        }

        if (requested > samplerLimit) {
            requested = samplerLimit;
        }

        capacity = requested;
        if (capacity < initialCount) {
            throw std::runtime_error("Initial texture count exceeds device sampler limits");
        }

        // allocate storage up to capacity and mark slots free
        textureImages.resize(capacity, VK_NULL_HANDLE);
        textureImageMemories.resize(capacity, VK_NULL_HANDLE);
        textureImageViews.resize(capacity, VK_NULL_HANDLE);
        textureSamplers.resize(capacity, VK_NULL_HANDLE);
        slotOccupied.resize(capacity, 0);

        // load initial textures into the first slots and mark them occupied
        for (size_t i = 0; i < filenames.size(); ++i) {
            VkDeviceMemory mem = VK_NULL_HANDLE;
            VkImage img = createImageFromFile(device, filenames[i], VK_FORMAT_R8G8B8A8_SRGB,
                                              VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                              mem, "Texture Image " + std::to_string(i));

            VkImageView view = createImageView(device, img, VK_FORMAT_R8G8B8A8_SRGB,
                                               VK_IMAGE_ASPECT_COLOR_BIT, "Texture Image View " + std::to_string(i));
            VkSampler samp = createSampler(device);

            textureImages[i] = img;
            textureImageMemories[i] = mem;
            textureImageViews[i] = view;
            textureSamplers[i] = samp;
            slotOccupied[i] = 1;
        }

        // create descriptor set layout with an array binding sized to capacity
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = 0;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding.descriptorCount = capacity; // array size / capacity
        binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        binding.pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo layoutInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &binding;
        if (vkCreateDescriptorSetLayout(device.getDevice(), &layoutInfo, nullptr, &textureDescLayout) != VK_SUCCESS)
            throw std::runtime_error("Failed to create texture descriptor set layout");

        // create pool capable of holding 'capacity' descriptors and 1 set
        textureDescPool = createDescriptorPool(device, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, capacity);    
        // allocate descriptor set
        textureDescSet = allocateDescriptorSet(device, textureDescLayout, textureDescPool,
                                               "Texture Descriptor Set");
        // initialize descriptor with the currently loaded textures (if any)
        // write only the initial loaded range (occupied slots at the start)
        uint32_t initialLoaded = 0;
        for (uint32_t i = 0; i < capacity; ++i) if (slotOccupied[i]) ++initialLoaded;
        if (initialLoaded > 0) {
            // build small vectors with only the loaded entries in ascending order
            std::vector<VkImageView> iv(initialLoaded);
            std::vector<VkSampler> sm(initialLoaded);
            for (uint32_t i = 0, j = 0; i < capacity; ++i) if (slotOccupied[i]) { iv[j] = textureImageViews[i]; sm[j] = textureSamplers[i]; ++j; }
            writeImageSamplersInDescriptorSet(device, iv, sm, textureDescSet);
        }
    }

    ~VulkanTextureBundle() {
        destroy();
    }

    void destroy() {
        //check and destroy all texture resources
        for(size_t i=0; i<textureImages.size(); i++){
            if (textureImageViews[i] != VK_NULL_HANDLE)
                vkDestroyImageView(device.getDevice(), textureImageViews[i], nullptr);
            if (textureImages[i] != VK_NULL_HANDLE)
                vkDestroyImage(device.getDevice(), textureImages[i], nullptr);
            if (textureImageMemories[i] != VK_NULL_HANDLE)
                vkFreeMemory(device.getDevice(), textureImageMemories[i], nullptr);
            if (textureSamplers[i] != VK_NULL_HANDLE)
                vkDestroySampler(device.getDevice(), textureSamplers[i], nullptr);
            
            //set to null handles
            textureImageViews[i] = VK_NULL_HANDLE;
            textureImages[i] = VK_NULL_HANDLE;
            textureImageMemories[i] = VK_NULL_HANDLE;
            textureSamplers[i] = VK_NULL_HANDLE;
        }
        if (textureDescLayout != VK_NULL_HANDLE)
            vkDestroyDescriptorSetLayout(device.getDevice(), textureDescLayout, nullptr);
        if (textureDescPool != VK_NULL_HANDLE)
            vkDestroyDescriptorPool(device.getDevice(), textureDescPool, nullptr);
        textureImages.clear();
        textureImageMemories.clear();
        textureImageViews.clear();
        textureSamplers.clear();

        //set to null handles
        textureDescLayout = VK_NULL_HANDLE;
        textureDescPool = VK_NULL_HANDLE;
        textureDescSet = VK_NULL_HANDLE;
        
    }

    const VulkanDescriptorData getDescData(int binding, int set) const{
        return {textureDescSet, textureDescLayout, textureDescPool, false, 0, binding, set};
    }

    // Add a new texture at runtime into the preallocated descriptor array.
    // Returns the index assigned to the texture, or UINT32_MAX on failure (capacity exhausted).
    uint32_t addTexture(const std::string& filename) {
        // find first free slot
        uint32_t index = UINT32_MAX;
        for (uint32_t i = 0; i < capacity; ++i) {
            if (!slotOccupied[i]) { index = i; break; }
        }

        // if no free slot, attempt to grow capacity (double up to device limits)
        if (index == UINT32_MAX) {
            const auto& limits = device.getProperties().limits;
            uint32_t samplerLimit = static_cast<uint32_t>(std::min(limits.maxPerStageDescriptorSamplers,
                                                                   limits.maxDescriptorSetSamplers));
            uint32_t newCapacity = capacity > 0 ? std::min(samplerLimit, capacity * 2u) : 1u;
            if (newCapacity <= capacity || newCapacity == 0) {
                return UINT32_MAX; // cannot grow further
            }
            resizeCapacity(newCapacity);

            // find free slot again after resize
            for (uint32_t i = 0; i < capacity; ++i) {
                if (!slotOccupied[i]) { index = i; break; }
            }
            if (index == UINT32_MAX) return UINT32_MAX; // still no slot
        }

        // create resources
        VkDeviceMemory mem = VK_NULL_HANDLE;
        VkImage img = createImageFromFile(device, filename, VK_FORMAT_R8G8B8A8_SRGB,
                                          VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                          mem, "Texture Image " + std::to_string(index));
        VkImageView view = createImageView(device, img, VK_FORMAT_R8G8B8A8_SRGB,
                                           VK_IMAGE_ASPECT_COLOR_BIT, "Texture Image View " + std::to_string(index));
        VkSampler samp = createSampler(device);

        // store into slot
        textureImages[index] = img;
        textureImageMemories[index] = mem;
        textureImageViews[index] = view;
        textureSamplers[index] = samp;
        slotOccupied[index] = 1;

        // update descriptor set for this single array element
        VkDescriptorImageInfo imageInfo{};
        imageInfo.sampler = samp;
        imageInfo.imageView = view;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = textureDescSet;
        write.dstBinding = 0;
        write.dstArrayElement = index;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.descriptorCount = 1;
        write.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device.getDevice(), 1, &write, 0, nullptr);

        return index;
    }
};



