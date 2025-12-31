#include "engine_layer/material.hpp"

Material::Material(int shaderIndex, FSBObject materialProperties)
    : shaderIndex(shaderIndex), materialProperties(materialProperties) {}
Material::Material(int shaderIndex) : shaderIndex(shaderIndex) {}

void Material::setProperty(std::string name, UniformVariant value) {
  materialProperties[name] = value;
}

const Material DEFAULT_MATERIAL = {0};
