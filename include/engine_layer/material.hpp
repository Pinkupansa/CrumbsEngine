#pragma once 
#include <cstdint>


//engine level material definition
//can be converted to vulkan level descriptor set later
struct Material {
    float tint[4];
    float specularIntensity;
    //texture
    int hasTexture;
    uint32_t textureImageIndex;
};