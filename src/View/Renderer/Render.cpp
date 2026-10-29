#include "Render.h"

#include "Shaders.h"

namespace s21 {

static QMatrix4x4 toQt(const s21::Matrix& m) {
  QMatrix4x4 result;
  for (int r = 0; r < 4; ++r) {
    for (int c = 0; c < 4; ++c) {
      result(r, c) = static_cast<float>(m(r, c));
    }
  }
  return result;
}

// --- Реализация техник ---

void EdgeTechnique::init(QOpenGLExtraFunctions* gl) {
  m_gl = gl;
  m_program.addShaderFromSourceCode(QOpenGLShader::Vertex, shaders::EDGE_VERT);
  m_program.addShaderFromSourceCode(QOpenGLShader::Fragment,
                                    shaders::EDGE_FRAG);
  m_program.link();
}
void EdgeTechnique::bindAndSetUniforms(const RenderObject& obj,
                                       const QMatrix4x4& global,
                                       const ICamera& cam,
                                       const RenderSettings& set) {
  m_program.bind();
  QMatrix4x4 model = global * toQt(obj.transform);
  m_program.setUniformValue(
      "mvp", cam.getProjectionMatrix() * cam.getViewMatrix() * model);
  m_program.setUniformValue("color", set.edgeColor.redF(),
                            set.edgeColor.greenF(), set.edgeColor.blueF());
  m_program.setUniformValue("dashed", set.edgeType == EdgeType::Dashed ? 1 : 0);
  m_gl->glLineWidth(set.edgeThickness);
}

void EdgeTechnique::bindWithColor(const QMatrix4x4& mvp, const QVector3D& color,
                                  int dashed) {
  m_program.bind();
  m_program.setUniformValue("mvp", mvp);
  m_program.setUniformValue("color", color.x(), color.y(), color.z());
  m_program.setUniformValue("dashed", dashed);
}

void BaseTechnique::setLights(const RenderSettings& set) {
  // Собираем только включённые источники
  std::vector<int> activeIndices;
  for (int i = 0; i < RenderSettings::MAX_LIGHTS; ++i) {
    if (set.pointLights[i].enabled) {
      activeIndices.push_back(i);
    }
  }

  int count = static_cast<int>(activeIndices.size());
  m_program.setUniformValue("lightCount", count);

  // Упаковываем активные источники подряд: 0, 1, 2...
  for (int j = 0; j < count; ++j) {
    int srcIdx = activeIndices[j];
    QString posName = QString("lightPos[%1]").arg(j);
    QString colName = QString("lightColor[%1]").arg(j);
    QString intName = QString("lightIntensity[%1]").arg(j);

    m_program.setUniformValue(posName.toStdString().c_str(),
                              set.pointLights[srcIdx].position);
    m_program.setUniformValue(colName.toStdString().c_str(),
                              set.pointLights[srcIdx].color.redF(),
                              set.pointLights[srcIdx].color.greenF(),
                              set.pointLights[srcIdx].color.blueF());
    m_program.setUniformValue(intName.toStdString().c_str(),
                              set.pointLights[srcIdx].intensity);
  }

  // Остальные слоты обнуляем
  for (int j = count; j < RenderSettings::MAX_LIGHTS; ++j) {
    QString posName = QString("lightPos[%1]").arg(j);
    QString colName = QString("lightColor[%1]").arg(j);
    QString intName = QString("lightIntensity[%1]").arg(j);

    m_program.setUniformValue(posName.toStdString().c_str(),
                              QVector3D(0, 0, 0));
    m_program.setUniformValue(colName.toStdString().c_str(), 0.0f, 0.0f, 0.0f);
    m_program.setUniformValue(intName.toStdString().c_str(), 0.0f);
  }
}

void PointTechnique::init(QOpenGLExtraFunctions* gl) {
  m_gl = gl;
  m_program.addShaderFromSourceCode(QOpenGLShader::Vertex, shaders::POINT_VERT);
  m_program.addShaderFromSourceCode(QOpenGLShader::Fragment,
                                    shaders::POINT_FRAG);
  m_program.link();
}
void PointTechnique::bindAndSetUniforms(const RenderObject& obj,
                                        const QMatrix4x4& global,
                                        const ICamera& cam,
                                        const RenderSettings& set) {
  m_program.bind();
  QMatrix4x4 model = global * toQt(obj.transform);
  m_program.setUniformValue(
      "mvp", cam.getProjectionMatrix() * cam.getViewMatrix() * model);
  m_program.setUniformValue("color", set.vertexColor.redF(),
                            set.vertexColor.greenF(), set.vertexColor.blueF());
  m_program.setUniformValue("circle",
                            set.vertexType == VertexType::Circle ? 1 : 0);
  m_program.setUniformValue("pointSize", set.vertexSize);
}

void FlatTechnique::init(QOpenGLExtraFunctions* gl) {
  m_gl = gl;
  m_program.addShaderFromSourceCode(QOpenGLShader::Vertex, shaders::FLAT_VERT);
  m_program.addShaderFromSourceCode(QOpenGLShader::Fragment,
                                    shaders::FLAT_FRAG);
  m_program.link();
}
void FlatTechnique::bindAndSetUniforms(const RenderObject& obj,
                                       const QMatrix4x4& global,
                                       const ICamera& cam,
                                       const RenderSettings& set) {
  m_program.bind();
  QMatrix4x4 model = global * toQt(obj.transform);
  m_program.setUniformValue(
      "mvp", cam.getProjectionMatrix() * cam.getViewMatrix() * model);
  m_program.setUniformValue("model", model);
  m_program.setUniformValue("objectColor", obj.material.baseColor.redF(),
                            obj.material.baseColor.greenF(),
                            obj.material.baseColor.blueF());
  setLights(set);
  m_program.setUniformValue(
      "viewPos", cam.getViewMatrix().inverted().column(3).toVector3D());
  m_program.setUniformValue("ambientStrength", set.ambientStrength);
  m_program.setUniformValue("specularStrength", set.specularStrength);
  m_program.setUniformValue("shininess", set.shininess);
}

void GouraudTechnique::init(QOpenGLExtraFunctions* gl) {
  m_gl = gl;
  m_program.addShaderFromSourceCode(QOpenGLShader::Vertex,
                                    shaders::GOURAUD_VERT);
  m_program.addShaderFromSourceCode(QOpenGLShader::Fragment,
                                    shaders::GOURAUD_FRAG);
  m_program.link();
}
void GouraudTechnique::bindAndSetUniforms(const RenderObject& obj,
                                          const QMatrix4x4& global,
                                          const ICamera& cam,
                                          const RenderSettings& set) {
  m_program.bind();
  QMatrix4x4 model = global * toQt(obj.transform);
  m_program.setUniformValue(
      "mvp", cam.getProjectionMatrix() * cam.getViewMatrix() * model);
  m_program.setUniformValue("model", model);
  m_program.setUniformValue("objectColor", obj.material.baseColor.redF(),
                            obj.material.baseColor.greenF(),
                            obj.material.baseColor.blueF());
  setLights(set);
  m_program.setUniformValue(
      "viewPos", cam.getViewMatrix().inverted().column(3).toVector3D());
  m_program.setUniformValue("ambientStrength", set.ambientStrength);
  m_program.setUniformValue("specularStrength", set.specularStrength);
  m_program.setUniformValue("shininess", set.shininess);
}

void PhongTechnique::init(QOpenGLExtraFunctions* gl) {
  m_gl = gl;
  m_program.addShaderFromSourceCode(QOpenGLShader::Vertex, shaders::PHONG_VERT);
  m_program.addShaderFromSourceCode(QOpenGLShader::Fragment,
                                    shaders::PHONG_FRAG);
  m_program.link();
}
void PhongTechnique::bindAndSetUniforms(const RenderObject& obj,
                                        const QMatrix4x4& global,
                                        const ICamera& cam,
                                        const RenderSettings& set) {
  m_program.bind();
  QMatrix4x4 model = global * toQt(obj.transform);
  m_program.setUniformValue(
      "mvp", cam.getProjectionMatrix() * cam.getViewMatrix() * model);
  m_program.setUniformValue("model", model);
  m_program.setUniformValue("objectColor", obj.material.baseColor.redF(),
                            obj.material.baseColor.greenF(),
                            obj.material.baseColor.blueF());
  setLights(set);
  m_program.setUniformValue(
      "viewPos", cam.getViewMatrix().inverted().column(3).toVector3D());
  m_program.setUniformValue("ambientStrength", set.ambientStrength);
  m_program.setUniformValue("specularStrength", set.specularStrength);
  m_program.setUniformValue("shininess", set.shininess);
}

void TextureTechnique::init(QOpenGLExtraFunctions* gl) {
  m_gl = gl;
  m_program.addShaderFromSourceCode(QOpenGLShader::Vertex,
                                    shaders::TEXTURE_VERT);
  m_program.addShaderFromSourceCode(QOpenGLShader::Fragment,
                                    shaders::TEXTURE_FRAG);
  m_program.link();
}
void TextureTechnique::bindAndSetUniforms(const RenderObject& obj,
                                          const QMatrix4x4& global,
                                          const ICamera& cam,
                                          const RenderSettings& set) {
  m_program.bind();
  QMatrix4x4 model = global * toQt(obj.transform);
  m_program.setUniformValue(
      "mvp", cam.getProjectionMatrix() * cam.getViewMatrix() * model);
  m_program.setUniformValue("model", model);
  m_program.setUniformValue("textureSampler", 0);
  setLights(set);
  m_program.setUniformValue(
      "viewPos", cam.getViewMatrix().inverted().column(3).toVector3D());
  m_program.setUniformValue("ambientStrength", set.ambientStrength);
  m_program.setUniformValue("specularStrength", set.specularStrength);
  m_program.setUniformValue("shininess", set.shininess);
}

// --- SceneRenderer ---

void SceneRenderer::init() {
  if (m_initialized) return;

  // Инициализируем техники
  m_edgeTech = std::make_unique<EdgeTechnique>();
  m_edgeTech->init(m_gl);
  m_pointTech = std::make_unique<PointTechnique>();
  m_pointTech->init(m_gl);
  m_flatTech = std::make_unique<FlatTechnique>();
  m_flatTech->init(m_gl);
  m_gouraudTech = std::make_unique<GouraudTechnique>();
  m_gouraudTech->init(m_gl);
  m_phongTech = std::make_unique<PhongTechnique>();
  m_phongTech->init(m_gl);
  m_textureTech = std::make_unique<TextureTechnique>();
  m_textureTech->init(m_gl);

  m_gl->glEnable(GL_PROGRAM_POINT_SIZE);
  m_gl->glEnable(GL_DEPTH_TEST);
  initFloor();
  m_initialized = true;
}

void SceneRenderer::ensureMeshUploaded(const std::shared_ptr<IMesh>& mesh) {
  if (!mesh || m_meshBuffers.count(mesh.get())) return;

  MeshData data = mesh->extractMeshData();
  auto buffers = std::make_unique<MeshGLBuffers>();

  // Solid
  if (!data.solidIndices.empty()) {
    buffers->faceVAO = std::make_unique<QOpenGLVertexArrayObject>();
    buffers->faceVAO->create();
    buffers->faceVBO =
        std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
    buffers->faceVBO->create();
    buffers->faceNBO =
        std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
    buffers->faceNBO->create();
    buffers->faceTBO =
        std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
    buffers->faceTBO->create();
    buffers->faceIBO =
        std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::IndexBuffer);
    buffers->faceIBO->create();

    buffers->faceVBO->bind();
    buffers->faceVBO->allocate(data.solidVertices.data(),
                               data.solidVertices.size() * sizeof(float));
    buffers->faceNBO->bind();
    if (data.hasNormals)
      buffers->faceNBO->allocate(data.normals.data(),
                                 data.normals.size() * sizeof(float));
    buffers->faceTBO->bind();
    if (data.hasUvs)
      buffers->faceTBO->allocate(data.uvs.data(),
                                 data.uvs.size() * sizeof(float));

    buffers->faceVAO->bind();
    buffers->faceVBO->bind();
    m_gl->glEnableVertexAttribArray(0);
    m_gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    if (data.hasNormals) {
      buffers->faceNBO->bind();
      m_gl->glEnableVertexAttribArray(1);
      m_gl->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    }
    if (data.hasUvs) {
      buffers->faceTBO->bind();
      m_gl->glEnableVertexAttribArray(2);
      m_gl->glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    }
    buffers->faceIBO->bind();
    buffers->faceIBO->allocate(data.solidIndices.data(),
                               data.solidIndices.size() * sizeof(uint));
    buffers->faceVAO->release();

    buffers->faceIndexCount = data.solidIndices.size();
    buffers->solidUploaded = true;
  }

