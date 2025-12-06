#pragma once

#include "debug.hpp"
#include "mesh.hpp"
#include <algorithm>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

Mesh generateTetrahedron() {
    // Vertices of a regular tetrahedron centered at the origin
    std::vector<glm::vec3> vertices = {
        {1.0f, 1.0f, 1.0f},   // 0
        {-1.0f, -1.0f, 1.0f}, // 1
        {-1.0f, 1.0f, -1.0f}, // 2
        {1.0f, -1.0f, -1.0f}  // 3
    };

    // Indices for tetrahedron faces
    std::vector<uint32_t> indices = {
        2, 1, 0,
        1, 3, 0,
        3, 2, 0,
        2, 3, 1};

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
    for (auto &n : vertexNormals)
        n = glm::normalize(n);

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

            float xPos = cos(xs * 2*M_PI) * sin(ys * M_PI);
            float yPos = cos(ys * M_PI);
            float zPos = sin(xs * 2*M_PI) * sin(ys * M_PI);

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

Mesh importMesh(std::string meshPath) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(
        meshPath,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_JoinIdenticalVertices
    );

    if (!scene || !scene->HasMeshes())
        throw std::runtime_error("Couldn't load mesh " + meshPath);

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<uint32_t> indices;
    std::vector<glm::vec2> uvs;

    for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
        aiMesh* mesh = scene->mMeshes[m];

        for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
            aiVector3D pos = mesh->mVertices[v];
            vertices.emplace_back(pos.x, pos.y, pos.z);

            aiVector3D norm = mesh->HasNormals() ? mesh->mNormals[v] : aiVector3D(0,1,0);
            normals.emplace_back(norm.x, norm.y, norm.z);

            if (mesh->HasTextureCoords(0)) {
                aiVector3D uv = mesh->mTextureCoords[0][v];
                uvs.emplace_back(uv.x, uv.y);
            } else {
                uvs.emplace_back(0.0f);
            }
        }

        for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
            aiFace face = mesh->mFaces[f];
            for (unsigned int i = 0; i < face.mNumIndices; ++i)
                indices.push_back(face.mIndices[i]);
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
    if (!file.is_open())
        throw std::runtime_error("Couldn't load mesh " + path);

    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string prefix;
        ss >> prefix;

        if (prefix == "v") {
            glm::vec3 v; ss >> v.x >> v.y >> v.z;
            vertices.push_back(v);
        }
        else if (prefix == "vt") {
            glm::vec2 uv; ss >> uv.x >> uv.y;
            temp_uvs.push_back(uv);
        }
        else if (prefix == "vn") {
            glm::vec3 n; ss >> n.x >> n.y >> n.z;
            temp_norms.push_back(n);
        }
        else if (prefix == "f") {
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
        { 1,0,-1 }, {-1,0,-1 }, {-1,0,1 }, { 1,0,1 }
    };

    std::vector<uint32_t> triangles = { 0,1,3, 1,2,3 };

    std::vector<glm::vec3> normals = {
        {0,1,0}, {0,1,0}, {0,1,0}, {0,1,0}
    };

    std::vector<glm::vec2> uvs = {
        {1,0}, {0,0}, {0,1}, {1,1}
    };

    return Mesh(vertices, triangles, normals, uvs);
}
