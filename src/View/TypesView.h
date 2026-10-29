#pragma once
#include <QColor>
#include <QGroupBox>
#include <QImage>
#include <QLabel>
#include <QMatrix4x4>
#include <QOpenGLExtraFunctions>
#include <QPushButton>
#include <QSlider>
#include <QVector3D>
#include <memory>
#include <vector>

#include "../Model/TypesModel.h"

namespace s21 {

// Forward declaration
class ViewerController;
class IProjectionStrategy;
struct RenderSettings;

enum class RenderMode {
  Wireframe,  // Каркасная модель
  Flat,       // Плоское затенение
  Gouraud,    // Мягкое затенение (метод Гуро)
  Phong       // Мягкое затенение (метод Фонга)
};

enum class ProjectionType { Central, Parallel };
enum class EdgeType { Solid, Dashed };
enum class VertexType { None, Circle, Square };

struct RayTraceSettings {
  int width = 800;
  int height = 600;
  int maxDepth = 3;
  int samplesPerPixel = 1;
  bool showFloor = true;
};

class ICamera {
 public:
  virtual ~ICamera() = default;
  virtual QMatrix4x4 getViewMatrix() const = 0;
  virtual QMatrix4x4 getProjectionMatrix() const = 0;
  virtual void setProjectionStrategy(
      std::unique_ptr<IProjectionStrategy> strategy) = 0;
  virtual void move(float dx, float dy, float dz) = 0;
  virtual void rotate(float yaw, float pitch) = 0;
  virtual void zoom(float factor) = 0;
  virtual float getYaw() const = 0;
  virtual void setYaw(float y) = 0;
  virtual QVector3D getPosition() const = 0;
  virtual QVector3D getTarget() const = 0;
  virtual float getFov() const = 0;
};

struct RenderObject {
  std::shared_ptr<IMesh> mesh;
  Matrix transform;
  Material material;
  bool isActive = false;
};

struct SceneSnapshot {
  Matrix globalTransform;
  std::vector<RenderObject> objects;
};

class ISceneProvider {
 public:
  virtual ~ISceneProvider() = default;
  virtual SceneSnapshot getSceneSnapshot() const = 0;
};

class IRenderer {
 public:
  virtual ~IRenderer() = default;
  virtual void init() = 0;
  virtual void render(const SceneSnapshot& snapshot, const ICamera& camera) = 0;
  virtual void resize(int w, int h) = 0;
  virtual void setGLFunctions(QOpenGLExtraFunctions* gl) = 0;
  virtual void setSettings(const RenderSettings& settings) = 0;
  virtual QImage renderRayTraced(const SceneSnapshot& snapshot,
                                 const ICamera& camera,
                                 const RenderSettings& settings) {
    (void)snapshot;
    (void)camera;
    (void)settings;
    return QImage();
  }
};

class IProjectionStrategy {
 public:
  virtual ~IProjectionStrategy() = default;
  virtual QMatrix4x4 getMatrix(float fovOrSize, float aspect, float zNear,
                               float zFar, float distance) const = 0;
};

class CentralProjection : public IProjectionStrategy {
 public:
  QMatrix4x4 getMatrix(float fovDegrees, float aspect, float zNear, float zFar,
                       float distance) const override {
    QMatrix4x4 matrix;
    matrix.perspective(fovDegrees, aspect, zNear, zFar);
    return matrix;
  }
};

class ParallelProjection : public IProjectionStrategy {
 public:
  QMatrix4x4 getMatrix(float orthoSize, float aspect, float zNear, float zFar,
                       float distance) const override {
    float halfHeight = distance * std::tan(qDegreesToRadians(orthoSize) / 2.0f);
    float halfWidth = halfHeight * aspect;

    QMatrix4x4 matrix;
    matrix.ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, zNear, zFar);
    return matrix;
  }
};

struct XYZSliders {
  QSlider* x = nullptr;
  QSlider* y = nullptr;
  QSlider* z = nullptr;
  QLabel* lblX = nullptr;
  QLabel* lblY = nullptr;
  QLabel* lblZ = nullptr;
};

struct LightUI {
  QGroupBox* group = nullptr;
  QSlider* x = nullptr;
  QSlider* y = nullptr;
  QSlider* z = nullptr;
  QPushButton* colorBtn = nullptr;
  QColor color;
  QSlider* intensity = nullptr;
};

struct PointLight {
  QVector3D position = {5.0f, 5.0f, 5.0f};
  QColor color = {255, 255, 255};
  float intensity = 1.0f;
  bool enabled = true;
};

struct RenderSettings {
  RenderMode renderMode = RenderMode::Wireframe;

  // Настройки каркасной модели
  EdgeType edgeType = EdgeType::Solid;
  QColor edgeColor = {255, 255, 255};
  float edgeThickness = 1.0f;

  // Настройки вершин
  VertexType vertexType = VertexType::None;
  QColor vertexColor = {255, 255, 255};
  float vertexSize = 5.0f;

  // Настройки затенения
  float ambientStrength = 0.1f;
  float specularStrength = 0.5f;
  float shininess = 32.0f;

  // Множественные источники света (до 5)
  static constexpr int MAX_LIGHTS = 5;
  std::array<PointLight, MAX_LIGHTS> pointLights;

  // Общие настройки
  QColor backgroundColor = QColor::fromRgbF(0.15f, 0.15f, 0.15f);
  ProjectionType projection = {ProjectionType::Central};

  // Настройки текстур
  QImage textureImage;
  bool useTexture = false;

  // Material индивидуальный для каждого объекта
  Material material;
  bool showFloor = true;
  int rayTraceWidth = 800;
  int rayTraceHeight = 600;
  int rayTraceDepth = 3;
};

}  // namespace s21