#pragma once 
#include "material.hpp"
class RenderData{
    public: 
        int meshIndex; 
        Material material; 
        RenderData(int meshIndex, Material& material): meshIndex(meshIndex), material(material){}
        RenderData(int meshIndex): meshIndex(meshIndex), material(DEFAULT_MATERIAL){}
};