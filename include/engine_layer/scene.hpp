#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "crumb.hpp"
class Camera;
class Scene {
 private:
  std::vector<Crumb*> m_crumbs;
  Camera* m_mainCamera;

 public:
  glm::vec3 groundColor;
  glm::vec3 skyColor;
  glm::vec3 lightDir;
  glm::vec3 lightColor;

  const bool hasCamera() const;
  const Camera* getMainCamera() const;
  void setCamera(Camera& camera);
  void addCrumb(Crumb& crumb);

  std::vector<Crumb*>& getAllCrumbs();
};
