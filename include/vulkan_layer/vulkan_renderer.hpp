#pragma once
#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include <functional>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>

#include "engine_layer/camera.hpp"
#include "engine_layer/mesh.hpp"
#include "engine_layer/scene.hpp"
#include "vulkan_layer/vulkan_buffer.hpp"
#include "vulkan_layer/vulkan_descriptor.hpp"
#include "vulkan_layer/vulkan_device.hpp"
#include "vulkan_layer/vulkan_flexible_shader_buffer.hpp"
#include "vulkan_layer/vulkan_image_drawer.hpp"
#include "vulkan_layer/vulkan_mesh_draw_info.hpp"
#include "vulkan_layer/vulkan_render_texture.hpp"
#include "vulkan_layer/vulkan_scene_ubo.hpp"
#include "vulkan_layer/vulkan_shader_data.hpp"
#include "vulkan_layer/vulkan_shadow_map.hpp"
#include "vulkan_layer/vulkan_swapchain.hpp"
#include "vulkan_layer/vulkan_texture_bundle.hpp"
#include "vulkan_layer/vulkan_ubo.hpp"
#include "vulkan_layer/vulkan_vertex.hpp"

struct ImDrawData;

class VulkanRenderer {
 private:
  // Shader paths
  std::string vertShaderPath = "./shaders/test.vert.spv";
  std::string fullScreenShaderPath = "./shaders/fullscreen.vert.spv";
  std::string fragShaderPath = "./shaders/test.frag.spv";
  std::string shadowShaderPath = "./shaders/shadows.vert.spv";

  std::vector<std::string> fragShaderPaths;
  // Window info
  GLFWwindow* window;
  bool isFirstFrame;
  // Core Vulkan objects
  VulkanInstance instance;
  VulkanDevice device;
  VulkanSurface mainSurface;

  VulkanSwapchain swapchain;

  VulkanTextureBundle textureBundle;

  // Vertex/index buffers
  VkDeviceSize vertexSize = sizeof(Vertex);
  VkDeviceSize indexSize = sizeof(uint32_t);

  VulkanBuffer vertexBuffer;
  VulkanBuffer indexBuffer;

  // Uniform buffers
  VkDeviceSize uboSize = sizeof(VulkanUniformBufferObject);
  VkDeviceSize alignment;
  VkDeviceSize uboAlignedSize;

  VulkanBuffer allObjectsUB;
  VulkanBuffer sceneDataUB;

  VulkanUBDescriptor allObjectsUBDescriptor;
  VulkanUBDescriptor sceneDataUBDescriptor;

  VulkanShadowMap shadowMap;
  VulkanImageDrawer shadowImageDrawer;

  // use heap because created when loading shader and hold references to
  // VkObjects that are destroyed if the object is destroyed
  std::vector<VulkanImageDrawer*> imageDrawersPerFragShader;
  std::vector<VulkanUBDescriptor*> objectsUBDescriptorPerFragShader;
  std::vector<std::vector<VulkanUniformBufferObject>> ubosPerFragShader;
  std::vector<VulkanBuffer*> objectsUBPerFragShader;
  std::vector<VulkanUBDescriptor*> customUBDescriptorPerFragShader;
  std::vector<std::vector<uint32_t>> drawCallMeshIndicesPerFragShader;
  std::vector<VulkanFSB*> flexibleBufferPerFragShader;

  // Scene / draw data
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  std::vector<MeshDrawInfo> meshPool;
  std::vector<uint32_t> drawCallMeshIndices;

  VulkanSceneUBO sceneData;
  // todo : separate UBOs for the vertex shader (shared) and UBO for each frag
  // shader
  std::vector<VulkanUniformBufferObject> ubos;

  bool loadedFirstDrawer = false;  // used to decide which drawer will clean

  std::vector<VulkanRenderTexture*> renderTextures;  // stored to clean later

 public:
  VulkanRenderer(GLFWwindow* _window, uint32_t _width, uint32_t _height);

  uint32_t loadMesh(const Mesh& mesh);

  void addMeshDrawCall(int shaderIndex, FSBObject fsbObject, uint32_t meshIndex,
                       glm::mat4 transform, bool castsShadows);

  void initSceneData(const Scene& scene);

  void drawFrame(ImDrawData* drawData = nullptr);

  VulkanRenderTexture* getMainDepthRenderTexture();
  VulkanRenderTexture* createRenderTexture(std::string name, int nColorAttachments);
  int loadShader(const VulkanShaderData& shaderData);

  uint16_t loadTexture(const std::string& texturePath);

  void buildTextureAtlas();

  glm::vec2 getTextureAtlasOffset(int textureIndex);
  glm::vec2 getRelativeTextureSize(int textureIndex);
  ~VulkanRenderer();

  void destroy();

  VkInstance getInstance() const { return instance.getInstance(); }
  VkPhysicalDevice getPhysicalDevice() const { return device.getPhysicalDevice(); }
  VkDevice getDevice() const { return device.getDevice(); }
  uint32_t getQueueFamilyIndex() const;
  VkQueue getQueue() const { return device.getGraphicsQueue(); }
  VkRenderPass getRenderPass() const;
};