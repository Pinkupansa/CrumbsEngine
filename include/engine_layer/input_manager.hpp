#pragma once

#include <GLFW/glfw3.h> 
#include <set>

class InputManager{

    private: 
        GLFWwindow* m_window;
        std::set<int> m_keysPressedThisFrame;
        std::set<int> m_keysReleasedThisFrame; 
        std::set<int> m_currentlyPressedKeys;
        
        void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
            if(action == GLFW_PRESS){
                m_keysPressedThisFrame.insert(key);
                m_currentlyPressedKeys.insert(key);
            }
            if(action == GLFW_RELEASE){
                m_keysReleasedThisFrame.insert(key);
                m_currentlyPressedKeys.erase(key);
            }
        }
    public: 
        InputManager(GLFWwindow* window): m_window(window){
            glfwSetKeyCallback(window, [](GLFWwindow* w, int k, int sc, int a, int m){
                // Recover your InputManager instance from the window user pointer
                InputManager* mgr = static_cast<InputManager*>(glfwGetWindowUserPointer(w));
                mgr->key_callback(w, k, sc, a, m);
            });
        }
        
        void newFrame(){
            m_keysPressedThisFrame.clear();
            m_keysReleasedThisFrame.clear();
        }

    
};