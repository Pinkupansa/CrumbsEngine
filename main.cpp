#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include <iostream>
#include <vulkan/vulkan.h>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>
#include "vertex.hpp"
#include "ubo.hpp"
#include "scene_ubo.hpp"
#include <chrono>
#include <glm/gtc/matrix_transform.hpp>
#include "vulkan_wrappers.hpp"



using Clock = std::chrono::high_resolution_clock;

std::vector<Vertex> vertices = {
    { {-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f} }, // bottom-left
    { {-0.5f,  0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f} }, // top-left
    { { 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f} }, // top-right
    { { 0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f} }  // bottom-right
};


std::vector<uint16_t> indices = {
    2, 1, 0, 0, 3, 2, 
    2, 1, 0, 0, 3, 2
};

std::vector<uint8_t> padData(std::vector<UniformBufferObject> ubos, VkDeviceSize alignedSize){
    std::vector<uint8_t> paddedData(alignedSize * ubos.size(), 0); // zero-initialized

    for (size_t i = 0; i < ubos.size(); ++i) {
        std::memcpy(paddedData.data() + i * alignedSize, &ubos[i], sizeof(UniformBufferObject));
    }
    return paddedData;
}
int main(){
    std::string vertShaderPath = "./shaders/test.vert.spv";
    std::string fragShaderPath = "./shaders/test.frag.spv";

    if(!glfwInit()){
        std::cout << "Failed to initialize GLFW\n";
        return -1;
    }
    std::cout << "GLFW initialized!\n";

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    uint32_t width = 800;
    uint32_t height = 600;
    GLFWwindow* window = glfwCreateWindow(width, height, "Vulkan Window", nullptr, nullptr); 

    VulkanInstance instance(window);
    
    VulkanDevice device(instance);

    VulkanSwapchain swapchain(device, instance, width, height);

    VulkanRenderPass renderPass(device, swapchain);

    glm::mat4 proj = glm::perspective(
        glm::radians(45.0f),         // fov
        swapchain.getExtent().width / (float)swapchain.getExtent().height, // aspect ratio
        0.1f, 100.0f                  // near/far planes
    );

    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 2.0f), // camera position
        glm::vec3(0.0f, 0.0f, 0.0f), // look at origin
        glm::vec3(0.0f, 1.0f, 0.0f)  // up vector
    );
    SceneUBO sceneData {view, proj, glm::vec3(0.0f, 0.0f, 2.0f), glm::vec3(1.0f) };
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f, 0, -2));
    model = glm::rotate(model, 1.0f, glm::vec3(0, 1, 0));
    UniformBufferObject ubo1{model};
    UniformBufferObject ubo2{glm::translate(glm::mat4(1.0f), glm::vec3(0.3f, 0, 0.2f))};
    
    std::vector<UniformBufferObject> ubos{ubo1, ubo2};
    std::vector<uint32_t> firstIndexArray{0, 6, 12};

    VkDeviceSize alignment = device.getProperties().limits.minUniformBufferOffsetAlignment;
    VkDeviceSize uboSize = sizeof(UniformBufferObject);
    VkDeviceSize alignedSize = (uboSize + alignment - 1) & ~(alignment - 1); // align to minUniformBufferOffsetAlignment

    VkDeviceSize bufferSize = alignedSize * ubos.size(); // enough space for N objects

    std::vector<uint8_t> paddedUBOs = padData(ubos, alignedSize);
    VulkanBuffer staticObjectsUB(device, VulkanBufferType::Uniform, bufferSize, paddedUBOs.data(), true, alignedSize, "Static Objects UB");
    VulkanDescriptor staticObjectsUBDescriptor(device, staticObjectsUB, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT , sizeof(UniformBufferObject));

    VulkanBuffer sceneDataUB(device, VulkanBufferType::Uniform, sizeof(SceneUBO), &sceneData, false, 0, "Scene UB");
    VulkanDescriptor sceneDataUBDescriptor(device, sceneDataUB, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(SceneUBO));
    
    VulkanPipeline graphicsPipeline(device, renderPass, swapchain, sceneDataUBDescriptor, staticObjectsUBDescriptor, vertShaderPath, fragShaderPath);


    VulkanBuffer indexBuffer(device, VulkanBufferType::Index, sizeof(uint16_t) * indices.size(), indices.data(), false, 0, "Index Buffer");

    VulkanBuffer vertexBuffer(device, VulkanBufferType::Vertex, sizeof(Vertex)*vertices.size(), vertices.data(), false, 0, "Vertex Buffer");

    
    VulkanFramebuffers framebuffers(device, swapchain, renderPass);

    VulkanCommandBuffers commandBuffers(device, framebuffers);
    commandBuffers.record(device, swapchain, renderPass, framebuffers, vertexBuffer, indexBuffer, sceneDataUBDescriptor,staticObjectsUBDescriptor, firstIndexArray, graphicsPipeline);

    VulkanSyncObjects syncObjects(device, 3);


    
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    

    float elapsedTime = 0; 
    auto lastTime = Clock::now();

    int currentFrame = 0; 
    //Render loop
    while(!glfwWindowShouldClose(window)){
        auto currentTime = Clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count(); // in seconds
        //std::cout << 1.0f / deltaTime << std::endl;
        lastTime = currentTime;
        elapsedTime += deltaTime;

        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f, 0, -2));
        model = glm::rotate(model, elapsedTime * 2, glm::vec3(1, 0, 1));
        UniformBufferObject ubo1{model};
        UniformBufferObject ubo2{glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(cos(elapsedTime), 0, -3.0f)), elapsedTime*3, glm::vec3(0, 1.0f, 1.0f))};
        ubos = {ubo1, ubo2}; 
        std::vector<uint8_t> paddedUBOs = padData(ubos, alignedSize);

        staticObjectsUB.update(paddedUBOs.data(), paddedUBOs.size(), 0);
    
        //wait till GPU finishes rendering previous frame
        vkWaitForFences(device.getDevice(), 1, &syncObjects.inFlightFence[currentFrame], VK_TRUE, UINT64_MAX); 
        vkResetFences(device.getDevice(), 1, &syncObjects.inFlightFence[currentFrame]);
        
        //ask for the next image that will be rendered into. bind a semaphore to know when it's available.
        //this semaphore must not be in use by another image, so we use as many images as frames in the swapchain
        uint32_t imageIndex;
        vkAcquireNextImageKHR(device.getDevice(), swapchain.getSwapchain(),
                      UINT64_MAX, syncObjects.imageAvailableSemaphore[currentFrame],
                      VK_NULL_HANDLE, &imageIndex);
        
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &syncObjects.imageAvailableSemaphore[currentFrame];
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

        glfwPollEvents();
    }
    

    glfwDestroyWindow(window);
    glfwTerminate();


}