#pragma once
#define GLFW_INCLUDE_VULKAN
#include "mesh.hpp"
#include "mesh_draw_info.hpp"
#include "scene_ubo.hpp"
#include "ubo.hpp"
#include "vertex.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_descriptor.hpp"
#include "vulkan_device.hpp"
#include "vulkan_image_drawer.hpp"
#include "vulkan_shadow_view.hpp"
#include "vulkan_swapchain.hpp"
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

#define MAX_VERTEX_NUMBER 100000
#define MAX_INDEX_NUMBER 100000
#define MAX_OBJECTS_UB 100000
#define MAX_SCENE_DATA 1

class VulkanRenderer {
    private:
    // Shader paths
    std::string vertShaderPath   = "./shaders/test.vert.spv";
    std::string fragShaderPath   = "./shaders/test.frag.spv";
    std::string shadowShaderPath = "./shaders/shadows.vert.spv";
    // Window info
    GLFWwindow* window;
    uint32_t width;
    uint32_t height;

    // Core Vulkan objects
    VulkanInstance instance;
    VulkanDevice device;
    VulkanSurface mainSurface;

    VulkanSwapchain swapchain;

    // Vertex/index buffers
    VkDeviceSize vertexSize = sizeof (Vertex);
    VkDeviceSize indexSize  = sizeof (uint32_t);

    VulkanBuffer vertexBuffer;
    VulkanBuffer indexBuffer;

    // Uniform buffers
    VkDeviceSize uboSize = sizeof (UniformBufferObject);
    VkDeviceSize alignment;
    VkDeviceSize uboAlignedSize;

    VulkanBuffer objectsUB;
    VulkanBuffer sceneDataUB;

    VulkanUBDescriptor objectsUBDescriptor;
    VulkanUBDescriptor sceneDataUBDescriptor;

    VulkanShadowView shadowView;
    VulkanImageDrawer mainImageDrawer;
    VulkanImageDrawer shadowImageDrawer;

    // Scene / draw data
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<MeshDrawInfo> meshPool;
    std::vector<uint32_t> drawCallMeshIndices;

    SceneUBO sceneData;
    std::vector<UniformBufferObject> ubos;

    // todo : Create shadow render pass, shadow pipeline, shadowframebuffers and shadow syncobjects
    int currentFrame = 0;

    std::vector<uint8_t>
    padData (std::vector<UniformBufferObject> ubos, VkDeviceSize alignedSize) {
        std::vector<uint8_t> paddedData (alignedSize * ubos.size (), 0); // zero-initialized

        for (size_t i = 0; i < ubos.size (); ++i) {
            std::memcpy (paddedData.data () + i * alignedSize, &ubos[i],
                         sizeof (UniformBufferObject));
        }
        return paddedData;
    }

    public:
    VulkanRenderer (GLFWwindow* _window, uint32_t _width, uint32_t _height)
    : window (_window), width (_width), height (_height), instance (window),
      device (instance), mainSurface (instance, device, window),
      swapchain (device, mainSurface),
      alignment (device.getProperties ().limits.minUniformBufferOffsetAlignment),
      uboAlignedSize ((uboSize + alignment - 1) & ~(alignment - 1)),

      vertexBuffer (device,
                    VulkanBufferType::Vertex,
                    MAX_VERTEX_NUMBER * sizeof (Vertex),
                    nullptr,
                    false,
                    0,
                    "Vertex Buffer"),
      indexBuffer (device,
                   VulkanBufferType::Index,
                   MAX_INDEX_NUMBER * sizeof (uint32_t),
                   nullptr,
                   false,
                   0,
                   "Index Buffer"),

      objectsUB (device,
                 VulkanBufferType::Uniform,
                 MAX_OBJECTS_UB * uboAlignedSize,
                 nullptr,
                 true,
                 uboAlignedSize,
                 "Objects UB"),
      sceneDataUB (device,
                   VulkanBufferType::Uniform,
                   MAX_SCENE_DATA * sizeof (SceneUBO),
                   nullptr,
                   false,
                   0,
                   "SceneData UB"),

