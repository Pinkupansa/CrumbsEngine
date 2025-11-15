#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <glm/gtc/matrix_transform.hpp>
#include "vulkan_wrappers.hpp"
#include "vertex.hpp"
#include "ubo.hpp"
#include "scene_ubo.hpp"
#include "mesh.hpp"
#include "mesh_draw_info.hpp"

#define MAX_VERTEX_NUMBER 100000
#define MAX_INDEX_NUMBER 100000
#define MAX_OBJECTS_UB 100000
#define MAX_SCENE_DATA 1
#define SHADOW_DEPTH_FORMAT VK_FORMAT_D32_SFLOAT
#define SHADOW_RES 1024
class VulkanRenderer
{
private:
    // Shader paths
    std::string vertShaderPath = "./shaders/test.vert.spv";
    std::string fragShaderPath = "./shaders/test.frag.spv";
    std::string shadowShaderPath = "./shaders/testshadow.vert.spv";
    // Window info
    GLFWwindow *window;
    uint32_t width;
    uint32_t height;

    // Core Vulkan objects
    VulkanInstance instance;
    VulkanDevice device;
    VulkanSwapchain swapchain;
    VulkanRenderPass mainRenderPass;
    VulkanRenderPass shadowRenderPass;

    // Vertex/index buffers
    VkDeviceSize vertexSize = sizeof(Vertex);
    VkDeviceSize indexSize = sizeof(uint32_t);

    VulkanBuffer vertexBuffer;
    VulkanBuffer indexBuffer;

    // Uniform buffers
    VkDeviceSize uboSize = sizeof(UniformBufferObject);
    VkDeviceSize alignment;
    VkDeviceSize uboAlignedSize;

    VulkanBuffer objectsUB;
    VulkanBuffer sceneDataUB;

    VulkanUBDescriptor objectsUBDescriptor;
    VulkanUBDescriptor sceneDataUBDescriptor;

    VulkanShadowView shadowView;
    // Pipeline and framebuffers
    VulkanPipeline mainPipeline;
    VulkanPipeline shadowPipeline;

    VulkanFramebuffers mainFramebuffers;
    VulkanFramebuffers shadowFramebuffers;

    // Command buffers
    VulkanCommandBuffers commandBuffers;
    VulkanCommandBuffers shadowCommandBuffers;
    // Synchronization
    VulkanSyncObjects syncObjects;
    VulkanSyncObjects shadowSyncObjects;

