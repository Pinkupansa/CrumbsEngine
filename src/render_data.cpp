#include "engine_layer/render_data.hpp"

RenderData::RenderData(int meshIndex, Material& material, bool castsShadows)
    : meshIndex(meshIndex), material(material), castsShadows(castsShadows) {}
RenderData::RenderData(int meshIndex, bool castsShadows)
    : meshIndex(meshIndex), material(DEFAULT_MATERIAL), castsShadows(castsShadows) {}
