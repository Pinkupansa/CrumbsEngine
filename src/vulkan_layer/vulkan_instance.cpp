#include "vulkan_layer/vulkan_instance.hpp"

#include "engine_layer/debug.hpp"
#include <iostream>

VkApplicationInfo defaultAppInfo() {
  VkApplicationInfo appInfo{};

  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "MyEngine";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_3;
  return appInfo;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanInstance::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
  std::string prefix;
  if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    prefix = "[ERROR] ";
  else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    prefix = "[WARN ] ";
  else
    prefix = "[INFO ] ";

  std::cerr << prefix << pCallbackData->pMessage << std::endl;
  return VK_FALSE;
}

void VulkanInstance::setupDebugMessenger() {
  // Create Info for the debugger
  VkDebugUtilsMessengerCreateInfoEXT createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback =
      debugCallback;  // setup the function called when message is sent
  createInfo.pUserData = nullptr;

  // look up for the creation function in the extension. If it exists, call it.
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  if (func && func(instance, &createInfo, nullptr, &debugMessenger) ==
                  VK_SUCCESS) {
    std::cout << "✅ Debug messenger created.\n";
  } else {
    std::cerr << "⚠️ Failed to create debug messenger.\n";
  }
}

VulkanInstance::VulkanInstance() {
  VkApplicationInfo appInfo = defaultAppInfo();
  uint32_t glfwExtensionCount = 0;
  const char**glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  const char* validationLayers[] = {"VK_LAYER_KHRONOS_validation"};  // extension for API errors
  std::vector<const char*> extensions;
  for (int i = 0; i < glfwExtensionCount; i++) {
    extensions.push_back(glfwExtensions[i]);
  }
  // For macOS (MoltenVK)
  extensions.push_back("VK_KHR_portability_enumeration");
  extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  // Create info for the VKInstance
  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledExtensionCount = extensions.size();
  createInfo.ppEnabledExtensionNames = extensions.data();
  createInfo.enabledLayerCount = 1;
  createInfo.ppEnabledLayerNames = validationLayers;

  // macOS-specific: required flag for MoltenVK
  createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

  if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create Vulkan instance!");
  }

  setupDebugMessenger();
}

VulkanInstance::~VulkanInstance() { destroy(); }

void VulkanInstance::destroy() {
  if (debugMessenger != VK_NULL_HANDLE) {
    auto destroyFunc = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
        instance, "vkDestroyDebugUtilsMessengerEXT");
    if (destroyFunc) destroyFunc(instance, debugMessenger, nullptr);
  }
  if (instance != VK_NULL_HANDLE) {
    vkDestroyInstance(instance, nullptr);
    instance = VK_NULL_HANDLE;
  }
}

const VkInstance& VulkanInstance::getInstance() const { return instance; }
