#pragma once 
#include <glm/glm.hpp>
#include "debug.hpp"
#include <string>
class Mesh{
    private:
        std::vector<glm::vec3> vertices; 
        std::vector<uint32_t> triangleIndices; 
        std::vector<glm::vec3> normals;
    
    public:
        
        const std::vector<glm::vec3>& getVertices() const{
            return vertices;
        }
        const std::vector<uint32_t>& getTriangles() const{
            return triangleIndices;
        }
        const std::vector<glm::vec3>& getNormals() const{
            return normals;
        }
        Mesh(std::vector<glm::vec3> _vertices, std::vector<uint32_t> _triangleIndices, std::vector<glm::vec3> _normals): vertices(_vertices), triangleIndices(_triangleIndices), normals(_normals){
            if(_normals.size() != _vertices.size()){
                Debug::LogWarning("Mesh : number of vertices (" + std::to_string (_vertices.size()) + ") does not match number of normals " + std::to_string(_normals.size()) + ") !");
            }
        }
};