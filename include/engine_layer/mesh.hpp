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
    std::vector<glm::vec3> tangents;
    std::vector<glm::vec3> bitangents;

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

    const std::vector<glm::vec3> &getTangents() const {
        return tangents;
    }

    const std::vector<glm::vec3> &getBitangents() const {
        return bitangents;
    }

    Mesh(std::vector<glm::vec3> _vertices, std::vector<uint32_t> _triangleIndices, std::vector<glm::vec3> _normals, std::vector<glm::vec2> _uvs) : vertices(_vertices), triangleIndices(_triangleIndices), normals(_normals), uvs(_uvs) {
        if (_normals.size() != _vertices.size()) {
            Debug::LogWarning("Mesh : number of vertices (" + std::to_string(_vertices.size()) + ") does not match number of normals " + std::to_string(_normals.size()) + ") !");
        }

        if (_uvs.size() != _vertices.size()) {
            Debug::LogWarning("Mesh : number of vertices (" + std::to_string(_vertices.size()) + ") does not match number of uvs " + std::to_string(_uvs.size()) + ") !");
        }

        // compute tangents and bitangents
        tangents.resize(vertices.size(), glm::vec3(0.0f));
        bitangents.resize(vertices.size(), glm::vec3(0.0f));

        for (size_t i = 0; i < triangleIndices.size(); i += 3) {
            uint32_t i0 = triangleIndices[i];
            uint32_t i1 = triangleIndices[i + 1];
            uint32_t i2 = triangleIndices[i + 2];

            glm::vec3 &v0 = vertices[i0];
            glm::vec3 &v1 = vertices[i1];
            glm::vec3 &v2 = vertices[i2];

            glm::vec2 &uv0 = uvs[i0];
            glm::vec2 &uv1 = uvs[i1];
            glm::vec2 &uv2 = uvs[i2];

            glm::vec3 deltaPos1 = v1 - v0;
            glm::vec3 deltaPos2 = v2 - v0;

            glm::vec2 deltaUV1 = uv1 - uv0;
            glm::vec2 deltaUV2 = uv2 - uv0;

            float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
            glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
            glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;

            tangents[i0] += tangent;
            tangents[i1] += tangent;
            tangents[i2] += tangent;

            bitangents[i0] += bitangent;
            bitangents[i1] += bitangent;
            bitangents[i2] += bitangent;
        }
    }
};