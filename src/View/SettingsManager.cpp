#include "SettingsManager.h"

namespace s21 {

SettingsManager::SettingsManager(const QString& org, const QString& app,
                                 QObject* parent)
    : QObject(parent), m_org(org), m_app(app) {}

RenderSettings SettingsManager::load() const {
  QSettings s(m_org, m_app);
  RenderSettings settings;

  // Режим рендера
  settings.renderMode =
      static_cast<RenderMode>(s.value(Keys::renderMode, 0).toInt());

  // Настройки граней
  settings.edgeType = static_cast<EdgeType>(s.value(Keys::edgeType, 0).toInt());
  settings.edgeColor =
      s.value(Keys::edgeColor, QColor(255, 255, 255)).value<QColor>();
  settings.edgeThickness = s.value(Keys::edgeThickness, 1.0f).toFloat();

  // Настройки вершин
  settings.vertexType =
      static_cast<VertexType>(s.value(Keys::vertexType, 0).toInt());
  settings.vertexColor =
      s.value(Keys::vertexColor, QColor(255, 255, 255)).value<QColor>();
  settings.vertexSize = s.value(Keys::vertexSize, 5.0f).toFloat();

  // Освещение и поверхности
  settings.pointLights[0].color =
      s.value(Keys::lightColor, QColor(255, 255, 255)).value<QColor>();

  float lightX = s.value(Keys::lightPosX, 5.0f).toFloat();
  float lightY = s.value(Keys::lightPosY, 5.0f).toFloat();
  float lightZ = s.value(Keys::lightPosZ, 5.0f).toFloat();

  // Загружаем до 5 источников
  for (int i = 0; i < RenderSettings::MAX_LIGHTS; ++i) {
    QString prefix = QString(Keys::pointLightPrefix) + QString::number(i) + "_";
    settings.pointLights[i].position =
        QVector3D(s.value(prefix + "posX", (i == 0 ? 5.0f : 0.0f)).toFloat(),
                  s.value(prefix + "posY", (i == 0 ? 5.0f : 0.0f)).toFloat(),
                  s.value(prefix + "posZ", (i == 0 ? 5.0f : 0.0f)).toFloat());
    settings.pointLights[i].color =
        s.value(prefix + "color", QColor(255, 255, 255)).value<QColor>();
    settings.pointLights[i].intensity =
        s.value(prefix + "intensity", 1.0f).toFloat();
    settings.pointLights[i].enabled =
        s.value(prefix + "enabled", i == 0).toBool();
  }

  settings.ambientStrength = s.value(Keys::ambientStrength, 0.1f).toFloat();
  settings.specularStrength = s.value(Keys::specularStrength, 0.5f).toFloat();
  settings.shininess = s.value(Keys::shininess, 32.0f).toFloat();

  // Общие настройки
  settings.backgroundColor =
      s.value(Keys::background, QColor(38, 38, 38)).value<QColor>();
  settings.projection =
      static_cast<ProjectionType>(s.value(Keys::projection, 0).toInt());

  settings.material.reflectivity = s.value(Keys::reflectivity, 0.0f).toFloat();
  settings.material.transparency = s.value(Keys::transparency, 0.0f).toFloat();
  settings.material.ior = s.value(Keys::ior, 1.5f).toFloat();
  settings.material.roughness = s.value(Keys::roughness, 0.0f).toFloat();
  settings.material.smoothShading = s.value(Keys::smoothShading, true).toBool();
  settings.showFloor = s.value(Keys::showFloor, true).toBool();
  settings.rayTraceWidth = s.value(Keys::rayTraceWidth, 800).toInt();
  settings.rayTraceHeight = s.value(Keys::rayTraceHeight, 600).toInt();
  settings.rayTraceDepth = s.value(Keys::rayTraceDepth, 3).toInt();

  return settings;
}

void SettingsManager::save(const RenderSettings& settings) {
  QSettings s(m_org, m_app);

  s.setValue(Keys::renderMode, static_cast<int>(settings.renderMode));

  s.setValue(Keys::edgeType, static_cast<int>(settings.edgeType));
  s.setValue(Keys::edgeColor, settings.edgeColor);
  s.setValue(Keys::edgeThickness, settings.edgeThickness);

  s.setValue(Keys::vertexType, static_cast<int>(settings.vertexType));
  s.setValue(Keys::vertexColor, settings.vertexColor);
  s.setValue(Keys::vertexSize, settings.vertexSize);

  s.setValue(Keys::lightColor, settings.pointLights[0].color);
  s.setValue(Keys::lightPosX, settings.pointLights[0].position.x());
  s.setValue(Keys::lightPosY, settings.pointLights[0].position.y());
  s.setValue(Keys::lightPosZ, settings.pointLights[0].position.z());

  for (int i = 0; i < RenderSettings::MAX_LIGHTS; ++i) {
    QString prefix = QString(Keys::pointLightPrefix) + QString::number(i) + "_";
    s.setValue(prefix + "posX", settings.pointLights[i].position.x());
    s.setValue(prefix + "posY", settings.pointLights[i].position.y());
    s.setValue(prefix + "posZ", settings.pointLights[i].position.z());
    s.setValue(prefix + "color", settings.pointLights[i].color);
    s.setValue(prefix + "intensity", settings.pointLights[i].intensity);
    s.setValue(prefix + "enabled", settings.pointLights[i].enabled);
  }

  s.setValue(Keys::ambientStrength, settings.ambientStrength);
  s.setValue(Keys::specularStrength, settings.specularStrength);
  s.setValue(Keys::shininess, settings.shininess);

  s.setValue(Keys::background, settings.backgroundColor);
  s.setValue(Keys::projection, static_cast<int>(settings.projection));

  s.setValue(Keys::reflectivity, settings.material.reflectivity);
  s.setValue(Keys::transparency, settings.material.transparency);
  s.setValue(Keys::ior, settings.material.ior);
  s.setValue(Keys::roughness, settings.material.roughness);
  s.setValue(Keys::smoothShading, settings.material.smoothShading);
  s.setValue(Keys::showFloor, settings.showFloor);
  s.setValue(Keys::rayTraceWidth, settings.rayTraceWidth);
  s.setValue(Keys::rayTraceHeight, settings.rayTraceHeight);
  s.setValue(Keys::rayTraceDepth, settings.rayTraceDepth);
}

RenderSettings SettingsManager::defaults() {
  RenderSettings s;
  s.renderMode = RenderMode::Wireframe;
  s.edgeType = EdgeType::Solid;
  s.edgeColor = QColor(255, 255, 255);
  s.edgeThickness = 1.0f;
  s.vertexType = VertexType::None;
  s.vertexColor = QColor(255, 255, 255);
  s.vertexSize = 5.0f;
  s.ambientStrength = 0.1f;
  s.specularStrength = 0.5f;
  s.shininess = 32.0f;
  s.pointLights[0].color = QColor(255, 255, 255);
  s.pointLights[0].position = QVector3D(5.0f, 5.0f, 5.0f);
  for (int i = 0; i < RenderSettings::MAX_LIGHTS; ++i) {
    s.pointLights[i] = PointLight{};
    s.pointLights[i].enabled = (i == 0);
  }
  s.backgroundColor = QColor(38, 38, 38);
  s.projection = ProjectionType::Central;

  s.material.reflectivity = 0.0f;
  s.material.transparency = 0.0f;
  s.material.ior = 1.5f;
  s.material.roughness = 0.0f;
  s.material.smoothShading = true;
  s.material.baseColor = QColor(200, 200, 200);
  s.showFloor = true;
  s.rayTraceWidth = 800;
  s.rayTraceHeight = 600;
  s.rayTraceDepth = 3;
  return s;
}

}  // namespace s21