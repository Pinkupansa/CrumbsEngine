#pragma once
#include "transform.hpp"
class Camera{ 
    public:
        Transform transform; 
        Camera(){};
        Camera(Transform& transform): transform(transform){}
        Camera(glm::vec3 position, glm::vec3 target){
            transform.position = position;
            glm::vec3 forward = glm::normalize(target - position);
            glm::vec3 up      = glm::vec3(0, 1, 0);

            // Build camera basis in world space
            glm::vec3 right = glm::normalize(glm::cross(up, forward));
            glm::vec3 camUp = glm::cross(forward, right);
            
            
            // Camera-to-world rotation matrix
            glm::mat3 R;
            R[0] = right;
            R[1] = camUp;
            R[2] = forward;   // camera looks down -Z

            transform.rotation = glm::quat_cast(R);
        }
        const glm::mat4 getView() const{
            
            glm::mat3 R;
            R[0] = transform.right();
            R[1] = transform.up();
            R[2] = -transform.forward();
            //Debug::Log(std::to_string(transform.right().x) + " " + std::to_string(transform.right().y) + " " + std::to_string(transform.right().z));
            glm::quat flipped = glm::quat_cast(R);
            return glm::mat4_cast(glm::conjugate(flipped)) * glm::translate(glm::mat4(1.0f), -transform.position);
        }

        glm::mat4 getProj(){

        }
};