#pragma once 
#include "transform.hpp"
#include "render_data.hpp"
#include <optional>

class Scene; 
/*Game entity class*/
class Crumb{ 
    private: 
        Scene* m_scene;
    public: 
        Transform transform;
        std::optional<RenderData> renderData;

        Crumb(Transform& transform): transform(transform){}
        Crumb(){}
        Crumb(RenderData& renderData): renderData(renderData){}
        Crumb(Transform& transform, RenderData& renderData): transform(transform), renderData(renderData){}
        
        void onSceneAdd(Scene* scene){
            m_scene = scene;
        }
};