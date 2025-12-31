#pragma once
#include "transform.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

class Camera {
 public:
  Transform transform;
  Camera();
  Camera(Transform& transform);
  Camera(glm::vec3 position, glm::vec3 target);
  const glm::mat4 getView() const;
  glm::mat4 getProj();
};
