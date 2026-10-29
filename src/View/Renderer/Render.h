#pragma once

#include <QMatrix4x4>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <memory>
#include <unordered_map>

#include "../RayTracer.h"
#include "../TypesView.h"

namespace s21 {

// Интерфейс техник
class IShaderTechnique {
 public:
  virtual ~IShaderTechnique() = default;
  virtual void init(QOpenGLExtraFunctions* gl) = 0;
  virtual void bindAndSetUniforms(const RenderObject& obj,
                                  const QMatrix4x4& globalTransform,
                                  const ICamera& camera,
                                  const RenderSettings& settings) = 0;
  virtual void release() = 0;
};

class BaseTechnique : public IShaderTechnique {
 public:
  void release() override { m_program.release(); }
  void setLights(const RenderSettings& set);

 protected:
  QOpenGLShaderProgram m_program;
  QOpenGLExtraFunctions* m_gl = nullptr;
};

// Объявления техник
class EdgeTechnique : public BaseTechnique {
 public:
  void init(QOpenGLExtraFunctions* gl) override;
  void bindAndSetUniforms(const RenderObject& obj, const QMatrix4x4& global,
                          const ICamera& cam,
                          const RenderSettings& set) override;
  void bindWithColor(const QMatrix4x4& mvp, const QVector3D& color, int dashed);
};
class PointTechnique : public BaseTechnique {
 public:
  void init(QOpenGLExtraFunctions* gl) override;
  void bindAndSetUniforms(const RenderObject& obj, const QMatrix4x4& global,
                          const ICamera& cam,
                          const RenderSettings& set) override;
};
class FlatTechnique : public BaseTechnique {
 public:
  void init(QOpenGLExtraFunctions* gl) override;
  void bindAndSetUniforms(const RenderObject& obj, const QMatrix4x4& global,
                          const ICamera& cam,
                          const RenderSettings& set) override;
};
class GouraudTechnique : public BaseTechnique {
 public:
  void init(QOpenGLExtraFunctions* gl) override;
  void bindAndSetUniforms(const RenderObject& obj, const QMatrix4x4& global,
                          const ICamera& cam,
                          const RenderSettings& set) override;
};
class PhongTechnique : public BaseTechnique {
 public:
  void init(QOpenGLExtraFunctions* gl) override;
  void bindAndSetUniforms(const RenderObject& obj, const QMatrix4x4& global,
                          const ICamera& cam,
                          const RenderSettings& set) override;
};
class TextureTechnique : public BaseTechnique {
 public:
  void init(QOpenGLExtraFunctions* gl) override;
  void bindAndSetUniforms(const RenderObject& obj, const QMatrix4x4& global,
                          const ICamera& cam,
                          const RenderSettings& set) override;
};

// Структура буферов для конкретной модели
struct MeshGLBuffers {
  std::unique_ptr<QOpenGLVertexArrayObject> edgeVAO;
  std::unique_ptr<QOpenGLBuffer> edgeVBO;
  std::unique_ptr<QOpenGLBuffer> edgeIBO;
  std::unique_ptr<QOpenGLVertexArrayObject> pointVAO;

  std::unique_ptr<QOpenGLVertexArrayObject> faceVAO;
  std::unique_ptr<QOpenGLBuffer> faceVBO;
  std::unique_ptr<QOpenGLBuffer> faceNBO;
  std::unique_ptr<QOpenGLBuffer> faceTBO;
  std::unique_ptr<QOpenGLBuffer> faceIBO;

  size_t indexCount = 0;
  size_t vertexCount = 0;
  size_t faceIndexCount = 0;
  bool wireframeUploaded = false;
  bool solidUploaded = false;
};

class SceneRenderer : public IRenderer {
 public:
  SceneRenderer() = default;
  ~SceneRenderer() override = default;

  void init() override;
  void render(const SceneSnapshot& snapshot, const ICamera& camera) override;
  void resize(int w, int h) override;
  void setGLFunctions(QOpenGLExtraFunctions* gl) override { m_gl = gl; }
  void setSettings(const RenderSettings& settings) override {
    m_settings = settings;
  }
  QImage renderRayTraced(const SceneSnapshot& snapshot, const ICamera& camera,
                         const RenderSettings& settings) override;

 private:
  void ensureMeshUploaded(const std::shared_ptr<IMesh>& mesh);
  void initFloor();
  void renderFloor(const QMatrix4x4& globalTransform, const ICamera& camera);

  QOpenGLExtraFunctions* m_gl = nullptr;
  RenderSettings m_settings;
  std::unordered_map<IMesh*, std::unique_ptr<MeshGLBuffers>> m_meshBuffers;
  std::unique_ptr<QOpenGLTexture> m_texture;
  qint64 m_loadedTextureKey = 0;

  // Техники
  std::unique_ptr<IShaderTechnique> m_edgeTech;
  std::unique_ptr<IShaderTechnique> m_pointTech;
  std::unique_ptr<IShaderTechnique> m_flatTech;
  std::unique_ptr<IShaderTechnique> m_gouraudTech;
  std::unique_ptr<IShaderTechnique> m_phongTech;
  std::unique_ptr<IShaderTechnique> m_textureTech;

  RayTracer m_rayTracer;

  // Floor buffers
  std::unique_ptr<QOpenGLVertexArrayObject> m_floorVAO;
  std::unique_ptr<QOpenGLBuffer> m_floorVBO;
  std::unique_ptr<QOpenGLBuffer> m_floorIBO;
  size_t m_floorIndexCount = 0;
  bool m_floorInitialized = false;

  bool m_initialized = false;
};

}  // namespace s21