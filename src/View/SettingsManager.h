#pragma once
#include <QColor>
#include <QObject>
#include <QSettings>

#include "TypesView.h"

namespace s21 {

class SettingsManager : public QObject {
  Q_OBJECT
 public:
  explicit SettingsManager(const QString& org = "s21",
                           const QString& app = "WireframeVisualizer",
                           QObject* parent = nullptr);

  RenderSettings load() const;
  void save(const RenderSettings& settings);

  static RenderSettings defaults();

 private:
  QString m_org;
  QString m_app;

  // Ключи для QSettings
  struct Keys {
    // Render Mode
    static constexpr const char* renderMode = "renderMode";

    // Edge settings
    static constexpr const char* edgeType = "edgeType";
    static constexpr const char* edgeColor = "edgeColor";
    static constexpr const char* edgeThickness = "edgeThickness";

    // Vertex settings
    static constexpr const char* vertexType = "vertexType";
    static constexpr const char* vertexColor = "vertexColor";
    static constexpr const char* vertexSize = "vertexSize";

    // Lighting & Face settings  <-- УБРАН faceColor
    static constexpr const char* lightColor = "lightColor";
    static constexpr const char* lightPosX = "lightPosX";
    static constexpr const char* lightPosY = "lightPosY";
    static constexpr const char* lightPosZ = "lightPosZ";
    static constexpr const char* ambientStrength = "ambientStrength";
    static constexpr const char* specularStrength = "specularStrength";
    static constexpr const char* shininess = "shininess";

    // Multiple point lights
    static constexpr const char* pointLightPrefix = "pointLight_";

    // General settings
    static constexpr const char* background = "background";
    static constexpr const char* projection = "projection";

    static constexpr const char* reflectivity = "reflectivity";
    static constexpr const char* transparency = "transparency";
    static constexpr const char* ior = "ior";
    static constexpr const char* roughness = "roughness";
    static constexpr const char* smoothShading = "smoothShading";
    static constexpr const char* showFloor = "showFloor";
    static constexpr const char* rayTraceWidth = "rayTraceWidth";
    static constexpr const char* rayTraceHeight = "rayTraceHeight";
    static constexpr const char* rayTraceDepth = "rayTraceDepth";
  };
};

}  // namespace s21