#pragma once
#include "material.hpp"
class RenderData {
 public:
  int meshIndex;
  Material material;
  bool castsShadows;

  RenderData(int meshIndex, Material& material, bool castsShadows = true);
  RenderData(int meshIndex, bool castsShadows = true);
};
