#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include "vulkan_layer/fsb_object.hpp"

// engine level material definition
// can be converted to vulkan level descriptor set later

struct Material {
  int shaderIndex;
  FSBObject materialProperties;
  Material(int shaderIndex, FSBObject materialProperties);
  Material(int shaderIndex);

  void setProperty(std::string name, UniformVariant value);
};

extern const Material DEFAULT_MATERIAL;
