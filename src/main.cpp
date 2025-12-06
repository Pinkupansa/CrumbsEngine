#define GLFW_INCLUDE_VULKAN

#include "primitive_meshes.hpp"
#include "vulkan_renderer.hpp"
#include <GLFW/glfw3.h>
using Clock = std::chrono::high_resolution_clock;

int main() {
    if (!glfwInit()) {
        std::cout << "Failed to initialize GLFW\n";
        return -1;
    }
    std::cout << "GLFW initialized!\n";

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    uint32_t width = 1600;
    uint32_t height = 900;
    GLFWwindow *window = glfwCreateWindow(width, height, "Vulkan Window", nullptr, nullptr);

    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 2.0f), // camera position
        glm::vec3(0.0f, 0.0f, 0.0f), // look at origin
        glm::vec3(0.0f, 1.0f, 0.0f)  // up vector
    );

    VulkanRenderer renderer(window, width, height);

    Mesh tetrahedron = importMesh("teapot.fbx");
    renderer.initSceneData(view, {0.0f, 3.0f, 10.0f}, {0.5f, 0.5f, 0.5f});

    uint32_t teapotIndex = renderer.loadMesh(tetrahedron);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.3f, 0, -3.0f));

    Mesh quadMesh = generateQuad();
    uint32_t quadIndex = renderer.loadMesh(quadMesh);
    glm::mat4 quadModel = glm::scale(glm::translate(glm::mat4(1.0f), {0.0f, -1.0f, 0.0f}), glm::vec3(30.0f));

    Mesh sphere = generateSphere();
    uint32_t sphereIndex = renderer.loadMesh(sphere);

    glm::mat4 sphereModel = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0, -3.0f));

    float elapsedTime = 0;
    auto lastTime = Clock::now();

    uint16_t poolTexIndex = renderer.loadTexture("textures/pooltex.jpg");
    Debug::Log("Loaded pool texture at index: " + std::to_string(poolTexIndex));
    int porcelainTexIndex = renderer.loadTexture("textures/porcelain.jpg");
    while (!glfwWindowShouldClose(window)) {
        auto currentTime = Clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count(); // in seconds
        lastTime = currentTime;
        elapsedTime += deltaTime;

        
        //Debug::Log(std::to_string(1 / deltaTime));
        renderer.addMeshDrawCall(teapotIndex, glm::rotate(glm::translate(glm::mat4(1.0f), {2*cos(elapsedTime), -1.0f, -4.0f + 2*sin(elapsedTime)}), elapsedTime, {0.0f, 1.0f, 0.0f}), porcelainTexIndex);
        renderer.addMeshDrawCall(sphereIndex, sphereModel, poolTexIndex);
        renderer.addMeshDrawCall(quadIndex, quadModel, poolTexIndex);
        renderer.drawFrame();
        glfwPollEvents();
    }

    renderer.destroy();
    glfwDestroyWindow(window);
    glfwTerminate();
}