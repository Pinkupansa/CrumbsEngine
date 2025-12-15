#define GLFW_INCLUDE_VULKAN

#include "primitive_meshes.hpp"
#include "vulkan_renderer.hpp"
#include <GLFW/glfw3.h>
#include "trajectory.hpp"
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
        glm::vec3(3.0f, 2.0f, 10.0f), // camera position
        glm::vec3(0.0f, 0.0f, 0.0f), // look at origin
        glm::vec3(0.0f, 1.0f, 0.0f)  // up vector
    );

    VulkanRenderer renderer(window, width, height);

    Mesh tetrahedron = importMesh("plant.fbx");
    renderer.initSceneData(view, {0.0f, 3.0f, 10.0f}, {0.8f, 0.8f, 0.8f});

    uint32_t teapotIndex = renderer.loadMesh(tetrahedron);
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.3f, 0, -3.0f));

    Mesh quadMesh = generateQuad();
    uint32_t quadIndex = renderer.loadMesh(quadMesh);
    glm::mat4 quadModel = glm::scale(glm::translate(glm::mat4(1.0f), {0.0f, -1.0f, 0.0f}), glm::vec3(30.0f));

    Mesh sphere = generateSphere();
    uint32_t sphereIndex = renderer.loadMesh(sphere);

    glm::mat4 sphereModel = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0, -3.0f));


    Mesh debugTextureQuad = generateQuad();
    uint32_t debugTextureQuadIndex = renderer.loadMesh(debugTextureQuad);
    glm::mat4 debugTextureQuadModel = glm::translate(glm::rotate(glm::scale(glm::mat4(1.0f), glm::vec3(1.0f)), 1.57f, {1.0f, 0.0f, 0.0f}), {0.0f, 1.0f, 1.0f});
    float elapsedTime = 0;
    auto lastTime = Clock::now();

    //load all texture in the texture folder
    // go through all files in the textures/ directory
    
    int oxygenTexIndex = renderer.loadTexture("textures/lava.png");
    int carbonTexIndex = renderer.loadTexture("textures/thing.png");
    int brickTexIndex = renderer.loadTexture("textures/tiles.jpg");
    int stonewallNormalMapIndex = renderer.loadTexture("textures/rock_normal_map.png");


    renderer.buildTextureAtlas();





    Trajectory traj = readTrajectory("test_traj.txt");

    float deltaFrame = 0.005; 
    while (!glfwWindowShouldClose(window)) {
        auto currentTime = Clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count(); // in seconds
        lastTime = currentTime;
        elapsedTime += deltaTime/10; 
        
        //Debug::Log(std::to_string(1 / deltaTime));
        Debug::Log(std::to_string(elapsedTime));
        //renderer.addMeshDrawCall(teapotIndex, glm::rotate(glm::translate(glm::scale(glm::mat4(1.0f), glm::vec3(0.05f)), {0, -20.0f, -20.0f}), -1.6f, {1.0f, 0.0f, 0.0f}), dragonTexIndex, -1);
        //renderer.addMeshDrawCall(sphereIndex, sphereModel, rockwallTexIndex, stonewallNormalMapIndex, glm::vec2(5.0f, 3.0f));
        //renderer.addMeshDrawCall(quadIndex, quadModel, brickTexIndex, -1, glm::vec2(30.0f, 30.0f));
        //renderer.addMeshDrawCall(debugTextureQuadIndex, debugTextureQuadModel, 0, -1) ;
        
        
        int frameIndex = (int)(elapsedTime/deltaFrame); 
        for(glm::vec3 pos : traj.data["H"][frameIndex]){
            glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), pos), glm::vec3(0.05f));
            renderer.addMeshDrawCall(sphereIndex, model, -1, -1);
        }
        for(glm::vec3 pos : traj.data["O"][frameIndex]){
            glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), pos), glm::vec3(0.05f));
            renderer.addMeshDrawCall(sphereIndex, model, oxygenTexIndex, -1);
        }
        for(glm::vec3 pos : traj.data["C"][frameIndex]){
            glm::mat4 model = glm::scale(glm::translate(glm::mat4(1.0f), pos), glm::vec3(0.05f));
            renderer.addMeshDrawCall(sphereIndex, model, carbonTexIndex, -1);
        }
        
        renderer.drawFrame();
        



        //if w is pressed translate the cam 
        
        glfwPollEvents();
    }

    renderer.destroy();
    glfwDestroyWindow(window);
    glfwTerminate();
}