  // Wireframe
  if (!data.wireframeIndices.empty()) {
    buffers->edgeVAO = std::make_unique<QOpenGLVertexArrayObject>();
    buffers->edgeVAO->create();
    buffers->edgeVBO =
        std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
    buffers->edgeVBO->create();
    buffers->edgeIBO =
        std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::IndexBuffer);
    buffers->edgeIBO->create();
    buffers->pointVAO = std::make_unique<QOpenGLVertexArrayObject>();
    buffers->pointVAO->create();

    buffers->edgeVBO->bind();
    buffers->edgeVBO->allocate(data.wireframeVertices.data(),
                               data.wireframeVertices.size() * sizeof(float));

    buffers->edgeVAO->bind();
    buffers->edgeVBO->bind();
    m_gl->glEnableVertexAttribArray(0);
    m_gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    buffers->edgeIBO->bind();
    buffers->edgeIBO->allocate(data.wireframeIndices.data(),
                               data.wireframeIndices.size() * sizeof(uint));
    buffers->edgeVAO->release();

    buffers->pointVAO->bind();
    buffers->edgeVBO->bind();
    m_gl->glEnableVertexAttribArray(0);
    m_gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    buffers->pointVAO->release();

    buffers->indexCount = data.wireframeIndices.size();
    buffers->vertexCount = data.wireframeVertices.size() / 3;
    buffers->wireframeUploaded = true;
  }

  m_meshBuffers[mesh.get()] = std::move(buffers);
}

