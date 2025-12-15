#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include "debug.hpp"
class Transform {
    public:
    glm::vec3 position;
    glm::quat rotation;
    glm::vec3 scale;

    Transform(): position(glm::vec3(0.0f)), rotation(glm::quat({1, 0, 0, 0})), scale(glm::vec3(1.0f)){}
    glm::mat4 getModelMatrix () {
        return glm::translate (glm::mat4 (1.0f), position) *
        glm::mat4_cast (rotation) * glm::scale (glm::mat4 (1.0f), scale);
    }

    const glm::vec3 forward() const{
        return rotation * glm::vec3(0,0,1);
    }
    const glm::vec3 right() const{
        return rotation * glm::vec3(-1,0,0);
    }
    const glm::vec3 up() const{
        return rotation * glm::vec3(0,1,0);
    }
    void rotate(glm::vec3 axis, float angle){
        rotation = axisAngleToQuaternion(axis, angle) * rotation;
        rotation = glm::normalize(rotation);
    }

    void translate(glm::vec3 translation){
        position += translation; 
    }
    
    void scaleByFactor(glm::vec3 scaleFactor){
        scale = glm::vec3(scale.x * scaleFactor.x, scale.y * scaleFactor.y, scale.z * scaleFactor.z);
    }

    private: 
        glm::quat axisAngleToQuaternion(glm::vec3 axis, float angle){
            glm::vec3 normalizedAxis = glm::normalize(axis);
            float half = angle*0.5f;
            return glm::quat(cos(half), normalizedAxis.x*sin(half), normalizedAxis.y*sin(half), normalizedAxis.z*sin(half));
        }
};