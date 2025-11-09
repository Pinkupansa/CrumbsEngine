#pragma once
#include <vulkan/vulkan.h>
#include "vulkan_instance.hpp"
#include <iostream>
#include "vulkan_instance.hpp"
class VulkanDevice {

private: 
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceProperties properties; 
    VkDevice device;
    VkQueue graphicsQueue;
    uint32_t graphicsFamilyIndex;
    
    VkCommandPool commandPool;

    PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = nullptr;
    PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT = nullptr;
    PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT = nullptr;
    PFN_vkCmdInsertDebugUtilsLabelEXT vkCmdInsertDebugUtilsLabelEXT = nullptr;

public:
    
    const VkDevice& getDevice() const{
        return device;
    }

    const VkPhysicalDevice& getPhysicalDevice() const{
        return physicalDevice;
    }

    const VkPhysicalDeviceProperties& getProperties() const{ 
        return properties;
    }
    const VkCommandPool& getCommandPool() const{
        return commandPool;
    }

    const VkQueue& getGraphicsQueue() const{
        return graphicsQueue;
    }
    
    VulkanDevice(VulkanInstance& instance){
        VkSurfaceKHR surface = instance.getSurface();
        //First call to get the device Count
        uint32_t deviceCount = 0; 
        vkEnumeratePhysicalDevices(instance.getInstance(), &deviceCount, nullptr);
        if(deviceCount == 0){
            throw std::runtime_error("No GPUs with Vulkan support found!");
        }
        //Second call after creating a device array of the correct size
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance.getInstance(), &deviceCount, devices.data());


        //Print available devices 
        std::cout << "Available Vulkan devices:\n";
        for (const auto& device : devices) {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(device, &props);
            std::cout << " - " << props.deviceName << "\n";
        }

        for(const auto& pdevice: devices){ 
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(pdevice, &props);
            //Pick the first discrete GPU
            if(props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU){
                std::cout << "-> Selected discrete GPU: " << props.deviceName << "\n";
                physicalDevice = pdevice;
            }
        }

        physicalDevice = devices[0];
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        //find a queueIndex
        graphicsFamilyIndex = -1;
        uint32_t queueFamilyCount = 0; 
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> families(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, families.data());

        for(uint32_t i = 0; i < queueFamilyCount; ++i){
            if(families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT){
                std::cout << "Found graphics queue family: " << i << "\n";
                graphicsFamilyIndex = i;
                break;
            }
        }
        if (graphicsFamilyIndex == -1){
            throw std::runtime_error("Couldn't find graphics queue family !");
        }

        float queuePriority = 1.0f; 

        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = graphicsFamilyIndex; // from Step 4
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,   
            "VK_KHR_portability_subset"        // required on macOS
        };

        VkDeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = 1;
        deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create logical device!");
        }
        
        vkSetDebugUtilsObjectNameEXT =(PFN_vkSetDebugUtilsObjectNameEXT)(vkGetDeviceProcAddr(device, "vkSetDebugUtilsObjectNameEXT"));

        vkCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)(vkGetDeviceProcAddr(device, "vkCmdBeginDebugUtilsLabelEXT"));

        vkCmdEndDebugUtilsLabelEXT = (PFN_vkCmdEndDebugUtilsLabelEXT)(vkGetDeviceProcAddr(device, "vkCmdEndDebugUtilsLabelEXT"));

        vkCmdInsertDebugUtilsLabelEXT =(PFN_vkCmdInsertDebugUtilsLabelEXT)(vkGetDeviceProcAddr(device, "vkCmdInsertDebugUtilsLabelEXT"));

        vkGetDeviceQueue(device, graphicsFamilyIndex, 0, &graphicsQueue);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = graphicsFamilyIndex; // your graphics queue
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // optional, could use VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT

        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create command pool!");
        }   
        
    }

    ~VulkanDevice(){
        destroy();
    }

    void destroy() {
        if (commandPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(device, commandPool, nullptr);
            commandPool = VK_NULL_HANDLE;
        }
        if (device != VK_NULL_HANDLE) {
            vkDestroyDevice(device, nullptr);
            device = VK_NULL_HANDLE;
        }
    }

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties){
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void nameObject(uint64_t vulkanObject, VkObjectType type, std::string name){
        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = type; // example
        nameInfo.objectHandle = vulkanObject;
        nameInfo.pObjectName = name.c_str();
        vkSetDebugUtilsObjectNameEXT(device, &nameInfo);
    }
};
