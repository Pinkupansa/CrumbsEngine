#pragma once

#include <glm/glm.hpp>
#include "debug.hpp"
#include "mesh.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
Mesh generateTetrahedron(){
    // Vertices of a regular tetrahedron centered at the origin
    std::vector<glm::vec3> vertices = {
        { 1.0f,  1.0f,  1.0f},   // 0
        {-1.0f, -1.0f,  1.0f},   // 1
        {-1.0f,  1.0f, -1.0f},   // 2
        { 1.0f, -1.0f, -1.0f}    // 3
    };

    // Indices for tetrahedron faces
    std::vector<uint32_t> indices = {
        2, 1, 0,
        1, 3, 0,
        3, 2, 0,
        2, 3, 1
    };

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
    return Mesh(vertices, indices, vertexNormals);
}


Mesh generateSphere() {
    const uint32_t X_SEGMENTS = 32; // longitude resolution
    const uint32_t Y_SEGMENTS = 16; // latitude resolution
    const float radius = 1.0f;

    std::vector<glm::vec3> vertices;
    std::vector<uint32_t> indices;
    std::vector<glm::vec3> normals;

    // Generate vertices and normals
    for (uint32_t y = 0; y <= Y_SEGMENTS; ++y) {
        for (uint32_t x = 0; x <= X_SEGMENTS; ++x) {
            float xSegment = (float)x / (float)X_SEGMENTS;
            float ySegment = (float)y / (float)Y_SEGMENTS;

            float xPos = std::cos(xSegment * 2.0f * M_PI) * std::sin(ySegment * M_PI);
            float yPos = std::cos(ySegment * M_PI);
            float zPos = std::sin(xSegment * 2.0f * M_PI) * std::sin(ySegment * M_PI);

            glm::vec3 pos = glm::vec3(xPos, yPos, zPos) * radius;
            vertices.push_back(pos);
            normals.push_back(glm::normalize(pos)); // normal = position normalized
        }
    }

    // Generate indices
    bool oddRow = false;
    for (uint32_t y = 0; y < Y_SEGMENTS; ++y) {
        for (uint32_t x = 0; x < X_SEGMENTS; ++x) {
            uint32_t i0 = y * (X_SEGMENTS + 1) + x;
            uint32_t i1 = (y + 1) * (X_SEGMENTS + 1) + x;
            uint32_t i2 = (y + 1) * (X_SEGMENTS + 1) + x + 1;
            uint32_t i3 = y * (X_SEGMENTS + 1) + x + 1;

            indices.push_back(i2);
            indices.push_back(i1);
            indices.push_back(i0);

            indices.push_back(i3);
            indices.push_back(i2);
            indices.push_back(i0);
        }
    }

    Debug::Log(std::to_string(indices.size()));
    return Mesh(vertices, indices, normals);
}


Mesh importMesh(std::string meshPath){
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(meshPath,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_JoinIdenticalVertices
    );

    if (!scene || !scene->HasMeshes()) {
        Debug::LogError("Can't load mesh " + meshPath + " !");
        throw std::runtime_error("Couldn't load mesh " + meshPath);
    }

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<uint32_t> indices;

    for (unsigned int m = 0; m < scene->mNumMeshes; ++m) {
        aiMesh* mesh = scene->mMeshes[m];

        // Vertices & normals
        for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
            aiVector3D pos = mesh->mVertices[v];
            aiVector3D norm = mesh->HasNormals() ? mesh->mNormals[v] : aiVector3D(0,1,0);

            vertices.push_back(glm::vec3(pos.x, pos.y, pos.z));
            normals.push_back(glm::vec3(norm.x, norm.y, norm.z));
        }

        // Indices
        for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
            aiFace face = mesh->mFaces[f];
            for (unsigned int i = 0; i < face.mNumIndices; ++i) {
                indices.push_back(face.mIndices[i]);
            }
        }
    }
    return Mesh(vertices, indices, normals);
}

Mesh loadOBJ(const std::string& path) {
    std::vector<glm::vec3> vertices; 
    std::vector<glm::vec3> normals;
    std::vector<uint32_t> indices;
    std::ifstream file(path);
    if (!file.is_open()){
        Debug::LogError("Can't load mesh " + path + " !");
        throw std::runtime_error("Couldn't load mesh " + path);
    }


    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string prefix;
        ss >> prefix;

        if (prefix == "v") {
            glm::vec3 v;
            ss >> v.x >> v.y >> v.z;
            vertices.push_back(v);
        } 
        else if (prefix == "vn") {
            glm::vec3 n;
            ss >> n.x >> n.y >> n.z;
            normals.push_back(n);
        } 
        else if (prefix == "f") {
            std::string vertStr;
            for (int i = 0; i < 3; ++i) {
                ss >> vertStr;
                std::replace(vertStr.begin(), vertStr.end(), '/', ' ');
                std::stringstream vss(vertStr);
                int vi, ti, ni;
                vss >> vi >> ti >> ni; // ignore texture index
                indices.push_back(vi - 1); // store vertex index
            }
        }
    }

    return Mesh(vertices, indices, normals);
}