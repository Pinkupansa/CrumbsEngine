#define GLFW_INCLUDE_VULKAN

#include "crumb.hpp"
#include "primitive_meshes.hpp"
#include "scene.hpp"
#include "vulkan_renderer.hpp"
#include <GLFW/glfw3.h>
#include "input_manager.hpp"
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
    if (!scene.hasCamera ()) {
        Debug::LogWarning ("Scene has no camera !");
    }
    renderer.initSceneData (scene.getMainCamera ()->getView (),
                            { 0.0f, 3.0f, 10.0f }, { 1, 1, 1 });

    for (Crumb* crumb : scene.getAllCrumbs ()) {
        renderCrumb (renderer, *crumb);
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

    VulkanRenderer renderer (window, width, height);


    Mesh quadMesh      = generateQuad ();
    uint32_t quadIndex = renderer.loadMesh (quadMesh);

    Mesh sphere          = generateSphere ();
    uint32_t sphereIndex = renderer.loadMesh (sphere);

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


    Material m ({ 1, 1, 1, 1 }, 1, rockwallTexIndex, stonewallNormalMapIndex, { 5, 3 });
    RenderData sphereRenderData (sphereIndex, m);
    Crumb sphereObject (sphereRenderData);
    sphereObject.transform.position = { 2, 0, -3 };
    
    Material m2 ({ 1, 1, 1, 1 }, 1, brickTexIndex, -1, { 50, 50 });
    RenderData floorRenderData (quadIndex, m2);
    Crumb floorObject (floorRenderData);
    floorObject.transform.rotate ({ 1, 0, 0 }, 0.2);
    floorObject.transform.translate ({ 0, -1, 0 });
    floorObject.transform.scaleByFactor (glm::vec3 (10));


    Scene scene;
    scene.addCrumb (sphereObject);
    scene.addCrumb (floorObject);

    Camera cam (glm::vec3 (0.0f, 10.0f, -2.0f), // camera position
                glm::vec3 (0.0f, 0.0f, 0.0f));
    scene.setCamera (cam);


    InputManager inputs(window);
    while (!glfwWindowShouldClose (window)) {
        auto currentTime = Clock::now ();
        float deltaTime =
        std::chrono::duration<float> (currentTime - lastTime).count (); // in seconds
        lastTime = currentTime;
        elapsedTime += deltaTime;

        float speed = 3;

        if(inputs.isPressed(GLFW_KEY_W)){
            cam.transform.translate(speed*deltaTime*cam.transform.forward());
        }

        if(inputs.isPressed(GLFW_KEY_S)){
            cam.transform.translate(-speed*deltaTime*cam.transform.forward());
        }

        if(inputs.isPressed(GLFW_KEY_A)){
            cam.transform.translate(-speed*deltaTime*cam.transform.right());
        }

        if(inputs.isPressed(GLFW_KEY_D)){
            cam.transform.translate(speed*deltaTime*cam.transform.right());
        }

        cam.transform.rotate({0, 1, 0}, -inputs.getMouseDisplacement().x * deltaTime);
        cam.transform.rotate(cam.transform.right(), -inputs.getMouseDisplacement().y * deltaTime);
        renderScene (renderer, scene);
        renderer.drawFrame ();

        inputs.newFrame();
        glfwPollEvents ();
    }

    renderer.destroy ();
    glfwDestroyWindow (window);
    glfwTerminate ();
}