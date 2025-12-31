#pragma once

#include <vulkan/vulkan.h>
#include <array>
#include <string>
#include <vector>

#include "vulkan_device.hpp"
#include <glm/glm.hpp>

class VulkanDescriptorData;

// ================= CONFIG =================
#define N_MIPMAPS 10
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
  VkDescriptorSetLayout textureDescLayout{VK_NULL_HANDLE};
  VkDescriptorPool textureDescPool{VK_NULL_HANDLE};
  VkDescriptorSet textureDescSet{VK_NULL_HANDLE};

 public:
  VulkanTextureBundle(VulkanDevice& deviceRef, int atlasSize);

  ~VulkanTextureBundle();

  void destroy();

  VkExtent3D roundMaxDimToPowerOfTwo(VkExtent3D srcTexSize);

  // ------------------------------------------------------------
  // Texture loading: creates N_MIPMAPS textures per input
  // ------------------------------------------------------------
  uint32_t addTexture(const std::string& filename);

  void pasteTextureOnAtlas(uint32_t textureIndex, VkOffset3D offset);

  bool isEndOfTextures(int power, int index,
                       const std::vector<std::vector<int>>&
                           textureIndicesPerPowersOfTwo);

  void fillCell(
      VkOffset3D topLeftOffset, int cellPowerSize,
      const std::vector<std::vector<int>>& textureIndicesPerPowersOfTwo,

      int startPower, int startIndex, int& endPower, int& endIndex);

  int ceiledLog2(int value);
  void buildTextureAtlas();

  glm::vec2 getTextureAtlasOffset(int textureIndex);
  glm::vec2 getRelativeTextureSize(int textureIndex);
  const VulkanDescriptorData getDescData(int binding, int set) const;

 private:
  int ceiledLog2(int v) const;
};