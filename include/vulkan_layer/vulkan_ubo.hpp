#pragma once
#include <glm/glm.hpp>


struct VulkanUniformBufferObject {
    alignas(16) glm::mat4 model;                  // 16-byte aligned, occupies 64 byte
    bool castsShadows;
};
std::vector<uint8_t> padUBOData (std::vector<VulkanUniformBufferObject> ubos,
                                  VkDeviceSize alignedSize) {
        std::vector<uint8_t> paddedData (alignedSize * ubos.size (), 0); // zero-initialized

        for (size_t i = 0; i < ubos.size (); ++i) {
            std::memcpy (paddedData.data () + i * alignedSize, &ubos[i],
                         sizeof (VulkanUniformBufferObject));
        }
        return paddedData;
    }
