#pragma once 
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include "vulkan_device.hpp"
#include "vulkan_instance.hpp"

class VulkanSurface{
    private:
        const VulkanInstance& pInstance; 

        VkSurfaceKHR surface; 
        VkSurfaceCapabilitiesKHR surfaceCapabilities;

    public: 
        const VkSurfaceKHR& getSurface() const{
            return surface;
        }

        const VkSurfaceCapabilitiesKHR& getCapabilities() const{
            return surfaceCapabilities;
        }
        
        VulkanSurface(const VulkanInstance& instance, const VulkanDevice& device, GLFWwindow* window): pInstance(instance){
            //Creation of the window tied to the instance
            if (glfwCreateWindowSurface(instance.getInstance(), window, nullptr, &surface) != VK_SUCCESS)
                throw std::runtime_error("Failed to create window surface!");
            
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.getPhysicalDevice(), surface, &surfaceCapabilities);
            device.nameObject((uint64_t)surface, VK_OBJECT_TYPE_SURFACE_KHR, "Main Surface");
        }
    
    ~VulkanSurface(){destroy();}
    void destroy(){
        if (surface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(pInstance.getInstance(), surface, nullptr);
            surface = VK_NULL_HANDLE;
        }
    }
};