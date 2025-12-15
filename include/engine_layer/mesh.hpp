#pragma once
#include "debug.hpp"
#include <glm/glm.hpp>
#include <string>

class Mesh {
private:
    std::vector<glm::vec3> m_vertices;
    std::vector<uint32_t> m_triangleIndices;
    std::vector<glm::vec3> m_normals;
    std::vector<glm::vec2> m_uvs; 
    std::vector<glm::vec3> m_tangents;
    std::vector<glm::vec3> m_bitangents;

public:
    const std::vector<glm::vec3> &getVertices() const {
        return m_vertices;
    }

    const std::vector<uint32_t> &getTriangles() const {
        return m_triangleIndices;
    }

    const std::vector<glm::vec3> &getNormals() const {
        return m_normals;
    }

    const std::vector<glm::vec2> &getUVs() const {
        return m_uvs;
    }

    const std::vector<glm::vec3> &getTangents() const {
        return m_tangents;
    }

    const std::vector<glm::vec3> &getBitangents() const {
        return m_bitangents;
    }

    Mesh(std::vector<glm::vec3> _vertices, std::vector<uint32_t> _triangleIndices, std::vector<glm::vec3> _normals, std::vector<glm::vec2> _uvs) : m_vertices(_vertices), m_triangleIndices(_triangleIndices), m_normals(_normals), m_uvs(_uvs) {
        if (_normals.size() != _vertices.size()) {
            Debug::LogWarning("Mesh : number of vertices (" + std::to_string(_vertices.size()) + ") does not match number of normals " + std::to_string(_normals.size()) + ") !");
        }

        if (_uvs.size() != _vertices.size()) {
            Debug::LogWarning("Mesh : number of vertices (" + std::to_string(_vertices.size()) + ") does not match number of uvs " + std::to_string(_uvs.size()) + ") !");
        }

        // compute tangents and bitangents
        m_tangents.resize(m_vertices.size(), glm::vec3(0.0f));
        m_bitangents.resize(m_vertices.size(), glm::vec3(0.0f));

        for (size_t i = 0; i < m_triangleIndices.size(); i += 3) {
            uint32_t i0 = m_triangleIndices[i];
            uint32_t i1 = m_triangleIndices[i + 1];
            uint32_t i2 = m_triangleIndices[i + 2];

            glm::vec3 &v0 = m_vertices[i0];
            glm::vec3 &v1 = m_vertices[i1];
            glm::vec3 &v2 = m_vertices[i2];

            glm::vec2 &uv0 = m_uvs[i0];
            glm::vec2 &uv1 = m_uvs[i1];
            glm::vec2 &uv2 = m_uvs[i2];

            glm::vec3 deltaPos1 = v1 - v0;
            glm::vec3 deltaPos2 = v2 - v0;

            glm::vec2 deltaUV1 = uv1 - uv0;
            glm::vec2 deltaUV2 = uv2 - uv0;

            float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
            glm::vec3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
            glm::vec3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;

            m_tangents[i0] += tangent;
            m_tangents[i1] += tangent;
            m_tangents[i2] += tangent;

            m_bitangents[i0] += bitangent;
            m_bitangents[i1] += bitangent;
            m_bitangents[i2] += bitangent;
        }
    }
};