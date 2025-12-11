#pragma once

#include "vulkan_device.hpp"
#include <vulkan/vulkan.h>

#include "vulkan_descriptor_data.hpp"
#include "vulkan_object_creation_utils.hpp"
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


    std::vector<VkOffset3D> textureAtlasCoords;
    std::vector<VkExtent3D> textureSizes;

    VulkanDevice& device;

    VkDescriptorSetLayout textureDescLayout;
    VkDescriptorPool textureDescPool;
    VkDescriptorSet textureDescSet;

    VkImage textureAtlasImage;
    VkDeviceMemory textureAtlasImageMemory;
    VkImageView textureAtlasImageView;
    VkSampler textureAtlasSampler;

    int atlasSize;

    public:
    VulkanTextureBundle (VulkanDevice& deviceRef, int atlasSize)
    : device (deviceRef), atlasSize (atlasSize) {

        textureDescLayout =
        createDescriptorLayout (device, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                VK_SHADER_STAGE_FRAGMENT_BIT, 0);
        textureDescPool =
        createDescriptorPool (device, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        textureDescSet =
        allocateDescriptorSet (device, textureDescLayout, textureDescPool,
                               "Texture Bundle Descriptor Set");


        textureAtlasImage =
        createImage (device, { (uint32_t)atlasSize, (uint32_t)atlasSize },
                     DEFAULT_TEXTURE_COLOR_FORMAT,
                     VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                     "Texture Atlas Image");
        textureAtlasImageMemory = allocateAndBindImageMemory (device, textureAtlasImage);
        transitionImageLayout (device, textureAtlasImage,
                               DEFAULT_TEXTURE_COLOR_FORMAT, VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        textureAtlasImageView =
        createImageView (device, textureAtlasImage, DEFAULT_TEXTURE_COLOR_FORMAT,
                         VK_IMAGE_ASPECT_COLOR_BIT, "Texture Atlas Image View");

        textureAtlasSampler = createSampler (device, "Texture Atlas Sampler");
    }


    ~VulkanTextureBundle () {
        destroy ();
    }

    void destroy () {
        // check and destroy all texture resources
        for (size_t i = 0; i < textureImages.size (); i++) {
            if (textureImageViews[i] != VK_NULL_HANDLE)
                vkDestroyImageView (device.getDevice (), textureImageViews[i], nullptr);
            if (textureImages[i] != VK_NULL_HANDLE)
                vkDestroyImage (device.getDevice (), textureImages[i], nullptr);
            if (textureImageMemories[i] != VK_NULL_HANDLE)
                vkFreeMemory (device.getDevice (), textureImageMemories[i], nullptr);

            // set to null handles
            textureImageViews[i]    = VK_NULL_HANDLE;
            textureImages[i]        = VK_NULL_HANDLE;
            textureImageMemories[i] = VK_NULL_HANDLE;
        }
        if (textureDescLayout != VK_NULL_HANDLE)
            vkDestroyDescriptorSetLayout (device.getDevice (), textureDescLayout, nullptr);
        if (textureDescPool != VK_NULL_HANDLE)
            vkDestroyDescriptorPool (device.getDevice (), textureDescPool, nullptr);
        textureImages.clear ();
        textureImageMemories.clear ();
        textureImageViews.clear ();

        // set to null handles
        textureDescLayout = VK_NULL_HANDLE;
        textureDescPool   = VK_NULL_HANDLE;
        textureDescSet    = VK_NULL_HANDLE;
    }

    const VulkanDescriptorData getDescData (int binding, int set) const {
        return { textureDescSet,
                 textureDescLayout,
                 textureDescPool,
                 false,
                 0,
                 binding,
                 set };
    }

    // Add a new texture at runtime into the preallocated descriptor array.
    // Returns the index assigned to the texture, or UINT32_MAX on failure (capacity exhausted).


    VkExtent3D roundMaxDimToPowerOfTwo (VkExtent3D srcTexSize) {
        VkExtent3D texSize = srcTexSize;
        if (texSize.width >= texSize.height) {
            Debug::Log ("Width greater than height");
            if (texSize.width != 1 << ceiledLog2 (texSize.width)){
                texSize.height = (int)(texSize.height * 1 << (ceiledLog2 (texSize.width) - 1)) / (float)texSize.width;
                texSize.width = 1 << (ceiledLog2 (texSize.width) - 1);
            }
        }

        else if (texSize.height > texSize.width) {
            if (texSize.height != 1 << ceiledLog2 (texSize.height)) {
                texSize.width =
                (int)(texSize.width * 1 << (ceiledLog2 (texSize.height) - 1)) /
                (float)texSize.height;
                texSize.height = 1 << (ceiledLog2 (texSize.height) - 1);
            }
        }
        return texSize;
    }
    uint32_t addTexture (const std::string& filename) {
        int freeIndex = textureImages.size ();

        textureImages.push_back (VK_NULL_HANDLE);
        textureImageMemories.push_back (VK_NULL_HANDLE);
        textureImageViews.push_back (VK_NULL_HANDLE);
        textureSizes.push_back ({ 0, 0, 0 });

        VkDeviceMemory imageMemory;
        VkExtent3D srcTexSize;
        VkImage textureImage =
        createImageFromFile (device, filename, DEFAULT_TEXTURE_COLOR_FORMAT,
                             VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                             imageMemory, "Texture Image " + std::to_string (freeIndex),
                             (int&)srcTexSize.width, (int&)srcTexSize.height);

        srcTexSize.depth = 1;


        
        VkExtent3D texSize = roundMaxDimToPowerOfTwo (srcTexSize);
        Debug::Log ("Original size: (" + std::to_string (srcTexSize.width) + ", " +
                    std::to_string (srcTexSize.height) + "), downscaled to: (" +
                    std::to_string (texSize.width) + ", " +
                    std::to_string (texSize.height) + ")");

        textureImage = blitDownsizedImage (
        device, textureImage, DEFAULT_TEXTURE_COLOR_FORMAT, srcTexSize.width,
        srcTexSize.height, texSize.width, texSize.height, imageMemory,
        ("Blitted Downsize Texture Image " + std::to_string (freeIndex)).c_str ());
        /* VkImageView textureImageView =
         createImageView (device, textureImage, DEFAULT_TEXTURE_COLOR_FORMAT,
                          VK_IMAGE_ASPECT_COLOR_BIT,
                          "Texture Image View " + std::to_string (freeIndex));

         VkSampler textureSampler =
         createSampler (device, "Texture Sampler " + std::to_string (freeIndex));*/

        textureImages[freeIndex] = textureImage;
        // textureImageViews[freeIndex] = textureImageView;
        textureImageMemories[freeIndex] = imageMemory;
        textureSizes[freeIndex]         = texSize;

        return static_cast<uint32_t> (freeIndex);
    }


    void pasteTextureOnAtlas (uint32_t textureIndex, VkOffset3D atlasOffset) {
        if (textureIndex >= textureImages.size ()) {
            return;
        }

        copyImage (device, textureImages[textureIndex], textureAtlasImage,
                   textureSizes[textureIndex], atlasOffset);
        Debug::Log ("Pasted texture " + std::to_string (textureIndex) +
                    " at atlas offset (" + std::to_string (atlasOffset.x) +
                    ", " + std::to_string (atlasOffset.y) + ")" + "with size (" +
                    std::to_string (textureSizes[textureIndex].width) + ", " +
                    std::to_string (textureSizes[textureIndex].height) + ")");
        textureAtlasCoords[textureIndex] = atlasOffset;
    }

    bool isEndOfTextures (int power, int index, const std::vector<std::vector<int>>& textureIndicesPerPowersOfTwo) {
        return power <= 0 and index >= textureIndicesPerPowersOfTwo[0].size ();
    }
    void fillCell (VkOffset3D topLeftOffset,
                   int cellPowerSize,
                   const std::vector<std::vector<int>>& textureIndicesPerPowersOfTwo,

                   int startPower,
                   int startIndex,
                   int& endPower,
                   int& endIndex) {

        while (startPower >= 0 && textureIndicesPerPowersOfTwo[startPower].size () == 0) {
            startPower--;
            startIndex = 0;
        }
        if (startPower < 0) {
            endPower = -1;
            endIndex = -1;
            return;
        }
        if (cellPowerSize > startPower) {
            // fill 4 subcells
            int halfCellSize = cellPowerSize - 1;
            int endP, endI;
            fillCell ({ topLeftOffset.x, topLeftOffset.y, 0 }, halfCellSize,
                      textureIndicesPerPowersOfTwo, startPower, startIndex, endP, endI);
            if (endP == -1) {
                endPower = endP;
                endIndex = endI;
                return;
            }
            fillCell ({ topLeftOffset.x + (1 << halfCellSize), topLeftOffset.y, 0 },
                      halfCellSize, textureIndicesPerPowersOfTwo, endP, endI, endP, endI);

            if (endP == -1) {
                endPower = endP;
                endIndex = endI;
                return;
            }
            fillCell ({ topLeftOffset.x, topLeftOffset.y + (1 << halfCellSize), 0 },
                      halfCellSize, textureIndicesPerPowersOfTwo, endP, endI, endP, endI);
            if (endP == -1) {
                endPower = endP;
                endIndex = endI;
                return;
            }

            fillCell ({ topLeftOffset.x + (1 << halfCellSize),
                        topLeftOffset.y + (1 << halfCellSize), 0 },
                      halfCellSize, textureIndicesPerPowersOfTwo, endP, endI,
                      endPower, endIndex);

        } else {
            Debug::Log (std::to_string (startPower) + ", " + std::to_string (startIndex));
            pasteTextureOnAtlas (textureIndicesPerPowersOfTwo[startPower][startIndex],
                                 topLeftOffset);
            if (startIndex + 1 < textureIndicesPerPowersOfTwo[startPower].size ()) {
                endPower = startPower;
                endIndex = startIndex + 1;
            } else {
                endPower = startPower - 1;
                endIndex = 0;
            }
            return;
        }
    }

    int ceiledLog2 (int value) {
        return (int)std::ceil (std::log2 ((float)value));
    }
    void buildTextureAtlas () {

        textureAtlasCoords.resize (textureImages.size ());
        std::vector<std::vector<int>> textureIndicesPerPowersOfTwo (
        ceiledLog2 (atlasSize)); // up to 2^15 = 32768 size textures
        for (uint32_t i = 0; i < textureImages.size (); i++) {
            VkExtent3D size = textureSizes[i];
            int maxDim      = std::max (size.width, size.height);
            int power       = ceiledLog2 (maxDim);
            if (power < textureIndicesPerPowersOfTwo.size ()) {
                textureIndicesPerPowersOfTwo[power].push_back (i);
            }
        }
        int endP, endI;
        fillCell ({ 0, 0, 0 }, ceiledLog2 (atlasSize) - 1, textureIndicesPerPowersOfTwo,
                  ceiledLog2 (atlasSize) - 1, 0, endP, endI);
        writeImageSamplerInDescriptorSet (device, textureAtlasImageView,
                                          textureAtlasSampler, textureDescSet);
    }

    glm::vec2 getTextureAtlasOffset (int textureIndex) {
        if (textureIndex >= textureAtlasCoords.size ()) {
            return glm::vec2 (0.0f, 0.0f);
        }
        VkOffset3D offset = textureAtlasCoords[textureIndex];
        return glm::vec2 ((float)offset.x / (float)atlasSize,
                          (float)offset.y / (float)atlasSize);
    }

    glm::vec2 getTextureSize (int textureIndex) {
        if (textureIndex >= textureSizes.size ()) {
            return glm::vec2 (0.0f, 0.0f);
        }
        VkExtent3D size = textureSizes[textureIndex];
        return glm::vec2 ((float)size.width / atlasSize, (float)size.height / atlasSize);
    }
};
