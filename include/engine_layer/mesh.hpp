#pragma once
#include "debug.hpp"
#include <glm/glm.hpp>
#include <string>

class Mesh {
private:
    std::vector<glm::vec3> vertices;
    std::vector<uint32_t> triangleIndices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs; 

public:
    const std::vector<glm::vec3> &getVertices() const {
        return vertices;
    }

    const std::vector<uint32_t> &getTriangles() const {
        return triangleIndices;
    }

    const std::vector<glm::vec3> &getNormals() const {
        return normals;
    }

    const std::vector<glm::vec2> &getUVs() const {
        return uvs;
    }

    Mesh(std::vector<glm::vec3> _vertices, std::vector<uint32_t> _triangleIndices, std::vector<glm::vec3> _normals, std::vector<glm::vec2> _uvs) : vertices(_vertices), triangleIndices(_triangleIndices), normals(_normals), uvs(_uvs) {
        if (_normals.size() != _vertices.size()) {
            Debug::LogWarning("Mesh : number of vertices (" + std::to_string(_vertices.size()) + ") does not match number of normals " + std::to_string(_normals.size()) + ") !");
        }

        if (_uvs.size() != _vertices.size()) {
            Debug::LogWarning("Mesh : number of vertices (" + std::to_string(_vertices.size()) + ") does not match number of uvs " + std::to_string(_uvs.size()) + ") !");
        }
    }
};