      objectsUBDescriptor (device,
                           objectsUB,
                           VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                           uboSize,
                           "Objects UB Descriptor Set"),
      sceneDataUBDescriptor (device,
                             sceneDataUB,
                             VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                             sizeof (SceneUBO),
                             "Scene UB Descriptor Set"),
      shadowView (device, 1600, 1200, DEFAULT_SHADOW_FORMAT),
      shadowImageDrawer (device,
                         shadowView.getExtent (),
                         shadowView.getAttachmentsPerImage (),
                         { sceneDataUBDescriptor.getDescData(0, 0), objectsUBDescriptor.getDescData(0, 1) },
                         {},
                         {},
                         { createShadowDepthAttachment () },
                         { createDepthClearValue({1.0f, 0})},
                         { shadowShaderPath },
                         {},
                         VK_CULL_MODE_NONE,
                         VK_COMPARE_OP_LESS,
                         "Shadow Image Drawer"),
      mainImageDrawer (device,
                       mainSurface.getCapabilities ().currentExtent,
                       swapchain.getAttachmentsPerFramebuffer (),
                       { sceneDataUBDescriptor.getDescData(0, 0), objectsUBDescriptor.getDescData(0, 1), shadowView.getDescData(0, 2)},
                       { createColorAttachment () },
                       { createColorClearValue({0.1f, 0.1f, 0.1f, 0.1f})},
                       { createDepthAttachment () },
                       {createDepthClearValue({1.0f, 0})},
                       { vertShaderPath },
                       { fragShaderPath },
                       VK_CULL_MODE_BACK_BIT,
                        VK_COMPARE_OP_LESS,
                       "Main Image Drawer") {
        std::cout << "Vertex buffer size: " << MAX_VERTEX_NUMBER * vertexSize << std::endl;
        std::cout << "Index buffer size: " << MAX_INDEX_NUMBER * indexSize << std::endl;
        std::cout << "Objects UB size: " << MAX_OBJECTS_UB * uboAlignedSize << std::endl;
        std::cout << "Scene data UB size: " << MAX_SCENE_DATA * sizeof (SceneUBO)
                  << std::endl;
    }

    uint32_t loadMesh (const Mesh& mesh) {
        VkDeviceSize vertexOffset = vertices.size ();
        VkDeviceSize indexOffset  = indices.size ();

        const auto& meshVertices = mesh.getVertices ();
        const auto& meshNormals  = mesh.getNormals ();
        const auto& meshIndices  = mesh.getTriangles ();

        for (size_t i = 0; i < meshVertices.size (); ++i) {
            vertices.push_back ({ meshVertices[i], { 1.0f, 1.0f, 1.0f }, meshNormals[i] });
        }
        indices.insert (indices.end (), meshIndices.begin (), meshIndices.end ());

        vertexBuffer.update (vertices.data () + vertexOffset,
                             meshVertices.size () * sizeof (Vertex),
                             vertexOffset * sizeof (Vertex));
        indexBuffer.update (indices.data () + indexOffset,
                            meshIndices.size () * sizeof (uint32_t),
                            indexOffset * sizeof (uint32_t));

        meshPool.push_back ({ (uint32_t)vertexOffset, (uint32_t)indexOffset,
                              (uint32_t)mesh.getTriangles ().size () });
        return meshPool.size () - 1;
    }

    void addMeshDrawCall (uint32_t meshIndex, glm::mat4 transform) {
        drawCallMeshIndices.push_back (meshIndex);
        ubos.push_back ({ transform });
    }

    void initSceneData (const glm::mat4 view, const glm::vec3 lightDir, const glm::vec3 lightColor) {
        glm::mat4 proj =
        glm::perspective (glm::radians (80.0f),
                          mainSurface.getCapabilities ().currentExtent.width /
                          (float)mainSurface.getCapabilities ().currentExtent.height,
                          0.1f, 100.0f);
        proj[1][1] *= -1;
        sceneData = SceneUBO (view, proj, lightDir, lightColor, { 0, 0, 0 });
        sceneDataUB.update (&sceneData, sizeof (SceneUBO), 0);
    }

    void drawFrame () {
        // pad and upload object UBOs
        std::vector<uint8_t> paddedUBOs = padData (ubos, uboAlignedSize);
        objectsUB.update (paddedUBOs.data (), paddedUBOs.size (), 0);

        shadowView.drawWithDrawer (shadowImageDrawer, vertexBuffer, indexBuffer,
                                   meshPool, drawCallMeshIndices);
        swapchain.drawWithDrawer (mainImageDrawer, vertexBuffer, indexBuffer,
                                  meshPool, drawCallMeshIndices);
        
        currentFrame = (currentFrame + 1) % 3;
        ubos.clear ();
        drawCallMeshIndices.clear ();
    }

    ~VulkanRenderer () {
        destroy ();
    }

    void destroy () {
        if (instance.getInstance () == VK_NULL_HANDLE) {
            return;
        }
        vkDeviceWaitIdle (device.getDevice ());
        mainImageDrawer.destroy ();
        sceneDataUBDescriptor.destroy ();
        sceneDataUB.destroy ();
        objectsUBDescriptor.destroy ();
        objectsUB.destroy ();
        vertexBuffer.destroy ();
        indexBuffer.destroy ();
        swapchain.destroy ();
        mainSurface.destroy ();
        device.destroy ();
        instance.destroy ();
    }
};
