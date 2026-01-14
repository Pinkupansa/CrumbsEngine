#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

class ImGuiManager {
public:
    ImGuiManager(GLFWwindow* window, VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, uint32_t queueFamily, VkQueue queue, VkRenderPass renderPass, uint32_t minImageCount, uint32_t imageCount);
    ~ImGuiManager();

    void newFrame();
    void render();
    
    // Helper to check if ImGui is capturing inputs
    bool wantCaptureInput() const;

private:
    VkDescriptorPool m_descriptorPool;
    VkDevice m_device;
};