void SceneRenderer::render(const SceneSnapshot& snapshot,
                           const ICamera& camera) {
  // Текстура
  if (m_settings.useTexture && !m_settings.textureImage.isNull()) {
    if (m_settings.textureImage.cacheKey() != m_loadedTextureKey) {
      m_texture = std::make_unique<QOpenGLTexture>(
          m_settings.textureImage.convertToFormat(QImage::Format_RGBA8888));
      m_texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
      m_texture->setWrapMode(QOpenGLTexture::Repeat);
      m_loadedTextureKey = m_settings.textureImage.cacheKey();
    }
  } else if (m_texture) {
    m_texture.reset();
    m_loadedTextureKey = 0;
  }

  m_gl->glClearColor(m_settings.backgroundColor.redF(),
                     m_settings.backgroundColor.greenF(),
                     m_settings.backgroundColor.blueF(), 1.0f);
  m_gl->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (!m_initialized) return;

  QMatrix4x4 globalTransform = toQt(snapshot.globalTransform);

  for (const auto& obj : snapshot.objects) {
    if (!obj.mesh) continue;
    ensureMeshUploaded(obj.mesh);
    auto& b = m_meshBuffers[obj.mesh.get()];

    if (m_settings.renderMode == RenderMode::Wireframe &&
        b->wireframeUploaded) {
      b->edgeVAO->bind();
      m_edgeTech->bindAndSetUniforms(obj, globalTransform, camera, m_settings);
      m_gl->glDrawElements(GL_LINES, static_cast<GLsizei>(b->indexCount),
                           GL_UNSIGNED_INT, nullptr);
      m_edgeTech->release();
      b->edgeVAO->release();

      if (m_settings.vertexType != VertexType::None) {
        b->pointVAO->bind();
        m_pointTech->bindAndSetUniforms(obj, globalTransform, camera,
                                        m_settings);
        m_gl->glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(b->vertexCount));
        m_pointTech->release();
        b->pointVAO->release();
      }
    } else if (b->solidUploaded) {
      IShaderTechnique* tech = nullptr;
      if (m_settings.useTexture && m_texture && m_texture->isCreated() &&
          obj.mesh->hasTexCoords())
        tech = m_textureTech.get();
      else {
        if (m_settings.renderMode == RenderMode::Flat)
          tech = m_flatTech.get();
        else if (m_settings.renderMode == RenderMode::Gouraud)
          tech = m_gouraudTech.get();
        else
          tech = m_phongTech.get();
      }
      if (tech) {
        b->faceVAO->bind();
        if (tech == m_textureTech.get()) m_texture->bind(0);
        tech->bindAndSetUniforms(obj, globalTransform, camera, m_settings);
        m_gl->glDrawElements(GL_TRIANGLES,
                             static_cast<GLsizei>(b->faceIndexCount),
                             GL_UNSIGNED_INT, nullptr);
        tech->release();
        b->faceVAO->release();
      }
    }
  }
  renderFloor(globalTransform, camera);
}

