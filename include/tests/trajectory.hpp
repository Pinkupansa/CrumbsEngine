#pragma once 
#include <string>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <glm/vec3.hpp>   // glm::dvec3
struct Trajectory {
    // element -> atom index -> frame -> position
    std::unordered_map<
        std::string,
        std::vector<std::vector<glm::vec3>>
    > data;

    size_t nFrames = 0;
};

#include <fstream>
#include <stdexcept>

Trajectory readTrajectory(const std::string& filename) {
    std::ifstream in(filename);
    if (!in) throw std::runtime_error("Cannot open file");

    Trajectory traj;
    std::string line;

    while (true) {
        size_t nAtoms;
        if (!(in >> nAtoms)) break;
        std::getline(in, line); // rest of line
        std::getline(in, line); // comment line

        // Per-frame temporary storage
        std::unordered_map<std::string, std::vector<glm::vec3>> frameData;

        for (size_t i = 0; i < nAtoms; ++i) {
            std::string type;
            glm::vec3 pos;
            in >> type >> pos.x >> pos.y >> pos.z;
            frameData[type].push_back(pos);
        }

        // Append frame data
        for (auto& [type, positions] : frameData) {
            traj.data[type].push_back(std::move(positions));
        }

        traj.nFrames++;
    }

    return traj;
}
