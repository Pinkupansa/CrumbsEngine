#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include "vulkan_renderer.hpp"
#include "primitive_meshes.hpp"
using Clock = std::chrono::high_resolution_clock;

int main(){
    if(!glfwInit()){
        std::cout << "Failed to initialize GLFW\n";
        return -1;
    }
    std::cout << "GLFW initialized!\n";

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    uint32_t width = 1000;
    uint32_t height = 750;
    GLFWwindow* window = glfwCreateWindow(width, height, "Vulkan Window", nullptr, nullptr); 

    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 2.0f), // camera position
        glm::vec3(0.0f, 0.0f, 0.0f), // look at origin
        glm::vec3(0.0f, 1.0f, 0.0f)  // up vector
    );

    glm::vec3 groundColor = {0.8f, 0.01f, 0.0f};
    VulkanRenderer renderer (window, width, height);
    renderer.initSceneData(view, {0.0f, 1.0f, 0.0f}, {0.8f, 0.8f, 0.8f}, {0.0f, 0.0f, 0.0f}, groundColor);

    Mesh teapotMesh = importMesh("teapot.fbx");
    uint32_t teapotIndex = renderer.loadMesh(teapotMesh);
    
    Mesh quadMesh = generateQuad();
    uint32_t quadIndex = renderer.loadMesh(quadMesh);
    glm::mat4 quadModel = glm::scale(glm::translate(glm::mat4(1.0f), {0.0f, -0.5f, 0.0f}), glm::vec3(30.0f));

    float elapsedTime = 0; 
    auto lastTime = Clock::now();

    while(!glfwWindowShouldClose(window)){
        auto currentTime = Clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count(); // in seconds
        lastTime = currentTime;
        elapsedTime += deltaTime;

        //Debug::Log(std::to_string(1/deltaTime));
        renderer.addMeshDrawCall(teapotIndex, glm::rotate(glm::translate(glm::mat4(1.0f), {cos(elapsedTime), -1.0f, -4.0f+sin(elapsedTime)}), elapsedTime, {0.0f, 1.0f, 0.0f}), {1.0f, 1.0f, 1.0f});
        renderer.addMeshDrawCall(teapotIndex, glm::rotate(glm::translate(glm::mat4(1.0f), {0.0f, sin(elapsedTime), -4.0f+cos(elapsedTime)}), elapsedTime, {0.0f, 1.0f, 0.0f}), {1.0f, 1.0f, 1.0f});
        renderer.addMeshDrawCall(quadIndex, quadModel,groundColor);
        renderer.drawFrame();
        glfwPollEvents();
    }

    renderer.destroy();
    glfwDestroyWindow(window);
    glfwTerminate();

}