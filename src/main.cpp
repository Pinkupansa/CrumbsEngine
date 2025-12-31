#define GLFW_INCLUDE_VULKAN

#include "engine_layer/crumb.hpp"
#include "engine_layer/debug.hpp"
#include "engine_layer/input_manager.hpp"
#include "engine_layer/primitive_meshes.hpp"
#include "engine_layer/scene.hpp"
#include "vulkan_layer/vulkan_renderer.hpp"
#include "vulkan_layer/vulkan_shader_data.hpp"
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

    Mesh portalgunMesh          = loadOBJShared ("portalgun.obj");
    uint32_t portalGunMeshIndex = renderer.loadMesh (portalgunMesh);

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
    int grassBillboardIndex =
    renderer.loadTexture ("textures/grass_billboard.png");
    int grassIndex       = renderer.loadTexture ("textures/portalfloor.png");
    int grassNormalIndex = renderer.loadTexture ("textures/grass3_normal.jpg");
    int rockwallTexIndex =
    renderer.loadTexture ("textures/cottage_diffuse.png");

    int portalgunTexIndex = renderer.loadTexture ("textures/portalgun_col.jpg");
    int portalgunNormalIndex =
    renderer.loadTexture ("textures/portalgun_nor.jpg");

    int stonewallNormalMapIndex =
    renderer.loadTexture ("textures/cottage_normal.png");

    int skyboxTexture = renderer.loadTexture ("textures/sky2.jpg");
    int dragonTexture =
    renderer.loadTexture ("textures/Dragon_ground_color.jpg");


    renderer.buildTextureAtlas ();


    VulkanRenderTexture* renderTex =
    renderer.createRenderTexture ("TransparencyRenderTexture");
    VulkanRenderTexture* screenRenderTex =
    renderer.createRenderTexture ("ScreenRenderTexture");
    VulkanShaderData shader1Data ("shaders/lit.frag", true, true, VulkanAlphaBlendMode::None);

    VulkanShaderData shader2Data ("shaders/unlit.frag", true, true,
                                  VulkanAlphaBlendMode::None);

    shader1Data.bindTargetRenderTexture (*screenRenderTex);
    shader2Data.bindTargetRenderTexture (*screenRenderTex);


    VulkanShaderData transparencyShader ("shaders/transparent.frag", true,
                                         false, VulkanAlphaBlendMode::Additive);
    transparencyShader.bindTargetRenderTexture (*renderTex);

    VulkanShaderData transparencyResolveShader (
    "shaders/transparency_resolve.frag", false, false, VulkanAlphaBlendMode::Weighted,
    true, nullptr, nullptr, nullptr, VK_CULL_MODE_NONE);
    transparencyResolveShader.bindSourceRenderTexture (*renderTex);
    transparencyResolveShader.bindTargetRenderTexture (*screenRenderTex);
    
    VulkanShaderData postprocessingShader("shaders/postprocessing.frag", false, false, VulkanAlphaBlendMode::None, true, nullptr, nullptr, nullptr, VK_CULL_MODE_NONE);
    postprocessingShader.bindSourceRenderTexture (*screenRenderTex);


    int litShaderIndex         = renderer.loadShader (shader1Data);
    int unlitShaderIndex       = renderer.loadShader (shader2Data);
    int transparentShaderIndex = renderer.loadShader (transparencyShader);
    int transparentResolveShaderIndex = renderer.loadShader (transparencyResolveShader);
    int postprocessingShaderIndex = renderer.loadShader (postprocessingShader);

    

    Mesh windowMesh     = generateQuad ();
    int windowMeshIndex = renderer.loadMesh (windowMesh);

    Material mWindow (transparentShaderIndex);
    mWindow.setProperty ("tint", glm::vec4 (1.0f, 1.0f, 1.0f, 1.0f));
    mWindow.setProperty ("atlasOffset", renderer.getTextureAtlasOffset (grassBillboardIndex));
    mWindow.setProperty ("relativeTextureSize",
                         renderer.getRelativeTextureSize (grassBillboardIndex));
    mWindow.setProperty ("tilingFactor", glm::vec2 (1.0f));
    RenderData windowRenderData (windowMeshIndex, mWindow, false);
    Crumb windowObj (windowRenderData);
    windowObj.transform.position = { 0, 0.2f, 0 };


    Material mWindow2 (transparentShaderIndex);
    mWindow2.setProperty ("tint", glm::vec4 (1.0f, 0.0f, 0.0f, 0.2f));
    RenderData windowRenderData2 (windowMeshIndex, mWindow2, false);
    Crumb windowObj2 (windowRenderData2);
    windowObj2.transform.position = { 0.2f, 0.5f, 0 };


    Material mPortal (litShaderIndex);
    mPortal.setProperty ("atlasOffset", renderer.getTextureAtlasOffset (portalgunTexIndex));
    mPortal.setProperty ("relativeTextureSize",
                         renderer.getRelativeTextureSize (portalgunTexIndex));
    mPortal.setProperty ("normalmapAtlasOffset",
                         renderer.getRelativeTextureSize (portalgunNormalIndex));
    mPortal.setProperty ("normalmapRelativeTextureSize",
                         renderer.getRelativeTextureSize (portalgunNormalIndex));
    mPortal.setProperty ("tilingFactor", glm::vec2 (1.0f));
    mPortal.setProperty ("specularity", 1.0f);
    RenderData portalGunRenderData (portalGunMeshIndex, mPortal);
    Crumb portalGun (portalGunRenderData);
    portalGun.transform.position = { 0, 0.1f, 0 };
    portalGun.transform.scaleByFactor (glm::vec3 (0.5f));


    Material m (litShaderIndex);
    m.setProperty ("atlasOffset", renderer.getTextureAtlasOffset (rockwallTexIndex));
    m.setProperty ("relativeTextureSize", renderer.getRelativeTextureSize (rockwallTexIndex));
    m.setProperty ("normalmapAtlasOffset",
                   renderer.getTextureAtlasOffset (stonewallNormalMapIndex));
    m.setProperty ("normalmapRelativeTextureSize",
                   renderer.getRelativeTextureSize (stonewallNormalMapIndex));
    m.setProperty ("tilingFactor", glm::vec2 (1.0f));
    RenderData sphereRenderData (sphereIndex, m);
    Crumb sphereObject (sphereRenderData);
    sphereObject.transform.position = { 2, -0.5f, -3 };
    sphereObject.transform.rotate ({ 1, 0, 0 }, -1.57);
    sphereObject.transform.scaleByFactor ({ 1, 1, 0.7f });

    Material m2 (litShaderIndex);
    m2.setProperty ("atlasOffset", renderer.getTextureAtlasOffset (grassIndex));
    m2.setProperty ("relativeTextureSize",
                    renderer.getRelativeTextureSize (grassIndex));
    m2.setProperty ("tilingFactor", glm::vec2 (10.0f));

    RenderData floorRenderData (quadIndex, m2);
    Crumb floorObject (floorRenderData);
    floorObject.transform.rotate ({ 1, 0, 0 }, 0);
    floorObject.transform.translate ({ 0, -1, 0 });
    floorObject.transform.scaleByFactor (glm::vec3 (10));


    Mesh skybox     = generateInvertedSphere ();
    int skyboxIndex = renderer.loadMesh (skybox);
    Material m3 (unlitShaderIndex);
    m3.setProperty ("atlasOffset", renderer.getTextureAtlasOffset (skyboxTexture));
    m3.setProperty ("relativeTextureSize", renderer.getRelativeTextureSize (skyboxTexture));
    m3.setProperty ("tilingFactor", glm::vec2 (1.0f));

    RenderData skyRenderData (skyboxIndex, m3, false);
    Crumb skyboxObject (skyRenderData);
    skyboxObject.transform.scaleByFactor (glm::vec3 (400.0f));
    skyboxObject.transform.rotate ({ 1, 0, 0 }, 3.14);

    Material mDragon (litShaderIndex);
    mDragon.setProperty ("atlasOffset", renderer.getTextureAtlasOffset (dragonTexture));
    mDragon.setProperty ("relativeTextureSize",
                         renderer.getRelativeTextureSize (dragonTexture));
    mDragon.setProperty ("tilingFactor", glm::vec2 (1.0f));

    /*RenderData dragonRenderData (dragonIndex, mDragon);
    Crumb dragonObject (dragonRenderData);
    dragonObject.transform.position = { 0, 2, 0 };
    dragonObject.transform.scaleByFactor (glm::vec3 (0.2f));
    dragonObject.transform.rotate ({ 1, 0, 0 }, 1.57);
    dragonObject.transform.rotate ({ 0, 1, 0 }, 3.14);
    dragonObject.transform.rotate ({ 0, 0, 1 }, 3.14);*/


    Scene scene;
    scene.addCrumb (sphereObject);
    scene.addCrumb (floorObject);
    scene.addCrumb (skyboxObject);
    // scene.addCrumb (dragonObject);
    scene.addCrumb (portalGun);
    scene.addCrumb (windowObj);
    scene.addCrumb (windowObj2);

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
        // Debug::Log (std::to_string (1 / deltaTime));
        float speed = 4;

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