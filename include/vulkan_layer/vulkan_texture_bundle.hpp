#pragma once

#include "vulkan_device.hpp"
#include <vulkan/vulkan.h>

#include "vulkan_descriptor_data.hpp"
#include "vulkan_object_creation_utils.hpp"
#include <algorithm>
#include <array>

// ================= CONFIG =================
#define N_MIPMAPS 7
// ==========================================

class VulkanTextureBundle {
    private:
    VulkanDevice& device;
    int atlasSize;

    // -------- Per-mipmap texture storage --------
    // [mip][texture]
    std::array<std::vector<VkImage>, N_MIPMAPS> textureImages;
    std::array<std::vector<VkDeviceMemory>, N_MIPMAPS> textureImageMemories;
    std::array<std::vector<VkImageView>, N_MIPMAPS> textureImageViews;
    std::array<std::vector<VkOffset3D>, N_MIPMAPS> textureAtlasCoords;
    std::array<std::vector<VkExtent3D>, N_MIPMAPS> textureSizes;

    // -------- Per-mipmap atlas --------
    std::array<VkImage, N_MIPMAPS> atlasImages{};
    std::array<VkDeviceMemory, N_MIPMAPS> atlasImageMemories{};
    std::array<VkImageView, N_MIPMAPS> atlasImageViews{};
    std::array<VkSampler, N_MIPMAPS> atlasSamplers{};

    // -------- Descriptor --------
    VkDescriptorSetLayout textureDescLayout{ VK_NULL_HANDLE };
    VkDescriptorPool textureDescPool{ VK_NULL_HANDLE };
    VkDescriptorSet textureDescSet{ VK_NULL_HANDLE };

