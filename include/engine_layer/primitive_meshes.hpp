#pragma once
#include "mesh.hpp"
#include <string>

Mesh generateTetrahedron();
Mesh generateSphere();
Mesh generateInvertedSphere();
Mesh importMesh(std::string meshPath);
Mesh loadOBJ(const std::string& path);
Mesh generateQuad();
Mesh generateInvertedCube();
Mesh loadOBJShared(const std::string& filename);
