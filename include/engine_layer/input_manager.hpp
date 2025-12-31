#pragma once

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <set>

class InputManager {
 private:
  GLFWwindow* m_window;
  std::set<int> m_keysPressedThisFrame;
  std::set<int> m_keysReleasedThisFrame;
  std::set<int> m_currentlyPressedKeys;

  glm::dvec2 m_lastMousePos;
  glm::dvec2 m_mousePos;

  void key_callback(GLFWwindow* window, int key, int scancode, int action,
                    int mods);

 public:
  InputManager(GLFWwindow* window);

  void newFrame();

  bool isPressed(int key);

  bool wasPressedThisFrame(int key);

  bool wasReleasedThisFrame(int key);

  glm::vec2 getMousePos();

  glm::vec2 getMouseDisplacement();
};
