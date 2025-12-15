#pragma once

#include <GLFW/glfw3.h>
#include <set>

class InputManager {

    private:
    GLFWwindow* m_window;
    std::set<int> m_keysPressedThisFrame;
    std::set<int> m_keysReleasedThisFrame;
    std::set<int> m_currentlyPressedKeys;

    glm::dvec2 m_lastMousePos;
    glm::dvec2 m_mousePos; 

    void key_callback (GLFWwindow* window, int key, int scancode, int action, int mods) {
        if (action == GLFW_PRESS) {
            m_keysPressedThisFrame.insert (key);
            m_currentlyPressedKeys.insert (key);
        }
        if (action == GLFW_RELEASE) {
            m_keysReleasedThisFrame.insert (key);
            m_currentlyPressedKeys.erase (key);
        }
    }

    public:
    InputManager (GLFWwindow* window) : m_window (window) {
        // 1️⃣ Set the user pointer to this instance
        glfwSetWindowUserPointer(window, this);
        glfwSetKeyCallback (window, [] (GLFWwindow* w, int k, int sc, int a, int m) {
            // Recover your InputManager instance from the window user pointer
            InputManager* mgr =
            static_cast<InputManager*> (glfwGetWindowUserPointer (w));
            mgr->key_callback (w, k, sc, a, m);
        });
    }

    void newFrame () {
        m_keysPressedThisFrame.clear ();
        m_keysReleasedThisFrame.clear ();

        m_lastMousePos = m_mousePos;
        glfwGetCursorPos(m_window, &m_mousePos.x, &m_mousePos.y);
    }

    bool isPressed (int key) {
        return m_currentlyPressedKeys.count (key) > 0;
    }

    bool wasPressedThisFrame (int key) {
        return m_keysPressedThisFrame.count (key) > 0;
    }

    bool wasReleasedThisFrame (int key) {
        return m_keysReleasedThisFrame.count (key) > 0;
    }

    glm::vec2 getMousePos(){
        return m_mousePos;
    }

    glm::vec2 getMouseDisplacement(){
        return m_mousePos - m_lastMousePos;
    }
};