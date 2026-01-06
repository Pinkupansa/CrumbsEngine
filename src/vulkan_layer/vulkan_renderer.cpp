#include "vulkan_layer/vulkan_renderer.hpp"

#include "engine_layer/camera.hpp"
#include "engine_layer/debug.hpp"
#include "engine_layer/mesh.hpp"
#include "engine_layer/scene.hpp"
#include "vulkan_layer/vulkan_buffer.hpp"
#include "vulkan_layer/vulkan_constants.hpp"
#include "vulkan_layer/vulkan_descriptor.hpp"
#include "vulkan_layer/vulkan_device.hpp"
#include "vulkan_layer/vulkan_flexible_shader_buffer.hpp"
#include "vulkan_layer/vulkan_image_drawer.hpp"
#include "vulkan_layer/vulkan_instance.hpp"
#include "vulkan_layer/vulkan_mesh_draw_info.hpp"
#include "vulkan_layer/vulkan_object_creation_utils.hpp"
#include "vulkan_layer/vulkan_render_texture.hpp"
#include "vulkan_layer/vulkan_scene_ubo.hpp"
#include "vulkan_layer/vulkan_shader_data.hpp"
#include "vulkan_layer/vulkan_shadow_map.hpp"
#include "vulkan_layer/vulkan_surface.hpp"
#include "vulkan_layer/vulkan_swapchain.hpp"
#include "vulkan_layer/vulkan_texture_bundle.hpp"
#include "vulkan_layer/vulkan_ubo.hpp"
#include "vulkan_layer/vulkan_vertex.hpp"
#include <GLFW/glfw3.h>
#include <functional>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

VulkanRenderer::VulkanRenderer (GLFWwindow* _window, uint32_t _width, uint32_t _height)
: window (_window), instance (),
  device (instance), mainSurface (instance, device, window),
  swapchain (device, mainSurface), textureBundle (device, ATLAS_SIZE),
  alignment (device.getProperties ().limits.minUniformBufferOffsetAlignment),
  uboAlignedSize ((uboSize + alignment - 1) & ~(alignment - 1)), isFirstFrame(false),

  vertexBuffer (device,
                VulkanBufferType::Vertex,
                MAX_VERTEX_NUMBER * sizeof (Vertex),
                nullptr,
                false,
                0,
                "Vertex Buffer"),
  indexBuffer (device, VulkanBufferType::Index, MAX_INDEX_NUMBER * sizeof (uint32_t), nullptr, false, 0, "Index Buffer"),

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
  shadowMap (device, SHADOW_MAP_SIZE, SHADOW_MAP_SIZE),
  
  shadowImageDrawer (device,
                     shadowMap.getShadowAttachment ()->getExtent (),
                     shadowMap.getAttachmentsPerFrameBuffer (),
                     true,
                     { sceneDataUBDescriptor.getDescData (0, 0),
                       allObjectsUBDescriptor.getDescData (0, 1) },
                     shadowMap.getSyncObjects (),
                     shadowMap.getFenceResetCallback (),
                     { shadowShaderPath },
                     {},
                     VK_CULL_MODE_NONE,
                     true,
                     true,
                     false,
                     VulkanAlphaBlendMode::None,
                     "Shadow Image Drawer") {
    /*TODO : Create instead one image drawer per fragShader, and one objectsUB
     * per shader ?*/
    std::cout << "Vertex buffer size: " << MAX_VERTEX_NUMBER * vertexSize << std::endl;
    std::cout << "Index buffer size: " << MAX_INDEX_NUMBER * indexSize << std::endl;
    std::cout << "Objects UB size: " << MAX_OBJECTS_UB * uboAlignedSize << std::endl;
    std::cout << "Scene data UB size: " << MAX_SCENE_DATA * sizeof (VulkanSceneUBO)
              << std::endl;

    VulkanRenderTexture* depthRenderTexture =
    new VulkanRenderTexture (device, mainSurface.getCapabilities ().currentExtent,
                             0, "Main Depth Render Texture");
    renderTextures.push_back (depthRenderTexture);
}

