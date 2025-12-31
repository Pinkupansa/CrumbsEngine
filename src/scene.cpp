#include "engine_layer/scene.hpp"
#include "engine_layer/camera.hpp"

const bool Scene::hasCamera() const { return m_mainCamera != nullptr; }
const Camera* Scene::getMainCamera() const { return m_mainCamera; }
void Scene::setCamera(Camera& camera) { m_mainCamera = &camera; }
void Scene::addCrumb(Crumb& crumb) {
  m_crumbs.push_back(&crumb);
  crumb.onSceneAdd(this);
}

std::vector<Crumb*>& Scene::getAllCrumbs() { return m_crumbs; }
