#pragma once

#include <vector>
#include "crumb.hpp"
#include "camera.hpp"
#include <optional>
class Scene{
    private: 
        std::vector<Crumb*> m_crumbs;
        Camera* m_mainCamera;

    public: 
        glm::vec3 groundColor; 
        glm::vec3 skyColor; 
        glm::vec3 lightDir; 
        glm::vec3 lightColor;

        const bool hasCamera() const{
            return m_mainCamera != nullptr;
        }
        const Camera* getMainCamera() const{
            return m_mainCamera;
        }
        void setCamera(Camera& camera){
            m_mainCamera = &camera;
        }
        void addCrumb(Crumb& crumb){
            m_crumbs.push_back(&crumb);
            crumb.onSceneAdd(this);
        }

        std::vector<Crumb*>& getAllCrumbs(){
            return m_crumbs;
        }
};