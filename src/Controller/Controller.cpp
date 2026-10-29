#include "Controller.h"

#include "../Model/AnalyticMesh.h"

namespace s21 {

ViewerController::ViewerController() : m_scene(std::make_shared<Scene>()) {}

bool ViewerController::loadModel(const std::string& path) {
  auto mesh = m_parser.load(path);
  if (!mesh) return false;

  auto obj = std::make_shared<SceneObject>(mesh);

  if (m_scene->addObject(obj)) {
    m_activeIndex = m_scene->getObjectCount() - 1;
    return true;
  }
  return false;
}

std::string ViewerController::getLastError() const {
  return m_parser.getLastError();
}

void ViewerController::setActiveObject(size_t index) {
  if (index < m_scene->getObjectCount()) m_activeIndex = index;
}

size_t ViewerController::getActiveObjectIndex() const { return m_activeIndex; }
size_t ViewerController::getObjectCount() const {
  return m_scene->getObjectCount();
}

void ViewerController::removeActiveObject() {
  m_scene->removeObject(m_activeIndex);
  if (m_activeIndex > 0) m_activeIndex--;
}

size_t ViewerController::getVertexCount() const {
  auto obj = m_scene->getObject(m_activeIndex);
  return obj ? obj->getMesh()->getVertexCount() : 0;
}

size_t ViewerController::getEdgeCount() const {
  auto obj = m_scene->getObject(m_activeIndex);
  return obj ? obj->getMesh()->getEdgeCount() : 0;
}

size_t ViewerController::getFaceCount() const {
  auto obj = m_scene->getObject(m_activeIndex);
  return obj ? obj->getMesh()->getFaceCount() : 0;
}

bool ViewerController::hasUvs() const {
  auto obj = m_scene->getObject(m_activeIndex);
  return obj ? obj->getMesh()->hasTexCoords() : false;
}

QString ViewerController::getObjectTypeName(size_t index) const {
  auto obj = m_scene->getObject(index);
  if (!obj || !obj->getMesh()) return "Empty";
  auto prim = obj->getMesh()->getAnalyticPrimitive();
  if (prim) return prim->name();
  return "Mesh";
}

void ViewerController::addSphere(float radius) {
  auto prim = std::make_shared<SpherePrimitive>(QVector3D(0, 0, 0), radius);
  auto mesh = std::make_shared<AnalyticMesh>(prim, 32);
  auto obj = std::make_shared<SceneObject>(mesh);
  if (m_scene->addObject(obj)) {
    m_activeIndex = m_scene->getObjectCount() - 1;
  }
}

void ViewerController::addCylinder(float radius, float height) {
  auto prim = std::make_shared<CylinderPrimitive>(
      QVector3D(0, 0, 0), QVector3D(0, 1, 0), radius, height);
  auto mesh = std::make_shared<AnalyticMesh>(prim, 32);
  auto obj = std::make_shared<SceneObject>(mesh);
  if (m_scene->addObject(obj)) {
    m_activeIndex = m_scene->getObjectCount() - 1;
  }
}

void ViewerController::addCone(float radius, float height) {
  auto prim = std::make_shared<ConePrimitive>(
      QVector3D(0, 0, 0), QVector3D(0, 1, 0), radius, height);
  auto mesh = std::make_shared<AnalyticMesh>(prim, 32);
  auto obj = std::make_shared<SceneObject>(mesh);
  if (m_scene->addObject(obj)) {
    m_activeIndex = m_scene->getObjectCount() - 1;
  }
}

void ViewerController::addCube(float size) {
  auto prim = std::make_shared<CubePrimitive>(QVector3D(0, 0, 0), size);
  auto mesh = std::make_shared<AnalyticMesh>(prim, 1);
  auto obj = std::make_shared<SceneObject>(mesh);
  if (m_scene->addObject(obj)) {
    m_activeIndex = m_scene->getObjectCount() - 1;
  }
}

void ViewerController::addPyramid(float baseSize, float height) {
  auto prim =
      std::make_shared<PyramidPrimitive>(QVector3D(0, 0, 0), baseSize, height);
  auto mesh = std::make_shared<AnalyticMesh>(prim, 1);
  auto obj = std::make_shared<SceneObject>(mesh);
  if (m_scene->addObject(obj)) {
    m_activeIndex = m_scene->getObjectCount() - 1;
  }
}

// --- Трансформации ---

void ViewerController::translate(float dx, float dy, float dz, bool global) {
  if (global) {
    m_scene->setGlobalTransform(AffineTransformer::translate(
        m_scene->getGlobalTransform(), dx, dy, dz));
  } else {
    auto obj = m_scene->getObject(m_activeIndex);
    if (obj)
      obj->setTransform(
          AffineTransformer::translate(obj->getTransform(), dx, dy, dz));
  }
}

void ViewerController::rotate(float ax, float ay, float az, bool global) {
  if (global) {
    m_scene->setGlobalTransform(
        AffineTransformer::rotate(m_scene->getGlobalTransform(), ax, ay, az));
  } else {
    auto obj = m_scene->getObject(m_activeIndex);
    if (obj)
      obj->setTransform(
          AffineTransformer::rotate(obj->getTransform(), ax, ay, az));
  }
}

void ViewerController::scale(float sx, float sy, float sz, bool global) {
  if (global) {
    m_scene->setGlobalTransform(
        AffineTransformer::scale(m_scene->getGlobalTransform(), sx, sy, sz));
  } else {
    auto obj = m_scene->getObject(m_activeIndex);
    if (obj)
      obj->setTransform(
          AffineTransformer::scale(obj->getTransform(), sx, sy, sz));
  }
}

void ViewerController::resetTransform(bool global) {
  if (global)
    m_scene->setGlobalTransform(AffineTransformer::reset());
  else {
    auto obj = m_scene->getObject(m_activeIndex);
    if (obj) obj->setTransform(AffineTransformer::reset());
  }
}

SceneSnapshot ViewerController::getSceneSnapshot() const {
  SceneSnapshot snapshot;
  snapshot.globalTransform = m_scene->getGlobalTransform();
  const auto& objects = m_scene->getObjects();
  snapshot.objects.reserve(objects.size());

  for (size_t i = 0; i < objects.size(); ++i) {
    RenderObject ro;
    ro.mesh = objects[i]->getMesh();
    ro.transform = objects[i]->getTransform();
    ro.material = objects[i]->getMaterial();
    ro.isActive = (i == m_activeIndex);
    snapshot.objects.push_back(ro);
  }
  return snapshot;
}

Material ViewerController::getActiveMaterial() const {
  auto obj = m_scene->getObject(m_activeIndex);
  return obj ? obj->getMaterial() : Material{};
}

void ViewerController::setActiveMaterial(const Material& mat) {
  auto obj = m_scene->getObject(m_activeIndex);
  if (obj) obj->setMaterial(mat);
}

// --- Реализация команд ---

TranslateCommand::TranslateCommand(ViewerController& c, float dx, float dy,
                                   float dz, bool g)
    : BaseCommand(c, g), m_dx(dx), m_dy(dy), m_dz(dz) {}
void TranslateCommand::execute() {
  m_controller.translate(m_dx, m_dy, m_dz, m_global);
}

RotateCommand::RotateCommand(ViewerController& c, float ax, float ay, float az,
                             bool g)
    : BaseCommand(c, g), m_ax(ax), m_ay(ay), m_az(az) {}
void RotateCommand::execute() {
  m_controller.rotate(m_ax, m_ay, m_az, m_global);
}

ScaleCommand::ScaleCommand(ViewerController& c, float sx, float sy, float sz,
                           bool g)
    : BaseCommand(c, g), m_sx(sx), m_sy(sy), m_sz(sz) {}
void ScaleCommand::execute() { m_controller.scale(m_sx, m_sy, m_sz, m_global); }

}  // namespace s21