    std::vector<VkPipelineStageFlags> waitStages{VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    // Scene / draw data
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<MeshDrawInfo> meshPool;
    std::vector<uint32_t> drawCallMeshIndices;

    SceneUBO sceneData;
    std::vector<UniformBufferObject> ubos;

    int currentFrame = 0;

    std::vector<uint8_t> padData(std::vector<UniformBufferObject> ubos, VkDeviceSize alignedSize)
    {
        std::vector<uint8_t> paddedData(alignedSize * ubos.size(), 0); // zero-initialized

        for (size_t i = 0; i < ubos.size(); ++i)
        {
            std::memcpy(paddedData.data() + i * alignedSize, &ubos[i], sizeof(UniformBufferObject));
        }
        return paddedData;
    }

public:
    VulkanRenderer(GLFWwindow *_window, uint32_t _width, uint32_t _height)
        : window(_window), width(_width), height(_height),
          instance(_window),
          device(instance),
          swapchain(device, instance, width, height),
          mainRenderPass(device, swapchain.getFormat(), swapchain.getDepthFormat(), true, true),
          shadowRenderPass(device, SHADOW_DEPTH_FORMAT, SHADOW_DEPTH_FORMAT, false, true, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),

          // alignment must be initialized before using it
          alignment(device.getProperties().limits.minUniformBufferOffsetAlignment),
          uboAlignedSize((uboSize + alignment - 1) & ~(alignment - 1)),

          vertexBuffer(device, VulkanBufferType::Vertex, MAX_VERTEX_NUMBER * sizeof(Vertex), nullptr, false, 0, "Vertex Buffer"),
          indexBuffer(device, VulkanBufferType::Index, MAX_INDEX_NUMBER * sizeof(uint32_t), nullptr, false, 0, "Index Buffer"),

          objectsUB(device, VulkanBufferType::Uniform, MAX_OBJECTS_UB * uboAlignedSize, nullptr, true, uboAlignedSize, "Objects UB"),
          sceneDataUB(device, VulkanBufferType::Uniform, MAX_SCENE_DATA * sizeof(SceneUBO), nullptr, false, 0, "SceneData UB"),

          objectsUBDescriptor(device, objectsUB, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, uboSize),
          sceneDataUBDescriptor(device, sceneDataUB, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(SceneUBO)),
          shadowView(device, (uint)SHADOW_RES, (uint)SHADOW_RES, SHADOW_DEPTH_FORMAT),
          mainPipeline(device, mainRenderPass, swapchain.getExtent(), sceneDataUBDescriptor, objectsUBDescriptor, shadowView, true, vertShaderPath, fragShaderPath),
          shadowPipeline(device, shadowRenderPass, shadowView.getExtent(), sceneDataUBDescriptor, objectsUBDescriptor, shadowView, false, shadowShaderPath, ""),
          mainFramebuffers(device, mainRenderPass, 3, swapchain.getAttachmentPerSwapchainImage(), swapchain.getExtent()),
          shadowFramebuffers(device, shadowRenderPass, 1, shadowView.getAttachmentsPerImage(), shadowView.getExtent()),
          commandBuffers(device, mainFramebuffers),
          shadowCommandBuffers(device, shadowFramebuffers),
          syncObjects(device, 3), shadowSyncObjects(device, 1)
    {}


    uint32_t loadMesh(const Mesh &mesh)
    {
        VkDeviceSize vertexOffset = vertices.size();
        VkDeviceSize indexOffset = indices.size();

        const auto &meshVertices = mesh.getVertices();
        const auto &meshNormals = mesh.getNormals();
        const auto &meshIndices = mesh.getTriangles();

        for (size_t i = 0; i < meshVertices.size(); ++i)
        {
            vertices.push_back({meshVertices[i], {1.0f, 1.0f, 1.0f}, meshNormals[i]});
        }
        indices.insert(indices.end(), meshIndices.begin(), meshIndices.end());

        vertexBuffer.update(vertices.data() + vertexOffset, meshVertices.size() * sizeof(Vertex), vertexOffset * sizeof(Vertex));
        indexBuffer.update(indices.data() + indexOffset, meshIndices.size() * sizeof(uint32_t), indexOffset * sizeof(uint32_t));

        meshPool.push_back({(uint32_t)vertexOffset, (uint32_t)indexOffset, (uint32_t)mesh.getTriangles().size()});
        return meshPool.size() - 1;
    }

    void addMeshDrawCall(uint32_t meshIndex, glm::mat4 transform, glm::vec3 objectColor)
    {
        drawCallMeshIndices.push_back(meshIndex);
        ubos.push_back({transform, objectColor});
    }

    void initSceneData(const glm::mat4 view, const glm::vec3 lightDir, const glm::vec3 lightColor, const glm::vec3 ambientLight, const glm::vec3 groundColor)
    {
        glm::mat4 proj = glm::perspective(glm::radians(85.0f),
                                          swapchain.getExtent().width / (float)swapchain.getExtent().height,
                                          0.1f, 100.0f);
        proj[1][1] *= -1;
        sceneData = {view, proj, lightDir, lightColor, ambientLight, groundColor};
        sceneDataUB.update(&sceneData, sizeof(SceneUBO), 0);
    }

    void drawShadows(){
        vkWaitForFences(device.getDevice(), 1, &shadowSyncObjects.inFlightFence[0], VK_TRUE, UINT64_MAX);
        vkResetFences(device.getDevice(), 1, &shadowSyncObjects.inFlightFence[0]);

        shadowCommandBuffers.record(device, shadowView.getExtent(), shadowRenderPass, shadowFramebuffers, 
                                    vertexBuffer, indexBuffer, sceneDataUBDescriptor, objectsUBDescriptor, 
                                    shadowView, shadowPipeline, meshPool, drawCallMeshIndices, 0);
        // Submit shadow pass
        VkSubmitInfo shadowSubmit{};
        shadowSubmit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        shadowSubmit.waitSemaphoreCount = 0;
        shadowSubmit.commandBufferCount = 1;
        shadowSubmit.pCommandBuffers = &shadowCommandBuffers.getCommandBuffers()[0];
        shadowSubmit.signalSemaphoreCount = 1;
        shadowSubmit.pSignalSemaphores = &shadowSyncObjects.renderFinishedSemaphore[0];


        vkQueueSubmit(device.getGraphicsQueue(), 1, &shadowSubmit, shadowSyncObjects.inFlightFence[0]);

    }
    void drawFrame()
    {
        drawShadows();
        // pad and upload object UBOs
        std::vector<uint8_t> paddedUBOs = padData(ubos, uboAlignedSize);
        objectsUB.update(paddedUBOs.data(), paddedUBOs.size(), 0);

        
        vkWaitForFences(device.getDevice(), 1, &syncObjects.inFlightFence[currentFrame], VK_TRUE, UINT64_MAX);
        vkResetFences(device.getDevice(), 1, &syncObjects.inFlightFence[currentFrame]);

        uint32_t imageIndex;
        vkAcquireNextImageKHR(device.getDevice(), swapchain.getSwapchain(),
                              UINT64_MAX, syncObjects.imageAvailableSemaphore[currentFrame], VK_NULL_HANDLE, &imageIndex);

        // record command buffer for this image
        commandBuffers.record(device, swapchain.getExtent(), mainRenderPass, mainFramebuffers,
                               vertexBuffer, indexBuffer, sceneDataUBDescriptor,
                               objectsUBDescriptor, shadowView, mainPipeline, meshPool,
                               drawCallMeshIndices, imageIndex);
        
        std::vector<VkSemaphore> waitSems = {shadowSyncObjects.renderFinishedSemaphore[0], syncObjects.imageAvailableSemaphore[currentFrame] };
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = waitSems.size();
        submitInfo.pWaitSemaphores = waitSems.data();
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &syncObjects.renderFinishedSemaphore[currentFrame];
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers.getCommandBuffers()[imageIndex];

        vkQueueSubmit(device.getGraphicsQueue(), 1, &submitInfo, syncObjects.inFlightFence[currentFrame]);

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &syncObjects.renderFinishedSemaphore[currentFrame];
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain.getSwapchain();
        presentInfo.pImageIndices = &imageIndex;
        vkQueuePresentKHR(device.getGraphicsQueue(), &presentInfo);

        currentFrame = (currentFrame + 1) % 3;
        ubos.clear();
        drawCallMeshIndices.clear();

        
    }

    ~VulkanRenderer()
    {
        destroy();
    }

    void destroy()
    {
        if(device.getDevice() == VK_NULL_HANDLE){
            return;
        }

        if (instance.getInstance() == VK_NULL_HANDLE)
        {
            return;
        }
        vkDeviceWaitIdle(device.getDevice());
        syncObjects.destroy();
        shadowSyncObjects.destroy();
        shadowFramebuffers.destroy();
        mainFramebuffers.destroy();
        commandBuffers.destroy();
        shadowCommandBuffers.destroy();
        mainPipeline.destroy();
        shadowPipeline.destroy();
        shadowView.destroy();
        sceneDataUBDescriptor.destroy();
        sceneDataUB.destroy();
        objectsUBDescriptor.destroy();
        objectsUB.destroy();
        vertexBuffer.destroy();
        indexBuffer.destroy();
        shadowRenderPass.destroy();
        mainRenderPass.destroy();
        swapchain.destroy();
        device.destroy();
        instance.destroy();
    }
};
