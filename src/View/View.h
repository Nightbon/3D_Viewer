#pragma once
#include <QApplication>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QMouseEvent>
#include <QOpenGLExtraFunctions>
#include <QOpenGLWidget>
#include <QPainter>
#include <QSettings>
#include <QSpinBox>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWheelEvent>

#include "../Controller/Controller.h"
#include "GifRecorder.h"
#include "SettingsManager.h"
#include "TransformState.h"

namespace s21 {

class QtGLWidget : public QOpenGLWidget, protected QOpenGLExtraFunctions {
  Q_OBJECT
 public:
  QtGLWidget(ViewerController& controller, IRenderer& renderer, ICamera& camera,
             QWidget* parent = nullptr);
  void resetMeshState();

 protected:
  void initializeGL() override;
  void paintGL() override;
  void resizeGL(int w, int h) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;

 private:
  ViewerController& m_controller;
  IRenderer& m_renderer;
  ICamera& m_camera;
  QPoint m_lastMousePos;
  QTimer* m_timer;
  bool m_meshUploaded = false;
};

class QtView : public QObject {
  Q_OBJECT
 public:
  QtView(int& argc, char** argv, ViewerController& controller,
         IRenderer& renderer, ICamera& camera);
  void run();
  void updateFileInfo(const QString& filepath);

 private slots:
  void onLoadFileClicked();
  void onMoveChanged();
  void onRotateChanged();
  void onScaleChanged();
  void onResetTransform();
  void onSettingsChanged();
  void onScreenshotClicked();
  void onRecordGifClicked();
  void onEdgeColorClicked();
  void onVertexColorClicked();
  void onBackgroundColorClicked();
  void onFaceColorClicked();
  void onRenderModeChanged(int index);
  void onLoadTextureClicked();
  void onUnloadTextureClicked();
  void onSaveUVMapClicked();
  void onActiveObjectChanged(int index);
  void onRemoveObjectClicked();
  void onRayTraceClicked();
  void syncMaterialUI();
  void onAddSphere();
  void onAddCylinder();
  void onAddCone();
  void onAddCube();
  void onAddPyramid();
  void refreshObjectList();

 private:
  void setupUI();
  void setupFileBlock(QVBoxLayout* layout);
  void setupRenderModeBlock(QVBoxLayout* layout);
  void setupTransformBlock(QVBoxLayout* layout);
  void setupProjectionBlock(QVBoxLayout* layout);
  void setupEdgesBlock(QVBoxLayout* layout);
  void setupVerticesBlock(QVBoxLayout* layout);
  void setupLightingBlock(QVBoxLayout* layout);
  void setupBackgroundBlock(QVBoxLayout* layout);
  void setupActionBlock(QVBoxLayout* layout);
  void setupGifRecorder();
  void setupTextureBlock(QVBoxLayout* layout);
  void setupSceneBlock(QVBoxLayout* layout);
  void onSavePreview();

  void createXYZSliders(QVBoxLayout* parent, const QString& title,
                        XYZSliders& sliders, float min, float max, float def,
                        const QString& suffix = "");

  static int floatToSlider(float val, float min, float max) {
    return qRound((val - min) / (max - min) * 100);
  }
  static float sliderToFloat(int val, float min, float max) {
    return min + val * (max - min) / 100.0f;
  }

  void setupRayTraceBlock(QVBoxLayout* layout);
  // Ray Trace
  QPushButton* m_btnRayTrace = nullptr;
  QSpinBox* m_rtWidthSpin = nullptr;
  QSpinBox* m_rtHeightSpin = nullptr;
  QSlider* m_rtDepthSlider = nullptr;
  QCheckBox* m_chkShowFloor = nullptr;

  // Material
  QPushButton* m_btnFaceColor = nullptr;
  QColor m_faceColor;
  QSlider* m_reflectivitySlider = nullptr;
  QSlider* m_transparencySlider = nullptr;
  QSlider* m_iorSlider = nullptr;
  QSlider* m_roughnessSlider = nullptr;
  QCheckBox* m_chkSmoothShading = nullptr;

  std::unique_ptr<QApplication> m_app;
  std::unique_ptr<QMainWindow> m_window;
  QtGLWidget* m_glWidget;

  ViewerController& m_controller;
  IRenderer& m_renderer;
  ICamera& m_camera;
  RenderSettings m_settings;

  SettingsManager m_settingsManager;
  TransformState m_transformState;

  GifRecorder m_gifRecorder;

  // File
  QLabel* m_lblFilename = nullptr;
  QLabel* m_lblVertices = nullptr;
  QLabel* m_lblEdges = nullptr;
  QLabel* m_lblFaces = nullptr;

  // Render Mode
  QComboBox* m_comboRenderMode = nullptr;

  // Transform
  XYZSliders m_moveSliders;
  XYZSliders m_rotSliders;
  QSlider* m_scaleSlider = nullptr;
  QLabel* m_scaleLabel = nullptr;
  QPushButton* m_btnReset = nullptr;

  QTabWidget* m_tabWidget = nullptr;

  // Projection
  QComboBox* m_comboProjection = nullptr;

  // Edges
  QComboBox* m_comboEdgeType = nullptr;
  QPushButton* m_btnEdgeColor = nullptr;
  QColor m_edgeColor;
  QSlider* m_edgeThicknessSlider = nullptr;

  // Vertices
  QComboBox* m_comboVertexType = nullptr;
  QPushButton* m_btnVertexColor = nullptr;
  QColor m_vertexColor;
  QSlider* m_vertexSizeSlider = nullptr;

  // Lighting
  std::array<LightUI, 5> m_lightUIs;

  QSlider* m_ambientSlider = nullptr;
  QSlider* m_specularSlider = nullptr;
  QSlider* m_shininessSlider = nullptr;

  // Background
  QPushButton* m_btnBackgroundColor = nullptr;
  QColor m_backgroundColor;

  // Actions
  QPushButton* m_btnScreenshot = nullptr;
  QPushButton* m_btnRecordGif = nullptr;
  QPushButton* m_btnPreview = nullptr;
  float m_previewStartYaw = 0.0f;
  bool m_isPreviewMode = false;

  // Texturing
  QPushButton* m_btnLoadTexture = nullptr;
  QPushButton* m_btnUnloadTexture = nullptr;
  QPushButton* m_btnSaveUVMap = nullptr;
  QLabel* m_lblTextureStatus = nullptr;

  // Scene Objects
  QComboBox* m_comboObjects = nullptr;
  QPushButton* m_btnRemoveObject = nullptr;
  QCheckBox* m_cbGlobalTransform = nullptr;
};

class GLWidgetFrameProvider : public FrameProvider {
 public:
  explicit GLWidgetFrameProvider(QtGLWidget* widget) : m_widget(widget) {}
  QImage captureFrame() const override { return m_widget->grabFramebuffer(); }

 private:
  QtGLWidget* m_widget;
};

}  // namespace s21