void SceneRenderer::resize(int w, int h) { m_gl->glViewport(0, 0, w, h); }

void SceneRenderer::initFloor() {
  m_floorVAO = std::make_unique<QOpenGLVertexArrayObject>();
  m_floorVAO->create();
  m_floorVBO = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::VertexBuffer);
  m_floorVBO->create();
  m_floorIBO = std::make_unique<QOpenGLBuffer>(QOpenGLBuffer::IndexBuffer);
  m_floorIBO->create();

  std::vector<float> verts;
  std::vector<unsigned int> idx;
  int lines = 20;
  float size = 10.0f;
  unsigned int base = 0;

  for (int i = 0; i <= lines; ++i) {
    float pos = -size + (2.0f * size * i / lines);

    verts.push_back(pos);
    verts.push_back(-1.0f);
    verts.push_back(-size);
    verts.push_back(pos);
    verts.push_back(-1.0f);
    verts.push_back(size);
    idx.push_back(base++);
    idx.push_back(base++);

    verts.push_back(-size);
    verts.push_back(-1.0f);
    verts.push_back(pos);
    verts.push_back(size);
    verts.push_back(-1.0f);
    verts.push_back(pos);
    idx.push_back(base++);
    idx.push_back(base++);
  }

  m_floorVBO->bind();
  m_floorVBO->allocate(verts.data(),
                       static_cast<int>(verts.size() * sizeof(float)));

  m_floorVAO->bind();
  m_floorVBO->bind();
  m_gl->glEnableVertexAttribArray(0);
  m_gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
  m_floorVAO->release();

  m_floorIBO->bind();
  m_floorIBO->allocate(idx.data(),
                       static_cast<int>(idx.size() * sizeof(unsigned int)));

  m_floorIndexCount = idx.size();
  m_floorInitialized = true;
}

