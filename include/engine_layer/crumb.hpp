#pragma once
#include "render_data.hpp"
#include "transform.hpp"
#include <optional>

class Scene;
/*Game entity class*/
class Crumb {
 private:
  Scene* m_scene;

 public:
  Transform transform;
  std::optional<RenderData> renderData;

  Crumb(Transform& transform);
  Crumb();
  Crumb(RenderData& renderData);
  Crumb(Transform& transform, RenderData& renderData);

  void onSceneAdd(Scene* scene);
};
