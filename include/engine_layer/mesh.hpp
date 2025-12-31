#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>

class Mesh {
 private:
  std::vector<glm::vec3> m_vertices;
  std::vector<uint32_t> m_triangleIndices;
  std::vector<glm::vec3> m_normals;
  std::vector<glm::vec2> m_uvs;
  std::vector<glm::vec3> m_tangents;
  std::vector<glm::vec3> m_bitangents;

 public:
  const std::vector<glm::vec3>& getVertices() const;

  const std::vector<uint32_t>& getTriangles() const;

  const std::vector<glm::vec3>& getNormals() const;

  const std::vector<glm::vec2>& getUVs() const;

  const std::vector<glm::vec3>& getTangents() const;

  const std::vector<glm::vec3>& getBitangents() const;

  Mesh(std::vector<glm::vec3> _vertices, std::vector<uint32_t> _triangleIndices,
       std::vector<glm::vec3> _normals, std::vector<glm::vec2> _uvs);
};