void SceneRenderer::renderFloor(const QMatrix4x4& globalTransform,
                                const ICamera& camera) {
  if (!m_floorInitialized || !m_settings.showFloor) return;

  QMatrix4x4 mvp =
      camera.getProjectionMatrix() * camera.getViewMatrix() * globalTransform;

  m_floorVAO->bind();
  m_floorIBO->bind();

  static_cast<EdgeTechnique*>(m_edgeTech.get())
      ->bindWithColor(mvp, QVector3D(0.4f, 0.4f, 0.4f), 0);
  m_gl->glLineWidth(1.0f);

  m_gl->glDrawElements(GL_LINES, static_cast<GLsizei>(m_floorIndexCount),
                       GL_UNSIGNED_INT, nullptr);

  m_edgeTech->release();
  m_floorVAO->release();
}

QImage SceneRenderer::renderRayTraced(const SceneSnapshot& snapshot,
                                      const ICamera& camera,
                                      const RenderSettings& settings) {
  if (snapshot.objects.empty()) return QImage();

  // Проверяем, есть ли хоть один объект с данными
  bool hasData = false;
  for (const auto& obj : snapshot.objects) {
    if (obj.mesh) {
      hasData = true;
      break;
    }
  }
  if (!hasData) return QImage();

  RayTraceSettings rtSettings;
  rtSettings.width = settings.rayTraceWidth;
  rtSettings.height = settings.rayTraceHeight;
  rtSettings.maxDepth = settings.rayTraceDepth;
  rtSettings.showFloor = settings.showFloor;

  RTMaterial rtMat;
  rtMat.color = QVector3D(settings.material.baseColor.redF(),
                          settings.material.baseColor.greenF(),
                          settings.material.baseColor.blueF());
  rtMat.reflectivity = settings.material.reflectivity;
  rtMat.transparency = settings.material.transparency;
  rtMat.ior = settings.material.ior;
  rtMat.roughness = settings.material.roughness;
  rtMat.smoothShading = settings.material.smoothShading;

  m_rayTracer.setSceneData(snapshot);
  m_rayTracer.setTransform(QMatrix4x4());
  m_rayTracer.setCamera(camera);
  m_rayTracer.setSettings(settings);
  m_rayTracer.setRayTraceSettings(rtSettings);
  m_rayTracer.setMaterial(rtMat);

  return m_rayTracer.render();
}

}  // namespace s21