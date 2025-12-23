#define GLFW_INCLUDE_VULKAN

#include "crumb.hpp"
#include "input_manager.hpp"
#include "primitive_meshes.hpp"
#include "scene.hpp"
#include "vulkan_renderer.hpp"
#include <GLFW/glfw3.h>
using Clock = std::chrono::high_resolution_clock;


void renderCrumb (VulkanRenderer& renderer, Crumb& crumb) {
    if (!crumb.renderData.has_value ()) {
        return;
    }


    renderer.addMeshDrawCall (crumb.renderData.value ().material.shaderIndex,
                              crumb.renderData.value ().material.materialProperties,
                              crumb.renderData.value ().meshIndex,
                              crumb.transform.getModelMatrix (),
                              crumb.renderData.value ().castsShadows);
}
void renderScene (VulkanRenderer& renderer, Scene& scene) {
    if (!scene.hasCamera ()) {
        Debug::LogWarning ("Scene has no camera !");
    }
    renderer.initSceneData (scene);

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

    Mesh sphere          = importMesh ("cottage_fbx.fbx");
    uint32_t sphereIndex = renderer.loadMesh (sphere);

    Mesh debugTextureQuad          = generateQuad ();
    uint32_t debugTextureQuadIndex = renderer.loadMesh (debugTextureQuad);
    glm::mat4 debugTextureQuadModel =
    glm::scale (glm::translate (glm::rotate (glm::mat4 (1.0f), 1.57f, { 1.0f, 0.0f, 0.0f }),
                                { 0.0f, -15.0f, -3.0f }),
                glm::vec3 (10.0f));


    Mesh dragon          = importMesh ("dragon.fbx");
    uint32_t dragonIndex = renderer.loadMesh (dragon);


    float elapsedTime = 0;
    auto lastTime     = Clock::now ();

    // load all texture in the texture folder
    //  go through all files in the textures/ directory

    int grassIndex       = renderer.loadTexture ("textures/grass3.jpg");
    int grassNormalIndex = renderer.loadTexture ("textures/grass3_normal.jpg");
    int rockwallTexIndex =
    renderer.loadTexture ("textures/cottage_diffuse.png");
    int brickTexIndex = renderer.loadTexture ("textures/rockwall.jpg");
    int stonewallNormalMapIndex =
    renderer.loadTexture ("textures/cottage_normal.png");

    int skyboxTexture = renderer.loadTexture ("textures/sky2.jpg");
    int dragonTexture =
    renderer.loadTexture ("textures/Dragon_ground_color.jpg");
    renderer.buildTextureAtlas ();

    int testShaderInd = renderer.loadShader ("shaders/test.frag", VK_COMPARE_OP_LESS);
    int testShader2Ind = renderer.loadShader ("shaders/test2.frag", VK_COMPARE_OP_LESS);


    Material m (testShaderInd);
    // m.setProperty("test", 0.5f);
    // m.setProperty("test2", 0.5f);
    RenderData sphereRenderData (sphereIndex, m);
    Crumb sphereObject (sphereRenderData);
    sphereObject.transform.position = { 2, -0.5f, -3 };
    sphereObject.transform.rotate ({ 1, 0, 0 }, -1.57);
    sphereObject.transform.scaleByFactor ({ 1, 1, 0.7f });

    Material m2 (testShader2Ind);
    RenderData floorRenderData (quadIndex, m2);
    Crumb floorObject (floorRenderData);
    floorObject.transform.rotate ({ 1, 0, 0 }, 0);
    floorObject.transform.translate ({ 0, -1, 0 });
    floorObject.transform.scaleByFactor (glm::vec3 (10));


    Mesh skybox     = generateInvertedSphere ();
    int skyboxIndex = renderer.loadMesh (skybox);
    Material m3 (testShader2Ind);
    RenderData skyRenderData (skyboxIndex, m3);
    Crumb skyboxObject (skyRenderData);
    skyboxObject.transform.scaleByFactor (glm::vec3 (100.0f));
    skyboxObject.transform.rotate ({ 1, 0, 0 }, 3.14);

    Material mDragon (testShader2Ind);
    RenderData dragonRenderData (dragonIndex, mDragon);
    Crumb dragonObject (dragonRenderData);
    dragonObject.transform.position = { 0, 2, 0 };
    dragonObject.transform.scaleByFactor (glm::vec3 (0.2f));
    dragonObject.transform.rotate ({ 1, 0, 0 }, 1.57);
    dragonObject.transform.rotate ({ 0, 1, 0 }, 3.14);
    dragonObject.transform.rotate ({ 0, 0, 1 }, 3.14);


    Scene scene;
    scene.addCrumb (sphereObject);
    scene.addCrumb (floorObject);
    // scene.addCrumb (skyboxObject);
    scene.addCrumb (dragonObject);
    Camera cam ({ 0, 0, 0 }, { 0, 0, 1 });
    scene.setCamera (cam);

    scene.groundColor = glm::vec3 (86, 156, 12) / 255.0f;
    scene.skyColor    = glm::vec3 (43, 168, 240) / 255.0f;
    scene.lightColor  = glm::vec3 (1.0f, 1.0f, 1.0f);
    scene.lightDir    = { 0.2f, 1.0f, 0.2f };

    InputManager inputs (window);
    while (!glfwWindowShouldClose (window)) {
        auto currentTime = Clock::now ();
        float deltaTime =
        std::chrono::duration<float> (currentTime - lastTime).count (); // in seconds
        lastTime = currentTime;
        elapsedTime += deltaTime;
        // Debug::Log(std::to_string(1/deltaTime));
        float speed = 3;

        if (inputs.isPressed (GLFW_KEY_W)) {
            cam.transform.translate (speed * deltaTime * cam.transform.forward ());
        }

        if (inputs.isPressed (GLFW_KEY_S)) {
            cam.transform.translate (-speed * deltaTime * cam.transform.forward ());
        }

        if (inputs.isPressed (GLFW_KEY_A)) {
            cam.transform.translate (-speed * deltaTime * cam.transform.right ());
        }

        if (inputs.isPressed (GLFW_KEY_D)) {
            cam.transform.translate (speed * deltaTime * cam.transform.right ());
        }

        cam.transform.rotate ({ 0, 1, 0 }, -inputs.getMouseDisplacement ().x * deltaTime);
        cam.transform.rotate (cam.transform.right (),
                              -inputs.getMouseDisplacement ().y * deltaTime);
        renderScene (renderer, scene);
        renderer.drawFrame ();

        inputs.newFrame ();
        glfwPollEvents ();
    }

    renderer.destroy ();
    glfwDestroyWindow (window);
    glfwTerminate ();
}