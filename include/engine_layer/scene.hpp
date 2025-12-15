#pragma once

#include <vector>
#include "crumb.hpp"
class Scene{
    private: 
        std::vector<Crumb*> m_crumbs;
    
    public: 
        void addCrumb(Crumb& crumb){
            m_crumbs.push_back(&crumb);
            crumb.onSceneAdd(this);
        }
        std::vector<Crumb*>& getAllCrumbs(){
            return m_crumbs;
        }
};