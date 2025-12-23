#pragma once
#define GLFW_INCLUDE_VULKAN
#include "mesh.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_descriptor.hpp"
#include "vulkan_device.hpp"
#include "vulkan_flexible_shader_buffer.hpp"
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


class VulkanRenderer {
    private:
    // Shader paths
    std::string vertShaderPath   = "./shaders/test.vert.spv";
    std::string fragShaderPath   = "./shaders/test.frag.spv";
    std::string shadowShaderPath = "./shaders/shadows.vert.spv";

    std::vector<std::string> fragShaderPaths;
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

    VulkanBuffer allObjectsUB;
    VulkanBuffer sceneDataUB;

    VulkanUBDescriptor allObjectsUBDescriptor;
    VulkanUBDescriptor sceneDataUBDescriptor;

    VulkanShadowView shadowView;
    VulkanImageDrawer shadowImageDrawer;

    // use heap because created when loading shader and hold references to VkObjects that are destroyed if the object is destroyed
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
    // todo : separate UBOs for the vertex shader (shared) and UBO for each frag shader
    std::vector<VulkanUniformBufferObject> ubos;


    bool loadedFirstDrawer = false; // used to decide which drawer will clean


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

      allObjectsUB (device,
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

      allObjectsUBDescriptor (device,
                           allObjectsUB,
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
                           allObjectsUBDescriptor.getDescData (0, 1) },
                         {},
                         { createShadowDepthAttachment () },
                         {},
                         { shadowShaderPath },
                         {},
                         VK_CULL_MODE_NONE,
                         VK_COMPARE_OP_LESS,
                         "Shadow Image Drawer")
    /*TODO : Create instead one image drawer per fragShader, and one objectsUB per shader ?*/ {
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


    void addMeshDrawCall (int shaderIndex, FSBObject fsbObject, uint32_t meshIndex, glm::mat4 transform, bool castsShadows) {
        if (shaderIndex >= imageDrawersPerFragShader.size ()) {
            Debug::LogWarning ("Shader index " + std::to_string (shaderIndex) + " does not exist !");
        }

        drawCallMeshIndicesPerFragShader[shaderIndex].push_back (meshIndex);
        drawCallMeshIndices.push_back (meshIndex);

        VulkanUniformBufferObject ubo = {transform, castsShadows};
        ubos.push_back (ubo);
        ubosPerFragShader[shaderIndex].push_back(ubo);

        int objectIndex = flexibleBufferPerFragShader[shaderIndex]->addObject (fsbObject);

        // flexibleBufferPerFragShader[shaderIndex]->debugPrintObject(objectIndex);
    }

    void initSceneData (const Scene& scene) {
        glm::mat4 proj =
        glm::perspective (glm::radians (80.0f),
                          mainSurface.getCapabilities ().currentExtent.width /
                          (float)mainSurface.getCapabilities ().currentExtent.height,
                          0.1f, 200.0f);
        proj[1][1] *= -1;
        sceneData =
        VulkanSceneUBO (scene.getMainCamera ()->getView (), proj, scene.lightDir,
                        scene.lightColor, scene.groundColor, scene.skyColor);
        sceneDataUB.update (&sceneData, sizeof (VulkanSceneUBO), 0);
    }

    void drawFrame () {
        
        // pad and upload object UBOs
        std::vector<uint8_t> paddedUBOs = padUBOData (ubos, uboAlignedSize);
        allObjectsUB.update (paddedUBOs.data (), paddedUBOs.size (), 0);

        shadowView.drawWithDrawer (shadowImageDrawer, vertexBuffer, indexBuffer,
                                   meshPool, drawCallMeshIndices);

        swapchain.updateFrameIndex ();
        //dummy draw for clear pass not to be empty

        addMeshDrawCall(0, flexibleBufferPerFragShader[0]->getDefaultObject(), 0, glm::scale(glm::mat4(1.0f), glm::vec3(0)), false); 

        for (int i = 0; i < imageDrawersPerFragShader.size (); i++) {
            if (flexibleBufferPerFragShader[i]->isEmpty ()) {
                continue;
            }
            std::vector<uint8_t> paddedUBOsForShader = padUBOData(ubosPerFragShader[i], uboAlignedSize);
            objectsUBPerFragShader[i]->update(paddedUBOsForShader.data(), paddedUBOsForShader.size(), 0);
            flexibleBufferPerFragShader[i]->pushToGPU ();
            swapchain.drawWithDrawer (*imageDrawersPerFragShader[i],
                                      vertexBuffer, indexBuffer, meshPool, i == 0,
                                      drawCallMeshIndicesPerFragShader[i]);
        }
        // swapchain.drawWithDrawer (mainImageDrawer, vertexBuffer, indexBuffer,
        //   meshPool, drawCallMeshIndices);

        swapchain.present ();

        ubos.clear ();
        drawCallMeshIndices.clear ();
        for (int i = 0; i < imageDrawersPerFragShader.size (); i++) {
            flexibleBufferPerFragShader[i]->clear ();
            drawCallMeshIndicesPerFragShader[i].clear ();
            ubosPerFragShader[i].clear();
        }
    }

    int loadShader (const std::string& filePath, VkCompareOp compareOp) {
        fragShaderPaths.push_back (filePath);

        std::string spvPath = filePath + ".spv";
        std::string jsonReflectPath = filePath + ".json"; // holds all uniform names

        VulkanBuffer* objectUBForShader =
        new VulkanBuffer (device, VulkanBufferType::Uniform,
                          MAX_OBJECTS_UB * uboAlignedSize, nullptr, true,
                          uboAlignedSize, "Shader " + filePath + " ObjectUB");

        objectsUBPerFragShader.push_back (objectUBForShader);

        VulkanUBDescriptor* objectUBDescForShader =
        new VulkanUBDescriptor (device, *objectsUBPerFragShader.back (),
                                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                uboSize, "Shader " + filePath + "Objects UB Descriptor Set");

        objectsUBDescriptorPerFragShader.push_back (objectUBDescForShader);

        VulkanFSB* fsb = new VulkanFSB (device, jsonReflectPath,
                                        DEFAULT_CUSTOM_FRAG_PROPERTIES_SET_NUMBER,
                                        alignment, "Shader buffer " + filePath);
        flexibleBufferPerFragShader.push_back (fsb);

        VulkanUBDescriptor* customUBDescForShader =
        new VulkanUBDescriptor (device, flexibleBufferPerFragShader.back ()->getBuffer (),
                                VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT,
                                fsb->getObjectSize (),
                                "Shader " + filePath + " UBODescriptor");

        customUBDescriptorPerFragShader.push_back (customUBDescForShader);

        
        VulkanImageDrawer* shaderDrawer = new VulkanImageDrawer (
        device, mainSurface.getCapabilities ().currentExtent,
        swapchain.getAttachmentsPerImage (),
        { sceneDataUBDescriptor.getDescData (0, 0),
          objectsUBDescriptorPerFragShader.back ()->getDescData (0, 1),
          shadowView.getDescData (0, 2), textureBundle.getDescData (0, 3),
          customUBDescForShader->getDescData (0, DEFAULT_CUSTOM_FRAG_PROPERTIES_SET_NUMBER) },

        { createColorAttachment (false, loadedFirstDrawer ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR) },
        { createDepthAttachment (loadedFirstDrawer ? VK_ATTACHMENT_LOAD_OP_LOAD :
                                                     VK_ATTACHMENT_LOAD_OP_CLEAR) },
        { createColorAttachment (true, loadedFirstDrawer ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR) },
        { vertShaderPath }, { spvPath },

        VK_CULL_MODE_BACK_BIT, compareOp, "Shader " + filePath);

        imageDrawersPerFragShader.push_back (shaderDrawer);
        
        std::vector<uint32_t> drawCallMeshIndicesForShader;
        drawCallMeshIndicesPerFragShader.push_back (drawCallMeshIndicesForShader);

        std::vector<VulkanUniformBufferObject> ubosForShader; 
        ubosPerFragShader.push_back(ubosForShader);

        int shaderIndex = fragShaderPaths.size () - 1; 
        loadedFirstDrawer = true;

        return shaderIndex;
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


        for (int i = 0; i < imageDrawersPerFragShader.size (); i++) {
            flexibleBufferPerFragShader[i]->destroy ();
            delete flexibleBufferPerFragShader[i];

            customUBDescriptorPerFragShader[i]->destroy ();
            delete customUBDescriptorPerFragShader[i];

            imageDrawersPerFragShader[i]->destroy ();
            delete imageDrawersPerFragShader[i];

            objectsUBDescriptorPerFragShader[i]->destroy();
            delete objectsUBDescriptorPerFragShader[i];

            objectsUBPerFragShader[i]->destroy();
            delete objectsUBPerFragShader[i];
        }
        shadowImageDrawer.destroy ();
        sceneDataUBDescriptor.destroy ();
        sceneDataUB.destroy ();
        allObjectsUBDescriptor.destroy ();
        allObjectsUB.destroy ();
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
