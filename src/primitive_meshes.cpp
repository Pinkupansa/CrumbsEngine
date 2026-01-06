#include "engine_layer/primitive_meshes.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include "engine_layer/debug.hpp"
#include "engine_layer/tiny_obj_loader.h"

#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

Mesh generateTetrahedron() {
  // Vertices of a regular tetrahedron centered at the origin
  std::vector<glm::vec3> vertices = {
      {1.0f, 1.0f, 1.0f},    // 0
      {-1.0f, -1.0f, 1.0f},  // 1
      {-1.0f, 1.0f, -1.0f},  // 2
      {1.0f, -1.0f, -1.0f}   // 3
  };

  // Indices for tetrahedron faces
  std::vector<uint32_t> indices = {2, 1, 0, 1, 3, 0, 3, 2, 0, 2, 3, 1};

  // Compute face normals
  std::vector<glm::vec3> faceNormals;
  for (size_t i = 0; i < indices.size(); i += 3) {
    glm::vec3 a = vertices[indices[i]];
    glm::vec3 b = vertices[indices[i + 1]];
    glm::vec3 c = vertices[indices[i + 2]];

    glm::vec3 normal = glm::normalize(glm::cross(b - a, c - a));
    faceNormals.push_back(normal);
  }

  // Initialize vertex normals
  std::vector<glm::vec3> vertexNormals(vertices.size(), glm::vec3(0.0f));

  // Accumulate face normals into each vertex
  for (size_t f = 0; f < faceNormals.size(); ++f) {
    uint32_t i0 = indices[3 * f];
    uint32_t i1 = indices[3 * f + 1];
    uint32_t i2 = indices[3 * f + 2];
    vertexNormals[i0] += faceNormals[f];
    vertexNormals[i1] += faceNormals[f];
    vertexNormals[i2] += faceNormals[f];
  }

  // Normalize vertex normals
  for (auto& n : vertexNormals) n = glm::normalize(n);

  // simple spherical UVs
  std::vector<glm::vec2> uvs;
  uvs.reserve(vertices.size());
  for (auto& v : vertices) {
    float u = 0.5f + atan2(v.z, v.x) / (2 * M_PI);
    float vcoord = 0.5f - asin(v.y) / M_PI;
    uvs.emplace_back(u, vcoord);
  }
  return Mesh(vertices, indices, vertexNormals, uvs);
}

Mesh generateSphere() {
  const uint32_t X_SEGMENTS = 32;
  const uint32_t Y_SEGMENTS = 16;

  std::vector<glm::vec3> vertices;
  std::vector<glm::vec3> normals;
  std::vector<uint32_t> indices;
  std::vector<glm::vec2> uvs;

  for (uint32_t y = 0; y <= Y_SEGMENTS; ++y) {
    for (uint32_t x = 0; x <= X_SEGMENTS; ++x) {
      float xs = float(x) / X_SEGMENTS;
      float ys = float(y) / Y_SEGMENTS;

      float xPos = cos(xs * 2 * M_PI) * sin(ys * M_PI);
      float yPos = cos(ys * M_PI);
      float zPos = sin(xs * 2 * M_PI) * sin(ys * M_PI);

      glm::vec3 p(xPos, yPos, zPos);

      vertices.push_back(p);
      normals.push_back(glm::normalize(p));
      uvs.emplace_back(xs, ys);
    }
  }

  for (uint32_t y = 0; y < Y_SEGMENTS; ++y) {
    for (uint32_t x = 0; x < X_SEGMENTS; ++x) {
      uint32_t i0 = y * (X_SEGMENTS + 1) + x;
      uint32_t i1 = (y + 1) * (X_SEGMENTS + 1) + x;
      uint32_t i2 = i1 + 1;
      uint32_t i3 = i0 + 1;

      indices.insert(indices.end(), {i2, i1, i0, i3, i2, i0});
    }
  }

  return Mesh(vertices, indices, normals, uvs);
}

Mesh generateInvertedSphere() {
  const uint32_t X_SEGMENTS = 32;
  const uint32_t Y_SEGMENTS = 16;

  std::vector<glm::vec3> vertices;
  std::vector<glm::vec3> normals;
  std::vector<uint32_t> indices;
  std::vector<glm::vec2> uvs;

  for (uint32_t y = 0; y <= Y_SEGMENTS; ++y) {
    for (uint32_t x = 0; x <= X_SEGMENTS; ++x) {
      float xs = float(x) / X_SEGMENTS;
      float ys = float(y) / Y_SEGMENTS;

      float xPos = cos(xs * 2 * M_PI) * sin(ys * M_PI);
      float yPos = cos(ys * M_PI);
      float zPos = sin(xs * 2 * M_PI) * sin(ys * M_PI);

      glm::vec3 p(xPos, yPos, zPos);

      vertices.push_back(p);
      normals.push_back(glm::normalize(-p));
      uvs.emplace_back(xs, ys);
    }
  }

  for (uint32_t y = 0; y < Y_SEGMENTS; ++y) {
    for (uint32_t x = 0; x < X_SEGMENTS; ++x) {
      uint32_t i0 = y * (X_SEGMENTS + 1) + x;
      uint32_t i1 = (y + 1) * (X_SEGMENTS + 1) + x;
      uint32_t i2 = i1 + 1;
      uint32_t i3 = i0 + 1;

      indices.insert(indices.end(), {i0, i1, i2, i0, i2, i3});
    }
  }

  return Mesh(vertices, indices, normals, uvs);
}