VulkanRenderTexture* VulkanRenderer::getMainDepthRenderTexture(){
  return renderTextures[0];
}
uint32_t VulkanRenderer::loadMesh (const Mesh& mesh) {
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

void VulkanRenderer::addMeshDrawCall (int shaderIndex,
                                      FSBObject fsbObject,
                                      uint32_t meshIndex,
                                      glm::mat4 transform,
                                      bool castsShadows) {
    if (shaderIndex >= imageDrawersPerFragShader.size ()) {
        Debug::LogWarning ("Shader index " + std::to_string (shaderIndex) + " does not exist !");
    }

    drawCallMeshIndicesPerFragShader[shaderIndex].push_back (meshIndex);
    drawCallMeshIndices.push_back (meshIndex);

    VulkanUniformBufferObject ubo = { transform, castsShadows };
    ubos.push_back (ubo);
    ubosPerFragShader[shaderIndex].push_back (ubo);

    flexibleBufferPerFragShader[shaderIndex]->addObject (fsbObject);

}

void VulkanRenderer::initSceneData (const Scene& scene) {
    glm::mat4 proj =
    glm::perspective (glm::radians (80.0f),
                      mainSurface.getCapabilities ().currentExtent.width /
                      (float)mainSurface.getCapabilities ().currentExtent.height,
                      0.1f, 500.0f);
    proj[1][1] *= -1;
    sceneData = VulkanSceneUBO (scene.getMainCamera ()->getView (), proj, scene.lightDir,
                                scene.lightColor, scene.groundColor, scene.skyColor);
    sceneDataUB.update (&sceneData, sizeof (VulkanSceneUBO), 0);
}
using Clock = std::chrono::high_resolution_clock;
void VulkanRenderer::drawFrame () {
    // pad and upload object UBOs
    

    std::vector<uint8_t> paddedUBOs = padUBOData (ubos, uboAlignedSize);
    allObjectsUB.update (paddedUBOs.data (), paddedUBOs.size (), 0);


    shadowImageDrawer.draw (vertexBuffer, indexBuffer, meshPool, drawCallMeshIndices);
    
    swapchain.updateFrameIndex ();
    Debug::Log("####HAUIOFHQWEOIFHJWOI#$####");
    for (int i = 0; i < imageDrawersPerFragShader.size (); i++) {
        if (!flexibleBufferPerFragShader[i]->isEmpty ()) {
            std::vector<uint8_t> paddedUBOsForShader =
            padUBOData (ubosPerFragShader[i], uboAlignedSize);
            objectsUBPerFragShader[i]->update (paddedUBOsForShader.data (),
                                               paddedUBOsForShader.size (), 0);
            flexibleBufferPerFragShader[i]->pushToGPU ();
        }

        imageDrawersPerFragShader[i]->draw (vertexBuffer, indexBuffer, meshPool,
                                            drawCallMeshIndicesPerFragShader[i]);
        
    }
    isFirstFrame = false;
    swapchain.present ();

    ubos.clear ();
    drawCallMeshIndices.clear ();
    for (int i = 0; i < imageDrawersPerFragShader.size (); i++) {
        flexibleBufferPerFragShader[i]->clear ();
        drawCallMeshIndicesPerFragShader[i].clear ();
        ubosPerFragShader[i].clear ();
    }
}

VulkanRenderTexture* VulkanRenderer::createRenderTexture (std::string name, int nColorAttachments) {
    VulkanRenderTexture* rendTex =
    new VulkanRenderTexture (device, mainSurface.getCapabilities ().currentExtent,
                             nColorAttachments, name);
    renderTextures.push_back (rendTex);
    return rendTex;
}
int VulkanRenderer::loadShader (const VulkanShaderData& shaderData) {
    std::string filePath = shaderData.filePath;
    fragShaderPaths.push_back (filePath);

    std::string spvPath         = filePath + ".spv";
    std::string jsonReflectPath = filePath + ".json"; // holds all uniform names

    VulkanBuffer* objectUBForShader =
    new VulkanBuffer (device, VulkanBufferType::Uniform, MAX_OBJECTS_UB * uboAlignedSize,
                      nullptr, true, uboAlignedSize, "Shader " + filePath + " ObjectUB");

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
                            fsb->getObjectSize (), "Shader " + filePath + " UBODescriptor");

    customUBDescriptorPerFragShader.push_back (customUBDescForShader);

    std::vector<VulkanDescriptorData> descriptorsForShader = {
        sceneDataUBDescriptor.getDescData (0, 0),
        objectsUBDescriptorPerFragShader.back ()->getDescData (0, 1),
        shadowMap.getTexture ().getDescData (0, 2), textureBundle.getDescData (0, 3),
        customUBDescForShader->getDescData (0, DEFAULT_CUSTOM_FRAG_PROPERTIES_SET_NUMBER)
    };

    for (int i = 0; i < shaderData.textureDescriptors.size (); i++) {
        descriptorsForShader.push_back (shaderData.textureDescriptors[i]->getDescData (
        0, DEFAULT_CUSTOM_FRAG_PROPERTIES_SET_NUMBER + 1 + i));
    }
    VulkanImageDrawer* shaderDrawer;
    if (shaderData.colorAttachments.size () == 0) {
        // render on swapchain, no depth test
        if(!shaderData.isFullScreenShader){
          //shaders rendering directly on swapchain should be fullscreen shaders sampling previous render textures
          Debug::LogWarning("Shader " + shaderData.filePath + " is rendering directly on swapchain, but is not a fullscreen shader");
        }

        shaderDrawer = new VulkanImageDrawer (
        device, mainSurface.getCapabilities ().currentExtent,
        swapchain.getAttachmentsPerFrameBuffer (), !loadedFirstDrawer, descriptorsForShader,
        swapchain.getSyncObjects (), swapchain.getFenceResetCallback (),
        { shaderData.isFullScreenShader ? fullScreenShaderPath : vertShaderPath },
        { spvPath }, shaderData.cullMode, false, false, shaderData.isFullScreenShader,
        shaderData.alphaBlendMode, "Shader " + filePath);
        loadedFirstDrawer = true;
    } else {
        std::vector<VulkanAttachment*> attachments = shaderData.colorAttachments;
        attachments.push_back (getMainDepthRenderTexture()->getDepthAttachment());
        attachments.insert (attachments.end (), shaderData.resolveAttachments.begin (),
                            shaderData.resolveAttachments.end ());
        // render on custom attachment, supposing it is color
        shaderDrawer = new VulkanImageDrawer (
        device, mainSurface.getCapabilities ().currentExtent, { attachments },
        true, descriptorsForShader, *shaderData.renderTargetSyncObjects,
        shaderData.renderTargetFenceResetCallback,
        { shaderData.isFullScreenShader ? fullScreenShaderPath : vertShaderPath },
        { spvPath }, shaderData.cullMode, shaderData.enableDepthTest,
        shaderData.enableDepthWrite, shaderData.isFullScreenShader,
        shaderData.alphaBlendMode, "Shader " + filePath);
    }

    imageDrawersPerFragShader.push_back (shaderDrawer);

    std::vector<uint32_t> drawCallMeshIndicesForShader;
    drawCallMeshIndicesPerFragShader.push_back (drawCallMeshIndicesForShader);

    std::vector<VulkanUniformBufferObject> ubosForShader;
    ubosPerFragShader.push_back (ubosForShader);

    int shaderIndex = fragShaderPaths.size () - 1;

    return shaderIndex;
}

