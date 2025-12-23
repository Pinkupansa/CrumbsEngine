#pragma once 
#include "material.hpp"
class RenderData{
    public: 
        int meshIndex; 
        Material material; 
        bool castsShadows; 

        RenderData(int meshIndex, Material& material, bool castsShadows = true): meshIndex(meshIndex), material(material), castsShadows(castsShadows){}
        RenderData(int meshIndex, bool castsShadows = true): meshIndex(meshIndex), material(DEFAULT_MATERIAL), castsShadows(castsShadows){}
};