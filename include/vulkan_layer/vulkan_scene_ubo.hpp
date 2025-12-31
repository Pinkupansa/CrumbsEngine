#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Fonction pour calculer la matrice view de la lumière
glm::mat4 computeLightView(const glm::vec3& lightDir,
                           const glm::vec3& sceneCenter, float distance);

// UBO de scène pour Vulkan
struct VulkanSceneUBO {
  glm::mat4 view;      // caméra principale
  glm::mat4 proj;      // projection principale
  glm::mat4 lightView; // vue de la lumière
  glm::mat4 lightProj; // projection orthographique pour shadow map
  alignas(16) glm::vec3 lightColor; // alignement 16 bytes Vulkan
  alignas(16) glm::vec3 lightDir; // normalized light direction in world space
  alignas(16) glm::vec3 groundColor;
  alignas(16) glm::vec3 skyColor;
  // Constructeur par défaut
  VulkanSceneUBO();

  // Constructeur avec paramètres
  VulkanSceneUBO(const glm::mat4& camView, const glm::mat4& camProj,
                 const glm::vec3& lightDirParam, const glm::vec3& lightCol,
                 const glm::vec3& groundCol = {1.0f, 1.0f, 1.0f},
                 const glm::vec3& skyCol = {1.0f, 1.0f, 1.0f});
};