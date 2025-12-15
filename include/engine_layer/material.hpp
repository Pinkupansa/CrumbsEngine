#pragma once
#include <cstdint>
#include <glm/glm.hpp>

// engine level material definition
// can be converted to vulkan level descriptor set later

struct Material {
    glm::vec4 tint;
    float specularIntensity;
    // texture
    int textureImageIndex  = -1;
    int normalMapIndex     = -1;
    glm::vec2 tilingFactor = { 1, 1 };
    bool isLit;
    bool castsShadows;

    Material (glm::vec4 tint,
              float specularIntensity,
              int textureImageIndex,
              int normalMapIndex,
              glm::vec2 tilingFactor,
              bool castsShadows,
              bool isLit)
    : tint (tint), specularIntensity (specularIntensity),
      textureImageIndex (textureImageIndex), normalMapIndex (normalMapIndex),
      tilingFactor (tilingFactor), castsShadows(castsShadows), isLit(isLit) {
    }
};

static const Material DEFAULT_MATERIAL = { glm::vec4 (1.0f), 1.0f, -1, -1, { 1, 1 } , true, true};