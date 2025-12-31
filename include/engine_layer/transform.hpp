#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Transform {
 public:
  glm::vec3 position;
  glm::quat rotation;
  glm::vec3 scale;

  Transform();
  glm::mat4 getModelMatrix();

  const glm::vec3 forward() const;
  const glm::vec3 right() const;
  const glm::vec3 up() const;
  void rotate(glm::vec3 axis, float angle);

  void translate(glm::vec3 translation);

  void scaleByFactor(glm::vec3 scaleFactor);

 private:
  glm::quat axisAngleToQuaternion(glm::vec3 axis, float angle);
};
