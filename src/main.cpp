#define GLFW_INCLUDE_VULKAN

#include "crumb.hpp"
#include "primitive_meshes.hpp"
#include "scene.hpp"
#include "vulkan_renderer.hpp"
#include <GLFW/glfw3.h>
using Clock = std::chrono::high_resolution_clock;


void renderCrumb (VulkanRenderer& renderer, Crumb& crumb) {
    if (!crumb.renderData.has_value ()) {
        return;
    }
    renderer.addMeshDrawCall (crumb.renderData.value ().meshIndex,
                              crumb.transform.getModelMatrix (),
                              crumb.renderData.value ().material.textureImageIndex,
                              crumb.renderData.value ().material.normalMapIndex,
                              crumb.renderData.value ().material.tilingFactor);
}
void renderScene (VulkanRenderer& renderer, Scene& scene) {
    for (Crumb* crumb : scene.getAllCrumbs ()) {
        renderCrumb(renderer, *crumb);
    }
}
int main () {
    if (!glfwInit ()) {
        std::cout << "Failed to initialize GLFW\n";
        return -1;
    }
    std::cout << "GLFW initialized!\n";

    glfwWindowHint (GLFW_CLIENT_API, GLFW_NO_API);

    uint32_t width  = 1600;
    uint32_t height = 900;
    GLFWwindow* window =
    glfwCreateWindow (width, height, "Vulkan Window", nullptr, nullptr);

    glm::mat4 view = glm::lookAt (glm::vec3 (0.0f, 0.0f, 2.0f), // camera position
                                  glm::vec3 (0.0f, 0.0f, 0.0f), // look at origin
                                  glm::vec3 (0.0f, 1.0f, 0.0f)  // up vector
    );

    VulkanRenderer renderer (window, width, height);

    Mesh tetrahedron = importMesh ("plant.fbx");
    renderer.initSceneData (view, { 0.0f, 3.0f, 10.0f }, { 1, 1, 1 });

    uint32_t teapotIndex = renderer.loadMesh (tetrahedron);
    glm::mat4 model = glm::translate (glm::mat4 (1.0f), glm::vec3 (0.3f, 0, -3.0f));

    Mesh quadMesh      = generateQuad ();
    uint32_t quadIndex = renderer.loadMesh (quadMesh);
    glm::mat4 quadModel =
    glm::scale (glm::translate (glm::mat4 (1.0f), { 0.0f, -1.0f, 0.0f }), glm::vec3 (30.0f));

    Mesh sphere          = generateSphere ();
    uint32_t sphereIndex = renderer.loadMesh (sphere);

    glm::mat4 sphereModel =
    glm::translate (glm::mat4 (1.0f), glm::vec3 (2.0f, 0, -3.0f));


    Mesh debugTextureQuad          = generateQuad ();
    uint32_t debugTextureQuadIndex = renderer.loadMesh (debugTextureQuad);
    glm::mat4 debugTextureQuadModel =
    glm::scale (glm::translate (glm::rotate (glm::mat4 (1.0f), 1.57f, { 1.0f, 0.0f, 0.0f }),
                                { 0.0f, -15.0f, -3.0f }),
                glm::vec3 (10.0f));


    float elapsedTime = 0;
    auto lastTime     = Clock::now ();

    // load all texture in the texture folder
    //  go through all files in the textures/ directory

    int dragonTexIndex   = renderer.loadTexture ("textures/grass.jpg");
    int rockwallTexIndex = renderer.loadTexture ("textures/rockwall.jpg");
    int brickTexIndex    = renderer.loadTexture ("textures/tiles.jpg");
    int stonewallNormalMapIndex =
    renderer.loadTexture ("textures/rock_normal_map.png");
    renderer.buildTextureAtlas ();


    Material m({1, 1, 1, 1}, 1, rockwallTexIndex, stonewallNormalMapIndex, {5, 3});
    RenderData sphereRenderData (sphereIndex, m);
    Crumb sphereObject (sphereRenderData);
    sphereObject.transform.position = { 2, 0, -3 };

    RenderData floorRenderData (quadIndex);
    Crumb floorObject (floorRenderData);
    floorObject.transform.rotate ({ 1, 0, 0}, 0.2);
    floorObject.transform.translate({0, -1, 0});
    floorObject.transform.scaleByFactor(glm::vec3(10));
    Scene scene;
    scene.addCrumb(sphereObject);
    scene.addCrumb(floorObject);

    while (!glfwWindowShouldClose (window)) {
        auto currentTime = Clock::now ();
        float deltaTime =
        std::chrono::duration<float> (currentTime - lastTime).count (); // in seconds
        lastTime = currentTime;
        elapsedTime += deltaTime;


        renderScene(renderer, scene);
        renderer.drawFrame ();
        glfwPollEvents ();
    }

    renderer.destroy ();
    glfwDestroyWindow (window);
    glfwTerminate ();
}