    public:
    VulkanTextureBundle (VulkanDevice& deviceRef, int atlasSize)
    : device (deviceRef), atlasSize (atlasSize) {

        textureDescLayout =
        createDescriptorLayout (device, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                VK_SHADER_STAGE_FRAGMENT_BIT, 0, N_MIPMAPS);

        textureDescPool =
        createDescriptorPool (device, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, N_MIPMAPS);

        textureDescSet =
        allocateDescriptorSet (device, textureDescLayout, textureDescPool,
                               "Texture Bundle Descriptor Set");

        // Create one atlas per mipmap level
        for (uint32_t mip = 0; mip < N_MIPMAPS; ++mip) {
            uint32_t mipAtlasSize = atlasSize >> mip;

            atlasImages[mip] =
            createImage (device, { mipAtlasSize, mipAtlasSize }, DEFAULT_TEXTURE_COLOR_FORMAT,
                         VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                         "Texture Atlas Image Mip " + std::to_string (mip));

            atlasImageMemories[mip] =
            allocateAndBindImageMemory (device, atlasImages[mip]);

            transitionImageLayout (device, atlasImages[mip], DEFAULT_TEXTURE_COLOR_FORMAT,
                                   VK_IMAGE_LAYOUT_UNDEFINED,
                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            atlasImageViews[mip] =
            createImageView (device, atlasImages[mip], DEFAULT_TEXTURE_COLOR_FORMAT,
                             VK_IMAGE_ASPECT_COLOR_BIT,
                             "Texture Atlas Image View Mip " + std::to_string (mip));

            atlasSamplers[mip] =
            createSampler (device, "Texture Atlas Sampler Mip " + std::to_string (mip));
        }
    }

    ~VulkanTextureBundle () {
        destroy ();
    }

    void destroy () {
        for (uint32_t mip = 0; mip < N_MIPMAPS; ++mip) {
            for (size_t i = 0; i < textureImages[mip].size (); ++i) {
                if (textureImageViews[mip][i] != VK_NULL_HANDLE)
                    vkDestroyImageView (device.getDevice (),
                                        textureImageViews[mip][i], nullptr);
                if (textureImages[mip][i] != VK_NULL_HANDLE)
                    vkDestroyImage (device.getDevice (), textureImages[mip][i], nullptr);
                if (textureImageMemories[mip][i] != VK_NULL_HANDLE)
                    vkFreeMemory (device.getDevice (), textureImageMemories[mip][i], nullptr);
            }

            textureImages[mip].clear ();
            textureImageMemories[mip].clear ();
            textureImageViews[mip].clear ();
            textureAtlasCoords[mip].clear ();
            textureSizes[mip].clear ();

            if (atlasSamplers[mip] != VK_NULL_HANDLE)
                vkDestroySampler (device.getDevice (), atlasSamplers[mip], nullptr);
            if (atlasImageViews[mip] != VK_NULL_HANDLE)
                vkDestroyImageView (device.getDevice (), atlasImageViews[mip], nullptr);
            if (atlasImages[mip] != VK_NULL_HANDLE)
                vkDestroyImage (device.getDevice (), atlasImages[mip], nullptr);
            if (atlasImageMemories[mip] != VK_NULL_HANDLE)
                vkFreeMemory (device.getDevice (), atlasImageMemories[mip], nullptr);
        }

        if (textureDescLayout != VK_NULL_HANDLE)
            vkDestroyDescriptorSetLayout (device.getDevice (), textureDescLayout, nullptr);
        if (textureDescPool != VK_NULL_HANDLE)
            vkDestroyDescriptorPool (device.getDevice (), textureDescPool, nullptr);

        textureDescLayout = VK_NULL_HANDLE;
        textureDescPool   = VK_NULL_HANDLE;
        textureDescSet    = VK_NULL_HANDLE;
    }

    VkExtent3D roundMaxDimToPowerOfTwo (VkExtent3D srcTexSize) {
        VkExtent3D texSize = srcTexSize;
        if (texSize.width >= texSize.height) {
  
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

    // ------------------------------------------------------------
    // Texture loading: creates N_MIPMAPS textures per input
    // ------------------------------------------------------------
    uint32_t addTexture (const std::string& filename) {
    uint32_t index = textureImages[0].size ();

    std::vector<VkImage>        mipImages;
    std::vector<VkDeviceMemory> mipMemories;
    std::vector<int>            mipWidths;
    std::vector<int>            mipHeights;

    // 1. Load image + generate mipmaps (original resolution)
    createImageAndMipmapsFromFile(
        device,
        filename,
        DEFAULT_TEXTURE_COLOR_FORMAT,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_SAMPLED_BIT,
        mipImages,
        mipMemories,
        N_MIPMAPS,
        "Texture " + std::to_string(index),
        mipWidths,
        mipHeights,
        ATLAS_PADDING
    );

    // 2. For each mip, downscale to nearest power-of-two
    for (uint32_t mip = 0; mip < N_MIPMAPS; ++mip) {
        VkExtent3D srcSize {
            (uint32_t)mipWidths[mip],
            (uint32_t)mipHeights[mip],
            1
        };

        VkExtent3D dstSize = roundMaxDimToPowerOfTwo(srcSize);

        VkDeviceMemory downsizedMemory;
        VkImage downsizedImage = blitDownsizedImage(
            device,
            mipImages[mip],
            DEFAULT_TEXTURE_COLOR_FORMAT,
            srcSize.width,
            srcSize.height,
            dstSize.width,
            dstSize.height,
            downsizedMemory,
            ("Texture " + std::to_string(index) +
             " mip " + std::to_string(mip) + " POT").c_str()
        );

        // Store only the downsized image
        textureImages[mip].push_back(downsizedImage);
        textureImageMemories[mip].push_back(downsizedMemory);
        textureImageViews[mip].push_back(
            createImageView(
                device,
                downsizedImage,
                DEFAULT_TEXTURE_COLOR_FORMAT,
                VK_IMAGE_ASPECT_COLOR_BIT,
                "Texture View mip " + std::to_string(mip)
            )
        );
        textureSizes[mip].push_back(dstSize);
        textureAtlasCoords[mip].push_back({0,0,0});

        // Cleanup original mip image
        vkDestroyImage(device.getDevice(), mipImages[mip], nullptr);
        vkFreeMemory(device.getDevice(), mipMemories[mip], nullptr);
    }

    return index;
}

    
    void pasteTextureOnAtlas (uint32_t textureIndex, VkOffset3D offset) {
        
        if (textureIndex >= textureImages[0].size ()) {
            return;
        }
       
        for(int mip = 0; mip < N_MIPMAPS; mip++){
            VkOffset3D atlasOffset = {(int)(offset.x/pow(2, mip)), (int)(offset.y/pow(2, mip)), 0};
            copyImage (device, textureImages[mip][textureIndex], atlasImages[mip],
                    textureSizes[mip][textureIndex], atlasOffset);
            Debug::Log ("Pasted texture " + std::to_string (textureIndex) +
                        " at atlas offset (" + std::to_string (atlasOffset.x) +
                        ", " + std::to_string (atlasOffset.y) + ")" + "with size (" +
                        std::to_string (textureSizes[mip][textureIndex].width) + ", " +
                        std::to_string (textureSizes[mip][textureIndex].height) + ")");
            textureAtlasCoords[mip][textureIndex] = atlasOffset;
        }
        
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

        for(int i = 0; i < N_MIPMAPS; i++){
            textureAtlasCoords[i].resize (textureImages[i].size ());
        }
        std::vector<std::vector<int>> textureIndicesPerPowersOfTwo (
        ceiledLog2 (atlasSize)); // up to 2^15 = 32768 size textures
        for (uint32_t i = 0; i < textureImages[0].size (); i++) {
            VkExtent3D size = textureSizes[0][i];
            int maxDim      = std::max (size.width, size.height);
            int power       = ceiledLog2 (maxDim);

            if (power < textureIndicesPerPowersOfTwo.size ()) {

                textureIndicesPerPowersOfTwo[power].push_back (i);
            }
        }
        int endP, endI;
        fillCell ({ 0, 0, 0 }, ceiledLog2 (atlasSize)-1, textureIndicesPerPowersOfTwo,
                  ceiledLog2 (atlasSize) - 1, 0, endP, endI);
        for (uint32_t mip = 0; mip < N_MIPMAPS; ++mip) {
            writeImageSamplerInDescriptorSetArray (device, atlasImageViews[mip],
                                                   atlasSamplers[mip],
                                                   textureDescSet, 0, mip);
        }
    }

    glm::vec2 getTextureAtlasOffset (int textureIndex) {
        if (textureIndex >= textureAtlasCoords[0].size ()) {
            return glm::vec2 (0.0f, 0.0f);
        }
        VkOffset3D offset = textureAtlasCoords[0][textureIndex];
        return glm::vec2 ((float)(offset.x + ATLAS_PADDING) / (float)atlasSize,
                          (float)(offset.y + ATLAS_PADDING) / (float)atlasSize);
    }
    glm::vec2 getTextureSize (int textureIndex) {
        if (textureIndex >= textureSizes[0].size ()) {
            return glm::vec2 (0.0f, 0.0f);
        }
        VkExtent3D size = textureSizes[0][textureIndex];
        return glm::vec2 ((float)(size.width - 2 * ATLAS_PADDING) / (float)atlasSize,
                          (float)(size.height - 2 * ATLAS_PADDING) / (float)atlasSize);
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

    private:
    int ceiledLog2 (int v) const {
        return (int)std::ceil (std::log2 ((float)v));
    }

};
