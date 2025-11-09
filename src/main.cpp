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

    uint32_t width = 800;
    uint32_t height = 600;
    GLFWwindow* window = glfwCreateWindow(width, height, "Vulkan Window", nullptr, nullptr); 

    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 2.0f), // camera position
        glm::vec3(0.0f, 0.0f, 0.0f), // look at origin
        glm::vec3(0.0f, 1.0f, 0.0f)  // up vector
    );

    VulkanRenderer renderer (window, width, height);

    Mesh tetrahedron = importMesh("teapot.fbx");
    renderer.initSceneData(view, {0.0f, 1.0f, 1.0f}, {0.2f, 0.9f, 0.3f});
    
    uint32_t quadIndex = renderer.loadMesh(tetrahedron);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.3f, 0, -3.0f));
    
    float elapsedTime = 0; 
    auto lastTime = Clock::now();

    while(!glfwWindowShouldClose(window)){
        auto currentTime = Clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count(); // in seconds
        lastTime = currentTime;
        elapsedTime += deltaTime;

        //Debug::Log(std::to_string(1/deltaTime));
        renderer.addMeshDrawCall(quadIndex, glm::rotate(glm::translate(glm::mat4(1.0f), {cos(elapsedTime), -1.0f, -4.0f+sin(elapsedTime)}), elapsedTime, {0.0f, 1.0f, 0.0f}));
         renderer.addMeshDrawCall(quadIndex, glm::rotate(glm::translate(glm::mat4(1.0f), {0.0f, sin(elapsedTime), -4.0f+cos(elapsedTime)}), elapsedTime, {0.0f, 1.0f, 0.0f}));
        renderer.drawFrame();
        glfwPollEvents();
    }

    renderer.destroy();
    glfwDestroyWindow(window);
    glfwTerminate();

}