Mesh importMesh(std::string meshPath) {
  Assimp::Importer importer;
  const aiScene* scene = importer.ReadFile(
      meshPath, aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                    aiProcess_JoinIdenticalVertices);

  if (!scene || !scene->HasMeshes())
    throw std::runtime_error("Couldn't load mesh " + meshPath);

  std::vector<glm::vec3> vertices;
  std::vector<glm::vec3> normals;
  std::vector<uint32_t> indices;
  std::vector<glm::vec2> uvs;

  Debug::Log("Importing mesh from " + meshPath + " with " +
             std::to_string(scene->mNumMeshes) + " meshes.");
  for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
    aiMesh* mesh = scene->mMeshes[m];
    int baseVertexIndex = vertices.size();
    for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
      aiVector3D pos = mesh->mVertices[v];
      vertices.emplace_back(pos.x, pos.y, pos.z);

      aiVector3D norm =
          mesh->HasNormals() ? mesh->mNormals[v] : aiVector3D(0, 1, 0);
      normals.emplace_back(norm.x, norm.y, norm.z);

      if (mesh->HasTextureCoords(0)) {
        aiVector3D uv = mesh->mTextureCoords[0][v];
        uvs.emplace_back(uv.x, uv.y);
      } else {
        uvs.emplace_back(0.0f, 0.0f);
      }
    }

    for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
      aiFace face = mesh->mFaces[f];
      for (unsigned int i = 0; i < face.mNumIndices; ++i)
        indices.push_back(face.mIndices[i] + baseVertexIndex);
    }
  }

  return Mesh(vertices, indices, normals, uvs);
}
Mesh loadOBJ(const std::string& path) {
  std::vector<glm::vec3> vertices;
  std::vector<glm::vec3> normals;
  std::vector<glm::vec2> uvs;
  std::vector<uint32_t> indices;

  std::vector<glm::vec2> temp_uvs;
  std::vector<glm::vec3> temp_norms;

  std::ifstream file(path);
  if (!file.is_open()) throw std::runtime_error("Couldn't load mesh " + path);

  std::string line;
  while (std::getline(file, line)) {
    std::stringstream ss(line);
    std::string prefix;
    ss >> prefix;

    if (prefix == "v") {
      glm::vec3 v;
      ss >> v.x >> v.y >> v.z;
      vertices.push_back(v);
    } else if (prefix == "vt") {
      glm::vec2 uv;
      ss >> uv.x >> uv.y;
      temp_uvs.push_back(uv);
    } else if (prefix == "vn") {
      glm::vec3 n;
      ss >> n.x >> n.y >> n.z;
      temp_norms.push_back(n);
    } else if (prefix == "f") {
      for (int i = 0; i < 3; ++i) {
        std::string vertStr;
        ss >> vertStr;
        std::replace(vertStr.begin(), vertStr.end(), '/', ' ');
        std::stringstream vss(vertStr);

        int vi, ti, ni;
        vss >> vi >> ti >> ni;

        indices.push_back(vi - 1);
        uvs.push_back(temp_uvs[ti - 1]);
        normals.push_back(temp_norms[ni - 1]);
      }
    }
  }

  return Mesh(vertices, indices, normals, uvs);
}

Mesh generateQuad() {
  std::vector<glm::vec3> vertices = {
      {1, 0, -1}, {-1, 0, -1}, {-1, 0, 1}, {1, 0, 1}};

  std::vector<uint32_t> triangles = {0, 1, 3, 1, 2, 3};

  std::vector<glm::vec3> normals = {
      {0, 1, 0}, {0, 1, 0}, {0, 1, 0}, {0, 1, 0}};

  std::vector<glm::vec2> uvs = {{1, 0}, {0, 0}, {0, 1}, {1, 1}};

  return Mesh(vertices, triangles, normals, uvs);
}
Mesh generateInvertedCube() {
  // Cube corners
  std::vector<glm::vec3> vertices = {
      {-1.0f, -1.0f, -1.0f},  // 0
      {1.0f, -1.0f, -1.0f},   // 1
      {1.0f, 1.0f, -1.0f},    // 2
      {-1.0f, 1.0f, -1.0f},   // 3
      {-1.0f, -1.0f, 1.0f},   // 4
      {1.0f, -1.0f, 1.0f},    // 5
      {1.0f, 1.0f, 1.0f},     // 6
      {-1.0f, 1.0f, 1.0f}     // 7
  };

  // Normals pointing inward
  std::vector<glm::vec3> normals = {
      {-1, -1, -1}, {1, -1, -1}, {1, 1, -1},  {-1, 1, -1},
      {-1, -1, 1},  {1, -1, 1},  {1, 1, 1},   {-1, 1, 1}};
  for (auto& n : normals)
    n = -glm::normalize(n);  // invert normals

  std::vector<glm::vec2> uvs = {{0, 0}, {1, 0}, {1, 1}, {0, 1},
                                {0, 0}, {1, 0}, {1, 1}, {0, 1}};

  std::vector<uint32_t> indices;

  // Each face (2 triangles) - winding order reversed for inward
  // Front (-Z)
  indices.insert(indices.end(), {0, 1, 2, 2, 3, 0});
  // Back (+Z)
  indices.insert(indices.end(), {5, 4, 7, 7, 6, 5});
  // Left (-X)
  indices.insert(indices.end(), {4, 0, 3, 3, 7, 4});
  // Right (+X)
  indices.insert(indices.end(), {1, 5, 6, 6, 2, 1});
  // Top (+Y)
  indices.insert(indices.end(), {3, 2, 6, 6, 7, 3});
  // Bottom (-Y)
  indices.insert(indices.end(), {4, 5, 1, 1, 0, 4});

  return Mesh(vertices, indices, normals, uvs);
}

