#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Fonction pour calculer la matrice view de la lumière
inline glm::mat4 computeLightView(const glm::vec3& lightDir,
                                 const glm::vec3& sceneCenter,
                                 float distance)
{
    glm::vec3 dir = glm::normalize(lightDir);
    // Position de la caméra de la lumière: place the light along the provided
    // direction from the scene center (direction is "to the light").
    glm::vec3 lightPos = sceneCenter + dir * distance;

    // Stable up vector (prefer +Y)
    glm::vec3 up(0.0f, -1.0f, 0.0f);
    if (glm::abs(glm::dot(up, dir)) > 0.99f)
        up = glm::vec3(1.0f, 0.0f, 0.0f);

    return glm::lookAt(lightPos, sceneCenter, up);
}

// UBO de scène pour Vulkan
struct VulkanSceneUBO {
    glm::mat4 view;         // caméra principale
    glm::mat4 proj;         // projection principale
    glm::mat4 lightView;    // vue de la lumière
    glm::mat4 lightProj;    // projection orthographique pour shadow map
    alignas(16) glm::vec3 lightColor; // alignement 16 bytes Vulkan
    alignas(16) glm::vec3 lightDir;   // normalized light direction in world space

    // Constructeur par défaut
    VulkanSceneUBO()
        : view(1.0f)
        , proj(1.0f)
        , lightView(1.0f)
        , lightProj(1.0f)
        , lightColor(0.0f, 0.0f, 0.0f)
    {}

    // Constructeur avec paramètres
    VulkanSceneUBO(const glm::mat4& camView,
             const glm::mat4& camProj,
             const glm::vec3& lightDirParam,
             const glm::vec3& lightCol,
             const glm::vec3& sceneCenter)
    {
        view = camView;
        proj = camProj;
        // store normalized light direction and compute a consistent light view
        glm::vec3 dir = glm::normalize(lightDirParam);
        lightDir = dir;
        lightView = computeLightView(dir, sceneCenter, 15.0f);

        // Projection orthographique Vulkan (z in [0,1])
        // Adjusted bounds to properly frame the scene
        lightProj = glm::orthoRH_ZO(-8.0f, 8.0f, -8.0f, 8.0f, 0.1f, 30.0f);

        lightColor = lightCol;
    }
};
