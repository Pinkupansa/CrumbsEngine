#pragma once
#define GLFW_INCLUDE_VULKAN
#include "mesh.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_descriptor.hpp"
#include "vulkan_device.hpp"
#include "vulkan_image_drawer.hpp"
#include "vulkan_mesh_draw_info.hpp"
#include "vulkan_scene_ubo.hpp"
#include "vulkan_shadow_view.hpp"
#include "vulkan_swapchain.hpp"
#include "vulkan_texture_bundle.hpp"
#include "vulkan_ubo.hpp"
#include "vulkan_vertex.hpp"

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

#define MAX_VERTEX_NUMBER 1000000
#define MAX_INDEX_NUMBER 1000000
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

    VulkanTextureBundle textureBundle;

    // Vertex/index buffers
    VkDeviceSize vertexSize = sizeof (Vertex);
    VkDeviceSize indexSize  = sizeof (uint32_t);

    VulkanBuffer vertexBuffer;
    VulkanBuffer indexBuffer;

    // Uniform buffers
    VkDeviceSize uboSize = sizeof (VulkanUniformBufferObject);
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

    VulkanSceneUBO sceneData;
    std::vector<VulkanUniformBufferObject> ubos;

    // todo : Create shadow render pass, shadow pipeline, shadowframebuffers and shadow syncobjects
    int currentFrame = 0;

    std::vector<uint8_t> padData (std::vector<VulkanUniformBufferObject> ubos,
                                  VkDeviceSize alignedSize) {
        std::vector<uint8_t> paddedData (alignedSize * ubos.size (), 0); // zero-initialized

        for (size_t i = 0; i < ubos.size (); ++i) {
            std::memcpy (paddedData.data () + i * alignedSize, &ubos[i],
                         sizeof (VulkanUniformBufferObject));
        }
        return paddedData;
    }

    public:
    VulkanRenderer (GLFWwindow* _window, uint32_t _width, uint32_t _height)
    : window (_window), width (_width), height (_height), instance (window),
      device (instance), mainSurface (instance, device, window),
      swapchain (device, mainSurface), textureBundle (device, ATLAS_SIZE),
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
                   MAX_SCENE_DATA * sizeof (VulkanSceneUBO),
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
                             sizeof (VulkanSceneUBO),
                             "Scene UB Descriptor Set"),
      shadowView (device, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, DEFAULT_SHADOW_FORMAT),
      shadowImageDrawer (device,
                         shadowView.getExtent (),
                         shadowView.getAttachmentsPerImage (),
                         { sceneDataUBDescriptor.getDescData (0, 0),
                           objectsUBDescriptor.getDescData (0, 1) },
                         {},
                         { createShadowDepthAttachment () },
                         {},
                         { shadowShaderPath },
                         {},
                         VK_CULL_MODE_NONE,
                         VK_COMPARE_OP_LESS,
                         "Shadow Image Drawer"),
      mainImageDrawer (device,
                       mainSurface.getCapabilities ().currentExtent,
                       swapchain.getAttachmentsPerFramebuffer (),
                       { sceneDataUBDescriptor.getDescData (0, 0),
                         objectsUBDescriptor.getDescData (0, 1),
                         shadowView.getDescData (0, 2), textureBundle.getDescData (0, 3) },
                       { createColorAttachment (false) },
                       { createDepthAttachment () },
                       { createColorAttachment(true)},
                       { vertShaderPath },
                       { fragShaderPath },
                       VK_CULL_MODE_BACK_BIT,
                       VK_COMPARE_OP_LESS,
                       "Main Image Drawer") {
        std::cout << "Vertex buffer size: " << MAX_VERTEX_NUMBER * vertexSize << std::endl;
        std::cout << "Index buffer size: " << MAX_INDEX_NUMBER * indexSize << std::endl;
        std::cout << "Objects UB size: " << MAX_OBJECTS_UB * uboAlignedSize << std::endl;
        std::cout << "Scene data UB size: " << MAX_SCENE_DATA * sizeof (VulkanSceneUBO)
                  << std::endl;
    }

    uint32_t loadMesh (const Mesh& mesh) {
        VkDeviceSize vertexOffset = vertices.size ();
        VkDeviceSize indexOffset  = indices.size ();

        const auto& meshVertices   = mesh.getVertices ();
        const auto& meshNormals    = mesh.getNormals ();
        const auto& meshIndices    = mesh.getTriangles ();
        const auto& meshUVs        = mesh.getUVs ();
        const auto& meshTangents   = mesh.getTangents ();
        const auto& meshBitangents = mesh.getBitangents ();


        for (size_t i = 0; i < meshVertices.size (); ++i) {
            vertices.push_back ({ meshVertices[i],
                                  { 1.0f, 1.0f, 1.0f },
                                  meshNormals[i],
                                  meshTangents[i],
                                  meshBitangents[i],
                                  meshUVs[i] });
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


    void addMeshDrawCall (uint32_t meshIndex,
                          glm::mat4 transform,
                          uint32_t textureIndex,
                          uint32_t normalMapIndex,
                          glm::vec2 tilingFactor = glm::vec2 (1.0f, 1.0f),
                          bool castsShadows = true,
                          bool isLit = true) {
        drawCallMeshIndices.push_back (meshIndex);
        ubos.push_back ({ transform, textureBundle.getTextureAtlasOffset (textureIndex),
                          textureBundle.getTextureSize (textureIndex),
                          textureBundle.getTextureAtlasOffset (normalMapIndex),
                          textureBundle.getTextureSize (normalMapIndex), tilingFactor, castsShadows, isLit});
    }

    void initSceneData (const Scene& scene) {
        glm::mat4 proj =
        glm::perspective (glm::radians (80.0f),
                          mainSurface.getCapabilities ().currentExtent.width /
                          (float)mainSurface.getCapabilities ().currentExtent.height,
                          0.1f, 200.0f);
        proj[1][1] *= -1;
        sceneData = VulkanSceneUBO (scene.getMainCamera()->getView(), proj, scene.lightDir, scene.lightColor, scene.groundColor, scene.skyColor);
        sceneDataUB.update (&sceneData, sizeof (VulkanSceneUBO), 0);
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

    uint16_t loadTexture (const std::string& texturePath) {
        return textureBundle.addTexture (texturePath);
    }

    void buildTextureAtlas () {
        textureBundle.buildTextureAtlas ();
    }
    ~VulkanRenderer () {
        destroy ();
    }

    void destroy () {
        if (instance.getInstance () == VK_NULL_HANDLE) {
            return;
        }
        vkDeviceWaitIdle (device.getDevice ());

        shadowImageDrawer.destroy ();
        mainImageDrawer.destroy ();
        sceneDataUBDescriptor.destroy ();
        sceneDataUB.destroy ();
        objectsUBDescriptor.destroy ();
        objectsUB.destroy ();
        vertexBuffer.destroy ();
        indexBuffer.destroy ();
        swapchain.destroy ();
        mainSurface.destroy ();
        shadowView.destroy ();
        textureBundle.destroy ();
        device.destroy ();
        instance.destroy ();
    }
};