struct LoadObjVertex {
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec2 uv;

  bool operator==(const LoadObjVertex& other) const {
    return pos == other.pos && normal == other.normal && uv == other.uv;
  }
};

// Hash function for Vertex to use in unordered_map
namespace std {
template <>
struct hash<LoadObjVertex> {
  size_t operator()(const LoadObjVertex& v) const {
    size_t h1 = hash<float>()(v.pos.x) ^ hash<float>()(v.pos.y) << 1 ^
                hash<float>()(v.pos.z) << 2;
    size_t h2 = hash<float>()(v.normal.x) ^ hash<float>()(v.normal.y) << 1 ^
                hash<float>()(v.normal.z) << 2;
    size_t h3 = hash<float>()(v.uv.x) ^ hash<float>()(v.uv.y) << 1;
    return h1 ^ h2 ^ h3;
  }
};
}  // namespace std

Mesh loadOBJShared(const std::string& filename) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;
  std::string warn, err;

  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                        filename.c_str())) {
    throw std::runtime_error("Failed to load OBJ: " + warn + err);
  }

  std::vector<glm::vec3> vertices;
  std::vector<glm::vec3> normals;
  std::vector<glm::vec2> uvs;
  std::vector<uint32_t> indices;

  std::unordered_map<LoadObjVertex, uint32_t> uniqueVertices;

  for (const auto& shape : shapes) {
    size_t indexOffset = 0;
    for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); ++f) {
      int fv = shape.mesh.num_face_vertices[f];
      for (int v = 0; v < fv; ++v) {
        tinyobj::index_t idx = shape.mesh.indices[indexOffset + v];

        LoadObjVertex vertex{};
        vertex.pos = {attrib.vertices[3 * idx.vertex_index + 0],
                      attrib.vertices[3 * idx.vertex_index + 1],
                      attrib.vertices[3 * idx.vertex_index + 2]};

        vertex.normal = idx.normal_index >= 0
                            ? glm::vec3(attrib.normals[3 * idx.normal_index + 0],
                                        attrib.normals[3 * idx.normal_index + 1],
                                        attrib.normals[3 * idx.normal_index + 2])
                            : glm::vec3(0.0f);

        vertex.uv = idx.texcoord_index >= 0
                        ? glm::vec2(attrib.texcoords[2 * idx.texcoord_index + 0],
                                    attrib.texcoords[2 * idx.texcoord_index + 1])
                        : glm::vec2(0.0f);

        // Check if vertex already exists
        if (uniqueVertices.count(vertex) == 0) {
          uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
          vertices.push_back(vertex.pos);
          normals.push_back(vertex.normal);
          uvs.push_back(vertex.uv);
        }

        indices.push_back(uniqueVertices[vertex]);
      }
      indexOffset += fv;
    }
  }
  std::vector<glm::vec3> tempNormals(vertices.size(), glm::vec3(0.0f));

  // Loop over triangles
  for (size_t i = 0; i < indices.size(); i += 3) {
    uint32_t i0 = indices[i];
    uint32_t i1 = indices[i + 1];
    uint32_t i2 = indices[i + 2];

    glm::vec3 v0 = vertices[i0];
    glm::vec3 v1 = vertices[i1];
    glm::vec3 v2 = vertices[i2];

    // Compute face normal
    glm::vec3 faceNormal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

    // Accumulate to vertex normals
    tempNormals[i0] += faceNormal;
    tempNormals[i1] += faceNormal;
    tempNormals[i2] += faceNormal;
  }

  // Normalize accumulated normals
  for (size_t i = 0; i < tempNormals.size(); ++i) {
    normals[i] = glm::normalize(tempNormals[i]);
  }

  return Mesh(vertices, indices, normals, uvs);
}
