#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include "fsb_object.hpp"
// engine level material definition
// can be converted to vulkan level descriptor set later

struct Material {
    int shaderIndex;
    FSBObject materialProperties;
    Material (int shaderIndex, FSBObject materialProperties): shaderIndex(shaderIndex){}
    Material(int shaderIndex) : shaderIndex(shaderIndex){}

    void setProperty(std::string name, UniformVariant value){
      materialProperties[name] = value;
    }
};

static const Material DEFAULT_MATERIAL = {0};