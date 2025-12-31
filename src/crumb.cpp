#include "engine_layer/crumb.hpp"

Crumb::Crumb(Transform& transform) : transform(transform) {}
Crumb::Crumb() {}
Crumb::Crumb(RenderData& renderData) : renderData(renderData) {}
Crumb::Crumb(Transform& transform, RenderData& renderData)
    : transform(transform), renderData(renderData) {}

void Crumb::onSceneAdd(Scene* scene) { m_scene = scene; }