uint16_t VulkanRenderer::loadTexture (const std::string& texturePath) {
    return textureBundle.addTexture (texturePath);
}

void VulkanRenderer::buildTextureAtlas () {
    textureBundle.buildTextureAtlas ();
}

glm::vec2 VulkanRenderer::getTextureAtlasOffset (int textureIndex) {
    return textureBundle.getTextureAtlasOffset (textureIndex);
}
glm::vec2 VulkanRenderer::getRelativeTextureSize (int textureIndex) {
    return textureBundle.getRelativeTextureSize (textureIndex);
}
VulkanRenderer::~VulkanRenderer () {
    destroy ();
}

void VulkanRenderer::destroy () {
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

        objectsUBDescriptorPerFragShader[i]->destroy ();
        delete objectsUBDescriptorPerFragShader[i];

        objectsUBPerFragShader[i]->destroy ();
        delete objectsUBPerFragShader[i];
    }
    shadowImageDrawer.destroy ();
    sceneDataUBDescriptor.destroy ();
    sceneDataUB.destroy ();
    allObjectsUBDescriptor.destroy ();
    allObjectsUB.destroy ();

    for (auto& rendTex : renderTextures) {
        rendTex->destroy ();
    }
    vertexBuffer.destroy ();
    indexBuffer.destroy ();
    swapchain.destroy ();
    mainSurface.destroy ();
    shadowMap.destroy ();
    textureBundle.destroy ();
    device.destroy ();
    instance.destroy ();
}
