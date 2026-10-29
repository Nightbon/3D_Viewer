#include "View.h"

#include <QMessageBox>
#include <array>

namespace s21 {

constexpr float CAMERA_SPEED = 0.5f;

// ==================== QtGLWidget ====================

QtGLWidget::QtGLWidget(ViewerController& controller, IRenderer& renderer,
                       ICamera& camera, QWidget* parent)
    : QOpenGLWidget(parent),
      m_controller(controller),
      m_renderer(renderer),
      m_camera(camera) {
  setFocusPolicy(Qt::StrongFocus);
  m_timer = new QTimer(this);
  connect(m_timer, &QTimer::timeout, this, [this]() { update(); });
  m_timer->start(16);
}

void QtGLWidget::initializeGL() {
  initializeOpenGLFunctions();
  m_renderer.setGLFunctions(this);
  m_renderer.init();
}

void QtGLWidget::resetMeshState() { m_meshUploaded = false; }

void QtGLWidget::paintGL() {
  SceneSnapshot snapshot = m_controller.getSceneSnapshot();
  m_renderer.render(snapshot, m_camera);
}

void QtGLWidget::resizeGL(int w, int h) { m_renderer.resize(w, h); }

void QtGLWidget::mousePressEvent(QMouseEvent* event) {
  m_lastMousePos = event->pos();
}

void QtGLWidget::mouseMoveEvent(QMouseEvent* event) {
  QPoint delta = event->pos() - m_lastMousePos;
  m_lastMousePos = event->pos();
  if (event->buttons() & Qt::RightButton) {
    m_camera.rotate(delta.x() * CAMERA_SPEED, -delta.y() * CAMERA_SPEED);
  }
}

void QtGLWidget::wheelEvent(QWheelEvent* event) {
  m_camera.zoom(event->angleDelta().y() / 120.0f * CAMERA_SPEED);
}

void QtGLWidget::keyPressEvent(QKeyEvent* event) {
  switch (event->key()) {
    case Qt::Key_W:
      m_camera.move(0, 0, -CAMERA_SPEED);
      break;
    case Qt::Key_S:
      m_camera.move(0, 0, CAMERA_SPEED);
      break;
    case Qt::Key_A:
      m_camera.move(-CAMERA_SPEED, 0, 0);
      break;
    case Qt::Key_D:
      m_camera.move(CAMERA_SPEED, 0, 0);
      break;
    case Qt::Key_Q:
      m_camera.move(0, -CAMERA_SPEED, 0);
      break;
    case Qt::Key_E:
      m_camera.move(0, CAMERA_SPEED, 0);
      break;
  }
}

// ==================== QtView ====================

QtView::QtView(int& argc, char** argv, ViewerController& controller,
               IRenderer& renderer, ICamera& camera)
    : m_app(std::make_unique<QApplication>(argc, argv)),
      m_window(std::make_unique<QMainWindow>()),
      m_controller(controller),
      m_renderer(renderer),
      m_camera(camera),
      m_gifRecorder(this),
      m_settingsManager("s21", "WireframeVisualizer") {
  m_settings = m_settingsManager.load();
  m_window->setWindowTitle("3DViewer v2.0");
  m_window->resize(1024, 768);
  setupUI();
  setupGifRecorder();
}

void QtView::setupUI() {
  QWidget* central = new QWidget();
  QHBoxLayout* mainLayout = new QHBoxLayout(central);

  m_glWidget = new QtGLWidget(m_controller, m_renderer, m_camera, central);
  mainLayout->addWidget(m_glWidget, 3);

  QVBoxLayout* control = new QVBoxLayout();
  control->setSpacing(6);
  control->setContentsMargins(8, 8, 8, 8);
  mainLayout->addLayout(control, 1);

  // File
  setupFileBlock(control);

  // Табы для настроек
  m_tabWidget = new QTabWidget();
  m_tabWidget->setDocumentMode(true);  // компактный вид

  // Вкладка 1: Геометрия
  QWidget* tab1 = new QWidget();
  QVBoxLayout* l1 = new QVBoxLayout(tab1);
  l1->setContentsMargins(8, 8, 8, 0);
  setupSceneBlock(l1);
  setupTransformBlock(l1);
  setupProjectionBlock(l1);
  l1->addStretch();
  m_tabWidget->addTab(tab1, "Transform");

  // Вкладка 2: Внешний вид
  QWidget* tab2 = new QWidget();
  QVBoxLayout* l2 = new QVBoxLayout(tab2);
  l2->setContentsMargins(8, 8, 8, 0);
  setupRenderModeBlock(l2);
  setupTextureBlock(l2);
  setupEdgesBlock(l2);
  setupVerticesBlock(l2);
  setupBackgroundBlock(l2);
  l2->addStretch();
  m_tabWidget->addTab(tab2, "Style");

  // Вкладка 3: Освещение и материал
  QWidget* tab3 = new QWidget();
  QVBoxLayout* l3 = new QVBoxLayout(tab3);
  l3->setContentsMargins(8, 8, 8, 0);
  setupLightingBlock(l3);
  l3->addStretch();
  m_tabWidget->addTab(tab3, "Lighting");

  // Вкладка 4: Ray Tracing
  QWidget* tab4 = new QWidget();
  QVBoxLayout* l4 = new QVBoxLayout(tab4);
  l4->setContentsMargins(8, 8, 8, 0);
  setupRayTraceBlock(l4);
  l4->addStretch();
  m_tabWidget->addTab(tab4, "Ray Trace");

  control->addWidget(m_tabWidget, 1);

  setupActionBlock(control);

  m_window->setCentralWidget(central);
  m_renderer.setSettings(m_settings);
}

// ==================== UI Blocks ====================

void QtView::setupRayTraceBlock(QVBoxLayout* layout) {
  QGroupBox* settingsGroup = new QGroupBox("Ray Trace Settings");
  QVBoxLayout* sbox = new QVBoxLayout(settingsGroup);

  QHBoxLayout* resRow = new QHBoxLayout();
  resRow->addWidget(new QLabel("Resolution:"));
  m_rtWidthSpin = new QSpinBox();
  m_rtWidthSpin->setRange(100, 1920);
  m_rtWidthSpin->setValue(m_settings.rayTraceWidth);
  m_rtHeightSpin = new QSpinBox();
  m_rtHeightSpin->setRange(100, 1440);
  m_rtHeightSpin->setValue(m_settings.rayTraceHeight);
  resRow->addWidget(m_rtWidthSpin);
  resRow->addWidget(new QLabel("x"));
  resRow->addWidget(m_rtHeightSpin);
  sbox->addLayout(resRow);

  QHBoxLayout* depthRow = new QHBoxLayout();
  depthRow->addWidget(new QLabel("Max Depth:"));
  m_rtDepthSlider = new QSlider(Qt::Horizontal);
  m_rtDepthSlider->setRange(1, 10);
  m_rtDepthSlider->setValue(m_settings.rayTraceDepth);
  depthRow->addWidget(m_rtDepthSlider);
  sbox->addLayout(depthRow);

  m_chkShowFloor = new QCheckBox("Show Floor");
  m_chkShowFloor->setChecked(m_settings.showFloor);
  sbox->addWidget(m_chkShowFloor);

  layout->addWidget(settingsGroup);

  QGroupBox* matGroup = new QGroupBox("Material");
  QVBoxLayout* mbox = new QVBoxLayout(matGroup);

  // Face Color
  QHBoxLayout* faceColorRow = new QHBoxLayout();
  faceColorRow->addWidget(new QLabel("Face Color:"));
  m_faceColor = m_settings.material.baseColor;
  m_btnFaceColor = new QPushButton();
  m_btnFaceColor->setStyleSheet(
      QString("background-color: %1; min-width: 24px; min-height: 24px;")
          .arg(m_faceColor.name()));
  faceColorRow->addWidget(m_btnFaceColor);
  mbox->addLayout(faceColorRow);

  auto makeSlider = [&](const QString& label, QSlider*& slider, int val,
                        int max = 100) {
    QHBoxLayout* row = new QHBoxLayout();
    row->addWidget(new QLabel(label));
    slider = new QSlider(Qt::Horizontal);
    slider->setRange(0, max);
    slider->setValue(val);
    row->addWidget(slider);
    mbox->addLayout(row);
  };

  makeSlider("Reflectivity:", m_reflectivitySlider,
             static_cast<int>(m_settings.material.reflectivity * 100));
  makeSlider("Transparency:", m_transparencySlider,
             static_cast<int>(m_settings.material.transparency * 100));
  makeSlider("IOR:", m_iorSlider,
             static_cast<int>((m_settings.material.ior - 1.0f) * 10), 20);
  makeSlider("Roughness:", m_roughnessSlider,
             static_cast<int>(m_settings.material.roughness * 100));

  m_chkSmoothShading = new QCheckBox("Smooth Shading (Soft Edges)");
  m_chkSmoothShading->setChecked(m_settings.material.smoothShading);
  mbox->addWidget(m_chkSmoothShading);

  layout->addWidget(matGroup);

  m_btnRayTrace = new QPushButton("Ray Trace Render");
  m_btnRayTrace->setStyleSheet("font-weight: bold; padding: 8px;");
  layout->addWidget(m_btnRayTrace);

  connect(m_btnRayTrace, &QPushButton::clicked, this,
          &QtView::onRayTraceClicked);
  connect(m_chkShowFloor, &QCheckBox::toggled, this,
          &QtView::onSettingsChanged);
  connect(m_reflectivitySlider, &QSlider::valueChanged, this,
          &QtView::onSettingsChanged);
  connect(m_transparencySlider, &QSlider::valueChanged, this,
          &QtView::onSettingsChanged);
  connect(m_iorSlider, &QSlider::valueChanged, this,
          &QtView::onSettingsChanged);
  connect(m_roughnessSlider, &QSlider::valueChanged, this,
          &QtView::onSettingsChanged);
  connect(m_chkSmoothShading, &QCheckBox::toggled, this,
          &QtView::onSettingsChanged);
  connect(m_rtWidthSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &QtView::onSettingsChanged);
  connect(m_rtHeightSpin, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &QtView::onSettingsChanged);
  connect(m_rtDepthSlider, &QSlider::valueChanged, this,
          &QtView::onSettingsChanged);
  connect(m_btnFaceColor, &QPushButton::clicked, this,
          &QtView::onFaceColorClicked);
}

void QtView::setupSceneBlock(QVBoxLayout* layout) {
  QGroupBox* group = new QGroupBox("Scene Objects");
  QVBoxLayout* box = new QVBoxLayout(group);

  QHBoxLayout* row1 = new QHBoxLayout();
  m_comboObjects = new QComboBox();
  m_btnRemoveObject = new QPushButton("Remove");
  m_btnRemoveObject->setEnabled(false);

  row1->addWidget(new QLabel("Active:"));
  row1->addWidget(m_comboObjects, 1);
  row1->addWidget(m_btnRemoveObject);
  box->addLayout(row1);

  m_cbGlobalTransform = new QCheckBox("Transform Entire Scene (Global)");
  box->addWidget(m_cbGlobalTransform);

  layout->addWidget(group);

  connect(m_comboObjects, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &QtView::onActiveObjectChanged);
  connect(m_btnRemoveObject, &QPushButton::clicked, this,
          &QtView::onRemoveObjectClicked);

  // Primitives
  QGroupBox* primGroup = new QGroupBox("Add Primitive");
  QGridLayout* primGrid = new QGridLayout(primGroup);
  QPushButton* btnSphere = new QPushButton("Sphere");
  QPushButton* btnCylinder = new QPushButton("Cylinder");
  QPushButton* btnCone = new QPushButton("Cone");
  QPushButton* btnCube = new QPushButton("Cube");
  QPushButton* btnPyramid = new QPushButton("Pyramid");

  primGrid->addWidget(btnSphere, 0, 0);
  primGrid->addWidget(btnCylinder, 0, 1);
  primGrid->addWidget(btnCone, 1, 0);
  primGrid->addWidget(btnCube, 1, 1);
  primGrid->addWidget(btnPyramid, 2, 0, 1, 2);

  layout->addWidget(primGroup);

  connect(btnSphere, &QPushButton::clicked, this, &QtView::onAddSphere);
  connect(btnCylinder, &QPushButton::clicked, this, &QtView::onAddCylinder);
  connect(btnCone, &QPushButton::clicked, this, &QtView::onAddCone);
  connect(btnCube, &QPushButton::clicked, this, &QtView::onAddCube);
  connect(btnPyramid, &QPushButton::clicked, this, &QtView::onAddPyramid);
}

void QtView::setupFileBlock(QVBoxLayout* layout) {
  QGroupBox* group = new QGroupBox("File");
  QVBoxLayout* box = new QVBoxLayout(group);
  QHBoxLayout* row = new QHBoxLayout();
  QPushButton* btnLoad = new QPushButton("Load .obj");
  m_lblFilename = new QLabel("Not loaded");
  m_lblFilename->setWordWrap(true);
  row->addWidget(btnLoad);
  row->addWidget(m_lblFilename, 1);
  box->addLayout(row);

  QHBoxLayout* info = new QHBoxLayout();
  m_lblVertices = new QLabel("V: 0");
  m_lblEdges = new QLabel("E: 0");
  m_lblFaces = new QLabel("F: 0");
  info->addWidget(m_lblVertices);
  info->addWidget(m_lblEdges);
  info->addWidget(m_lblFaces);
  box->addLayout(info);

  layout->addWidget(group);
  connect(btnLoad, &QPushButton::clicked, this, &QtView::onLoadFileClicked);
}

void QtView::setupRenderModeBlock(QVBoxLayout* layout) {
  QGroupBox* group = new QGroupBox("Render Mode");
  QVBoxLayout* box = new QVBoxLayout(group);

  m_comboRenderMode = new QComboBox();
  m_comboRenderMode->addItem("Wireframe",
                             static_cast<int>(RenderMode::Wireframe));
  m_comboRenderMode->addItem("Flat Shading",
                             static_cast<int>(RenderMode::Flat));
  m_comboRenderMode->addItem("Gouraud Shading",
                             static_cast<int>(RenderMode::Gouraud));
  m_comboRenderMode->addItem("Phong Shading",
                             static_cast<int>(RenderMode::Phong));

  // Устанавливаем текущий режим
  int currentMode = static_cast<int>(m_settings.renderMode);
  m_comboRenderMode->setCurrentIndex(currentMode);

  box->addWidget(m_comboRenderMode);
  layout->addWidget(group);

  connect(m_comboRenderMode,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &QtView::onRenderModeChanged);
}

void QtView::setupTextureBlock(QVBoxLayout* layout) {
  QGroupBox* group = new QGroupBox("Texture Mapping");
  QVBoxLayout* box = new QVBoxLayout(group);

  QHBoxLayout* row = new QHBoxLayout();
  m_btnLoadTexture = new QPushButton("Load");
  m_btnUnloadTexture = new QPushButton("Unload");
  m_btnSaveUVMap = new QPushButton("Save UV");

  m_btnLoadTexture->setEnabled(false);
  m_btnUnloadTexture->setEnabled(false);
  m_btnSaveUVMap->setEnabled(false);

  row->addWidget(m_btnLoadTexture);
  row->addWidget(m_btnUnloadTexture);
  row->addWidget(m_btnSaveUVMap);
  box->addLayout(row);

  m_lblTextureStatus = new QLabel("No UV map");
  m_lblTextureStatus->setWordWrap(true);
  box->addWidget(m_lblTextureStatus);

  layout->addWidget(group);

  connect(m_btnLoadTexture, &QPushButton::clicked, this,
          &QtView::onLoadTextureClicked);
  connect(m_btnUnloadTexture, &QPushButton::clicked, this,
          &QtView::onUnloadTextureClicked);
  connect(m_btnSaveUVMap, &QPushButton::clicked, this,
          &QtView::onSaveUVMapClicked);
}

void QtView::createXYZSliders(QVBoxLayout* parent, const QString& title,
                              XYZSliders& s, float min, float max, float def,
                              const QString& suffix) {
  QGroupBox* group = new QGroupBox(title);
  QVBoxLayout* box = new QVBoxLayout(group);

  auto makeAxis = [&](const QString& name, QSlider*& slider, QLabel*& lbl) {
    QHBoxLayout* row = new QHBoxLayout();
    row->addWidget(new QLabel(name));
    slider = new QSlider(Qt::Horizontal);
    slider->setRange(0, 100);
    slider->setValue(floatToSlider(def, min, max));
    lbl = new QLabel(QString::number(def, 'f', 1) + suffix);
    lbl->setMinimumWidth(40);
    row->addWidget(slider, 1);
    row->addWidget(lbl);
    box->addLayout(row);

    connect(
        slider, &QSlider::valueChanged, this,
        [this, slider, lbl, min, max, suffix]() {
          lbl->setText(QString::number(sliderToFloat(slider->value(), min, max),
                                       'f', 1) +
                       suffix);
        });
  };

  makeAxis("X", s.x, s.lblX);
  makeAxis("Y", s.y, s.lblY);
  makeAxis("Z", s.z, s.lblZ);
  parent->addWidget(group);
}

void QtView::setupTransformBlock(QVBoxLayout* layout) {
  // Move
  createXYZSliders(layout, "Move", m_moveSliders, -50, 50, 0);
  connect(m_moveSliders.x, &QSlider::valueChanged, this,
          &QtView::onMoveChanged);
  connect(m_moveSliders.y, &QSlider::valueChanged, this,
          &QtView::onMoveChanged);
  connect(m_moveSliders.z, &QSlider::valueChanged, this,
          &QtView::onMoveChanged);

  // Rotate
  createXYZSliders(layout, "Rotate", m_rotSliders, -180, 180, 0, "°");
  connect(m_rotSliders.x, &QSlider::valueChanged, this,
          &QtView::onRotateChanged);
  connect(m_rotSliders.y, &QSlider::valueChanged, this,
          &QtView::onRotateChanged);
  connect(m_rotSliders.z, &QSlider::valueChanged, this,
          &QtView::onRotateChanged);

  // Scale
  QGroupBox* scaleGroup = new QGroupBox("Scale");
  QHBoxLayout* scaleBox = new QHBoxLayout(scaleGroup);
  m_scaleSlider = new QSlider(Qt::Horizontal);
  m_scaleSlider->setRange(10, 500);
  m_scaleSlider->setValue(100);
  m_scaleLabel = new QLabel("1.0x");
  m_scaleLabel->setMinimumWidth(40);
  scaleBox->addWidget(m_scaleSlider);
  scaleBox->addWidget(m_scaleLabel);
  layout->addWidget(scaleGroup);

  connect(m_scaleSlider, &QSlider::valueChanged, this, &QtView::onScaleChanged);

  // Reset
  m_btnReset = new QPushButton("Reset Transform");
  layout->addWidget(m_btnReset);
  connect(m_btnReset, &QPushButton::clicked, this, &QtView::onResetTransform);
}

void QtView::setupProjectionBlock(QVBoxLayout* layout) {
  QHBoxLayout* row = new QHBoxLayout();
  row->addWidget(new QLabel("Projection:"));
  m_comboProjection = new QComboBox();
  m_comboProjection->addItem("Perspective", 0);
  m_comboProjection->addItem("Orthographic", 1);
  m_comboProjection->setCurrentIndex(
      m_settings.projection == ProjectionType::Central ? 0 : 1);
  row->addWidget(m_comboProjection, 1);
  layout->addLayout(row);

  connect(
      m_comboProjection, QOverload<int>::of(&QComboBox::currentIndexChanged),
      this, [this]() {
        auto type = m_comboProjection->currentIndex() == 0
                        ? ProjectionType::Central
                        : ProjectionType::Parallel;
        if (type == ProjectionType::Central)
          m_camera.setProjectionStrategy(std::make_unique<CentralProjection>());
        else
          m_camera.setProjectionStrategy(
              std::make_unique<ParallelProjection>());
        onSettingsChanged();
      });
}

void QtView::setupEdgesBlock(QVBoxLayout* layout) {
  QGroupBox* group = new QGroupBox("Edges");
  QVBoxLayout* box = new QVBoxLayout(group);

  QHBoxLayout* row1 = new QHBoxLayout();
  row1->addWidget(new QLabel("Type:"));
  m_comboEdgeType = new QComboBox();
  m_comboEdgeType->addItem("Solid", 0);
  m_comboEdgeType->addItem("Dashed", 1);
  m_comboEdgeType->setCurrentIndex(static_cast<int>(m_settings.edgeType));
  row1->addWidget(m_comboEdgeType);
  row1->addStretch();

  m_edgeColor = m_settings.edgeColor;
  m_btnEdgeColor = new QPushButton();
  m_btnEdgeColor->setStyleSheet(
      QString("background-color: %1; min-width: 24px; min-height: 24px;")
          .arg(m_edgeColor.name()));
  row1->addWidget(new QLabel("Color:"));
  row1->addWidget(m_btnEdgeColor);
  box->addLayout(row1);

  QHBoxLayout* row2 = new QHBoxLayout();
  row2->addWidget(new QLabel("Thickness:"));
  m_edgeThicknessSlider = new QSlider(Qt::Horizontal);
  m_edgeThicknessSlider->setRange(1, 100);
  m_edgeThicknessSlider->setValue(
      static_cast<int>(m_settings.edgeThickness * 10));
  row2->addWidget(m_edgeThicknessSlider);
  box->addLayout(row2);

  layout->addWidget(group);

  connect(m_comboEdgeType, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &QtView::onSettingsChanged);
  connect(m_edgeThicknessSlider, &QSlider::valueChanged, this,
          &QtView::onSettingsChanged);
  connect(m_btnEdgeColor, &QPushButton::clicked, this,
          &QtView::onEdgeColorClicked);
}

void QtView::setupVerticesBlock(QVBoxLayout* layout) {
  QGroupBox* group = new QGroupBox("Vertices");
  QVBoxLayout* box = new QVBoxLayout(group);

  QHBoxLayout* row1 = new QHBoxLayout();
  row1->addWidget(new QLabel("Display:"));
  m_comboVertexType = new QComboBox();
  m_comboVertexType->addItem("None", 0);
  m_comboVertexType->addItem("Circle", 1);
  m_comboVertexType->addItem("Square", 2);
  m_comboVertexType->setCurrentIndex(static_cast<int>(m_settings.vertexType));
  row1->addWidget(m_comboVertexType);
  row1->addStretch();

  m_vertexColor = m_settings.vertexColor;
  m_btnVertexColor = new QPushButton();
  m_btnVertexColor->setStyleSheet(
      QString("background-color: %1; min-width: 24px; min-height: 24px;")
          .arg(m_vertexColor.name()));
  row1->addWidget(new QLabel("Color:"));
  row1->addWidget(m_btnVertexColor);
  box->addLayout(row1);

  QHBoxLayout* row2 = new QHBoxLayout();
  row2->addWidget(new QLabel("Size:"));
  m_vertexSizeSlider = new QSlider(Qt::Horizontal);
  m_vertexSizeSlider->setRange(1, 50);
  m_vertexSizeSlider->setValue(static_cast<int>(m_settings.vertexSize));
  row2->addWidget(m_vertexSizeSlider);
  box->addLayout(row2);

  layout->addWidget(group);

  connect(m_comboVertexType,
          QOverload<int>::of(&QComboBox::currentIndexChanged), this,
          &QtView::onSettingsChanged);
  connect(m_vertexSizeSlider, &QSlider::valueChanged, this,
          &QtView::onSettingsChanged);
  connect(m_btnVertexColor, &QPushButton::clicked, this,
          &QtView::onVertexColorClicked);
}

void QtView::setupLightingBlock(QVBoxLayout* layout) {
  QWidget* container = new QWidget();
  QVBoxLayout* box = new QVBoxLayout(container);
  box->setSpacing(4);
  box->setContentsMargins(4, 4, 4, 4);

  // Lights
  for (int i = 0; i < 5; ++i) {
    m_lightUIs[i].group = new QGroupBox(QString("Light %1").arg(i + 1));
    m_lightUIs[i].group->setCheckable(true);
    m_lightUIs[i].group->setChecked(m_settings.pointLights[i].enabled);
    QGridLayout* grid = new QGridLayout(m_lightUIs[i].group);
    grid->setSpacing(2);
    grid->setContentsMargins(4, 4, 4, 4);

    // Position sliders
    m_lightUIs[i].x = new QSlider(Qt::Horizontal);
    m_lightUIs[i].x->setRange(0, 100);
    m_lightUIs[i].x->setValue(TransformState::toSlider(
        m_settings.pointLights[i].position.x(), -50, 50));
    m_lightUIs[i].y = new QSlider(Qt::Horizontal);
    m_lightUIs[i].y->setRange(0, 100);
    m_lightUIs[i].y->setValue(TransformState::toSlider(
        m_settings.pointLights[i].position.y(), -50, 50));
    m_lightUIs[i].z = new QSlider(Qt::Horizontal);
    m_lightUIs[i].z->setRange(0, 100);
    m_lightUIs[i].z->setValue(TransformState::toSlider(
        m_settings.pointLights[i].position.z(), -50, 50));

    grid->addWidget(new QLabel("X"), 0, 0);
    grid->addWidget(m_lightUIs[i].x, 0, 1);
    grid->addWidget(new QLabel("Y"), 1, 0);
    grid->addWidget(m_lightUIs[i].y, 1, 1);
    grid->addWidget(new QLabel("Z"), 2, 0);
    grid->addWidget(m_lightUIs[i].z, 2, 1);

    // Color + intensity row
    m_lightUIs[i].color = m_settings.pointLights[i].color;
    m_lightUIs[i].colorBtn = new QPushButton();
    m_lightUIs[i].colorBtn->setFixedSize(20, 20);
    m_lightUIs[i].colorBtn->setStyleSheet(
        QString("background-color: %1; border: none;")
            .arg(m_lightUIs[i].color.name()));

    m_lightUIs[i].intensity = new QSlider(Qt::Horizontal);
    m_lightUIs[i].intensity->setRange(0, 200);
    m_lightUIs[i].intensity->setValue(
        static_cast<int>(m_settings.pointLights[i].intensity * 100));

    QHBoxLayout* botRow = new QHBoxLayout();
    botRow->addWidget(m_lightUIs[i].colorBtn);
    botRow->addWidget(new QLabel("I:"));
    botRow->addWidget(m_lightUIs[i].intensity, 1);
    grid->addLayout(botRow, 3, 0, 1, 2);

    box->addWidget(m_lightUIs[i].group);

    // Connects
    connect(m_lightUIs[i].group, &QGroupBox::toggled, this,
            &QtView::onSettingsChanged);
    connect(m_lightUIs[i].x, &QSlider::valueChanged, this,
            &QtView::onSettingsChanged);
    connect(m_lightUIs[i].y, &QSlider::valueChanged, this,
            &QtView::onSettingsChanged);
    connect(m_lightUIs[i].z, &QSlider::valueChanged, this,
            &QtView::onSettingsChanged);
    connect(m_lightUIs[i].intensity, &QSlider::valueChanged, this,
            &QtView::onSettingsChanged);
    connect(m_lightUIs[i].colorBtn, &QPushButton::clicked, this, [this, i]() {
      QColor c = QColorDialog::getColor(m_lightUIs[i].color, m_window.get(),
                                        QString("Light %1 Color").arg(i + 1),
                                        QColorDialog::DontUseNativeDialog);
      if (c.isValid()) {
        m_lightUIs[i].color = c;
        m_lightUIs[i].colorBtn->setStyleSheet(
            QString("background-color: %1; border: none;").arg(c.name()));
        onSettingsChanged();
      }
    });
  }

  // Common params
  QHBoxLayout* commonRow = new QHBoxLayout();
  commonRow->addWidget(new QLabel("Amb:"));
  m_ambientSlider = new QSlider(Qt::Horizontal);
  m_ambientSlider->setRange(0, 100);
  m_ambientSlider->setValue(static_cast<int>(m_settings.ambientStrength * 100));
  commonRow->addWidget(m_ambientSlider);

  commonRow->addWidget(new QLabel("Spec:"));
  m_specularSlider = new QSlider(Qt::Horizontal);
  m_specularSlider->setRange(0, 100);
  m_specularSlider->setValue(
      static_cast<int>(m_settings.specularStrength * 100));
  commonRow->addWidget(m_specularSlider);

  commonRow->addWidget(new QLabel("Shin:"));
  m_shininessSlider = new QSlider(Qt::Horizontal);
  m_shininessSlider->setRange(1, 128);
  m_shininessSlider->setValue(static_cast<int>(m_settings.shininess));
  commonRow->addWidget(m_shininessSlider);

  box->addLayout(commonRow);
  box->addStretch();

  layout->addWidget(container);

  connect(m_ambientSlider, &QSlider::valueChanged, this,
          &QtView::onSettingsChanged);
  connect(m_specularSlider, &QSlider::valueChanged, this,
          &QtView::onSettingsChanged);
  connect(m_shininessSlider, &QSlider::valueChanged, this,
          &QtView::onSettingsChanged);
}

void QtView::setupBackgroundBlock(QVBoxLayout* layout) {
  QHBoxLayout* row = new QHBoxLayout();
  row->addWidget(new QLabel("Background:"));
  row->addStretch();

  m_backgroundColor = m_settings.backgroundColor;
  m_btnBackgroundColor = new QPushButton();
  m_btnBackgroundColor->setStyleSheet(
      QString("background-color: %1; min-width: 24px; min-height: 24px;")
          .arg(m_backgroundColor.name()));
  row->addWidget(m_btnBackgroundColor);
  layout->addLayout(row);

  connect(m_btnBackgroundColor, &QPushButton::clicked, this,
          &QtView::onBackgroundColorClicked);
}

void QtView::setupActionBlock(QVBoxLayout* layout) {
  QHBoxLayout* row = new QHBoxLayout();
  m_btnScreenshot = new QPushButton("Screenshot");
  m_btnRecordGif = new QPushButton("Record GIF");
  m_btnPreview = new QPushButton("Save Preview");

  row->addWidget(m_btnScreenshot);
  row->addWidget(m_btnRecordGif);
  row->addWidget(m_btnPreview);
  layout->addLayout(row);

  connect(m_btnScreenshot, &QPushButton::clicked, this,
          &QtView::onScreenshotClicked);
  connect(m_btnRecordGif, &QPushButton::clicked, this,
          &QtView::onRecordGifClicked);
  connect(m_btnPreview, &QPushButton::clicked, this, &QtView::onSavePreview);
}

// ==================== Slots ====================

void QtView::onRayTraceClicked() {
  if (m_controller.getVertexCount() == 0) {
    QMessageBox::warning(m_window.get(), "Warning", "No model loaded");
    return;
  }

  QApplication::setOverrideCursor(Qt::WaitCursor);
  SceneSnapshot snapshot = m_controller.getSceneSnapshot();
  QImage image = m_renderer.renderRayTraced(snapshot, m_camera, m_settings);
  QApplication::restoreOverrideCursor();

  QString file = QFileDialog::getSaveFileName(
      m_window.get(), "Save Ray Traced Image", QDir::homePath(),
      "Images (*.bmp *.jpeg *.png)", nullptr, QFileDialog::DontUseNativeDialog);

  if (!file.isEmpty()) {
    if (!file.endsWith(".bmp", Qt::CaseInsensitive) &&
        !file.endsWith(".jpeg", Qt::CaseInsensitive) &&
        !file.endsWith(".png", Qt::CaseInsensitive))
      file += ".png";
    image.save(file);
  }

  QDialog* preview = new QDialog(m_window.get());
  preview->setWindowTitle("Ray Trace Preview");
  preview->setAttribute(Qt::WA_DeleteOnClose);
  QVBoxLayout* layout = new QVBoxLayout(preview);
  QLabel* label = new QLabel();
  QPixmap pixmap = QPixmap::fromImage(
      image.scaled(std::min(image.width(), 960), std::min(image.height(), 720),
                   Qt::KeepAspectRatio, Qt::SmoothTransformation));
  label->setPixmap(pixmap);
  label->setAlignment(Qt::AlignCenter);
  layout->addWidget(label);
  QPushButton* btnClose = new QPushButton("Close");
  layout->addWidget(btnClose);
  connect(btnClose, &QPushButton::clicked, preview, &QDialog::accept);
  preview->resize(pixmap.size() + QSize(40, 80));
  preview->exec();
}

void QtView::syncMaterialUI() {
  Material mat = m_controller.getActiveMaterial();

  // Отключаем сигналы, чтобы не вызвать onSettingsChanged
  m_reflectivitySlider->blockSignals(true);
  m_transparencySlider->blockSignals(true);
  m_iorSlider->blockSignals(true);
  m_roughnessSlider->blockSignals(true);
  m_chkSmoothShading->blockSignals(true);

  m_reflectivitySlider->setValue(static_cast<int>(mat.reflectivity * 100));
  m_transparencySlider->setValue(static_cast<int>(mat.transparency * 100));
  m_iorSlider->setValue(static_cast<int>((mat.ior - 1.0f) * 100));
  m_roughnessSlider->setValue(static_cast<int>(mat.roughness * 100));
  m_chkSmoothShading->setChecked(mat.smoothShading);

  m_reflectivitySlider->blockSignals(false);
  m_transparencySlider->blockSignals(false);
  m_iorSlider->blockSignals(false);
  m_roughnessSlider->blockSignals(false);
  m_chkSmoothShading->blockSignals(false);
}

void QtView::onLoadFileClicked() {
  QString file = QFileDialog::getOpenFileName(m_window.get(), "Open OBJ",
                                              QDir::homePath(), "OBJ (*.obj)");
  if (!file.isEmpty()) {
    if (m_controller.loadModel(file.toStdString())) {
      // Обновляем список объектов
      refreshObjectList();

      m_btnRemoveObject->setEnabled(true);
      updateFileInfo(file);
      onResetTransform();

      // Проверка текстур для активного объекта
      bool hasUvs = m_controller.hasUvs();
      m_btnLoadTexture->setEnabled(hasUvs);
      m_lblTextureStatus->setText(hasUvs ? "Texture Enabled"
                                         : "Texture Disabled.");
      m_settings.useTexture = false;
      m_settings.textureImage = QImage();
      onSettingsChanged();

    } else {
      m_lblFilename->setText("Failed");
    }
  }
}

void QtView::updateFileInfo(const QString& filepath) {
  QFileInfo info(filepath);
  m_lblFilename->setText(info.fileName());
  m_lblVertices->setText("V: " +
                         QString::number(m_controller.getVertexCount()));
  m_lblEdges->setText("E: " + QString::number(m_controller.getEdgeCount()));
  m_lblFaces->setText("F: " + QString::number(m_controller.getFaceCount()));
}

void QtView::onAddSphere() {
  m_controller.addSphere();
  refreshObjectList();
  m_glWidget->update();
}

void QtView::onAddCylinder() {
  m_controller.addCylinder();
  refreshObjectList();
  m_glWidget->update();
}

void QtView::onAddCone() {
  m_controller.addCone();
  refreshObjectList();
  m_glWidget->update();
}

void QtView::onAddCube() {
  m_controller.addCube();
  refreshObjectList();
  m_glWidget->update();
}

void QtView::onAddPyramid() {
  m_controller.addPyramid();
  refreshObjectList();
  m_glWidget->update();
}

void QtView::refreshObjectList() {
  int previousIndex = m_comboObjects->currentIndex();

  m_comboObjects->blockSignals(true);
  m_comboObjects->clear();

  for (size_t i = 0; i < m_controller.getObjectCount(); ++i) {
    QString type = m_controller.getObjectTypeName(i);
    QString name = QString("Object %1: %2").arg(i + 1).arg(type);
    m_comboObjects->addItem(name);
  }

  int newIndex = static_cast<int>(m_controller.getActiveObjectIndex());
  if (newIndex >= 0 && newIndex < m_comboObjects->count()) {
    m_comboObjects->setCurrentIndex(newIndex);
  } else if (m_comboObjects->count() > 0) {
    m_comboObjects->setCurrentIndex(0);
  }

  m_comboObjects->blockSignals(false);
  m_btnRemoveObject->setEnabled(m_controller.getObjectCount() > 0);

  if (m_comboObjects->currentIndex() >= 0) {
    onActiveObjectChanged(m_comboObjects->currentIndex());
  }
}

// ========== RENDER MODE ==========

void QtView::onRenderModeChanged(int index) {
  m_settings.renderMode = static_cast<RenderMode>(index);
  onSettingsChanged();
}

// ========== AFFINE TRANSFORMS WITH COMMAND PATTERN ==========

void QtView::onMoveChanged() {
  QVector3D newMove(
      TransformState::fromSlider(m_moveSliders.x->value(), -50, 50),
      TransformState::fromSlider(m_moveSliders.y->value(), -50, 50),
      TransformState::fromSlider(m_moveSliders.z->value(), -50, 50));

  QVector3D delta;
  if (m_transformState.updateMove(newMove, delta)) {
    bool isGlobal = m_cbGlobalTransform->isChecked();
    TranslateCommand cmd(m_controller, delta.x(), delta.y(), delta.z(),
                         isGlobal);
    cmd.execute();
    m_glWidget->update();
  }
}

void QtView::onRotateChanged() {
  QVector3D newRotate(
      TransformState::fromSlider(m_rotSliders.x->value(), -180, 180),
      TransformState::fromSlider(m_rotSliders.y->value(), -180, 180),
      TransformState::fromSlider(m_rotSliders.z->value(), -180, 180));

  QVector3D delta;
  if (m_transformState.updateRotate(newRotate, delta)) {
    bool isGlobal = m_cbGlobalTransform->isChecked();
    RotateCommand cmd(m_controller, delta.x(), delta.y(), delta.z(), isGlobal);
    cmd.execute();
    m_glWidget->update();
  }
}

void QtView::onScaleChanged() {
  float newScale = m_scaleSlider->value() / 100.0f;
  m_scaleLabel->setText(QString::number(newScale, 'f', 2) + "x");

  float ratio;
  if (m_transformState.updateScale(newScale, ratio)) {
    bool isGlobal = m_cbGlobalTransform->isChecked();
    ScaleCommand cmd(m_controller, ratio, ratio, ratio, isGlobal);
    cmd.execute();
    m_glWidget->update();
  }
}

void QtView::onResetTransform() {
  bool isGlobal = m_cbGlobalTransform->isChecked();
  m_controller.resetTransform(isGlobal);
  m_transformState.reset();

  // Сброс слайдеров
  m_moveSliders.x->setValue(TransformState::toSlider(0, -50, 50));
  m_moveSliders.y->setValue(TransformState::toSlider(0, -50, 50));
  m_moveSliders.z->setValue(TransformState::toSlider(0, -50, 50));
  m_rotSliders.x->setValue(TransformState::toSlider(0, -180, 180));
  m_rotSliders.y->setValue(TransformState::toSlider(0, -180, 180));
  m_rotSliders.z->setValue(TransformState::toSlider(0, -180, 180));
  m_scaleSlider->setValue(100);

  m_glWidget->update();
}

// ========== SETTINGS ==========

void QtView::onSettingsChanged() {
  m_settings.edgeType = static_cast<EdgeType>(m_comboEdgeType->currentIndex());
  m_settings.edgeThickness = m_edgeThicknessSlider->value() / 10.0f;
  m_settings.vertexType =
      static_cast<VertexType>(m_comboVertexType->currentIndex());
  m_settings.vertexSize = static_cast<float>(m_vertexSizeSlider->value());
  m_settings.projection = m_comboProjection->currentIndex() == 0
                              ? ProjectionType::Central
                              : ProjectionType::Parallel;

  // Lighting
  for (int i = 0; i < 5; ++i) {
    m_settings.pointLights[i].enabled = m_lightUIs[i].group->isChecked();
    m_settings.pointLights[i].position = QVector3D(
        TransformState::fromSlider(m_lightUIs[i].x->value(), -50, 50),
        TransformState::fromSlider(m_lightUIs[i].y->value(), -50, 50),
        TransformState::fromSlider(m_lightUIs[i].z->value(), -50, 50));
    m_settings.pointLights[i].color = m_lightUIs[i].color;
    m_settings.pointLights[i].intensity =
        m_lightUIs[i].intensity->value() / 100.0f;
  }

  m_settings.ambientStrength = m_ambientSlider->value() / 100.0f;
  m_settings.specularStrength = m_specularSlider->value() / 100.0f;
  m_settings.shininess = static_cast<float>(m_shininessSlider->value());

  // Material
  m_settings.material.baseColor = m_faceColor;
  m_settings.material.reflectivity = m_reflectivitySlider->value() / 100.0f;
  m_settings.material.transparency = m_transparencySlider->value() / 100.0f;
  m_settings.material.ior = 1.0f + m_iorSlider->value() / 100.0f;
  m_settings.material.roughness = m_roughnessSlider->value() / 100.0f;
  m_settings.material.smoothShading = m_chkSmoothShading->isChecked();

  // Ray trace
  m_settings.rayTraceWidth = m_rtWidthSpin->value();
  m_settings.rayTraceHeight = m_rtHeightSpin->value();
  m_settings.rayTraceDepth = m_rtDepthSlider->value();
  m_settings.showFloor = m_chkShowFloor->isChecked();

  m_controller.setActiveMaterial(m_settings.material);

  m_renderer.setSettings(m_settings);
  m_settingsManager.save(m_settings);
  m_glWidget->update();
}

void QtView::onEdgeColorClicked() {
  QColor c = QColorDialog::getColor(m_edgeColor, m_window.get(), "Edge Color",
                                    QColorDialog::DontUseNativeDialog);
  if (c.isValid()) {
    m_edgeColor = c;
    m_settings.edgeColor = c;
    m_btnEdgeColor->setStyleSheet(
        QString("background-color: %1; min-width: 24px; min-height: 24px;")
            .arg(c.name()));
    onSettingsChanged();
  }
}

void QtView::onVertexColorClicked() {
  QColor c =
      QColorDialog::getColor(m_vertexColor, m_window.get(), "Vertex Color",
                             QColorDialog::DontUseNativeDialog);
  if (c.isValid()) {
    m_vertexColor = c;
    m_settings.vertexColor = c;
    m_btnVertexColor->setStyleSheet(
        QString("background-color: %1; min-width: 24px; min-height: 24px;")
            .arg(c.name()));
    onSettingsChanged();
  }
}

void QtView::onFaceColorClicked() {
  QColor c = QColorDialog::getColor(m_faceColor, m_window.get(), "Face Color",
                                    QColorDialog::DontUseNativeDialog);
  if (c.isValid()) {
    m_faceColor = c;
    m_settings.material.baseColor = c;
    m_btnFaceColor->setStyleSheet(
        QString("background-color: %1; min-width: 24px; min-height: 24px;")
            .arg(c.name()));
    onSettingsChanged();
  }
}

void QtView::onBackgroundColorClicked() {
  QColor c =
      QColorDialog::getColor(m_backgroundColor, m_window.get(), "Background",
                             QColorDialog::DontUseNativeDialog);
  if (c.isValid()) {
    m_backgroundColor = c;
    m_settings.backgroundColor = c;
    m_btnBackgroundColor->setStyleSheet(
        QString("background-color: %1; min-width: 24px; min-height: 24px;")
            .arg(c.name()));
    onSettingsChanged();
  }
}

// ==================== Actions ====================

void QtView::onScreenshotClicked() {
  QString file = QFileDialog::getSaveFileName(
      m_window.get(), "Save Screenshot", QDir::homePath(),
      "Images (*.bmp *.jpeg)", nullptr, QFileDialog::DontUseNativeDialog);
  if (!file.isEmpty()) {
    if (!file.endsWith(".bmp") && !file.endsWith(".jpeg")) file += ".jpeg";
    m_glWidget->grabFramebuffer().save(file);
  }
}

void QtView::onRecordGifClicked() {
  if (m_gifRecorder.isRecording()) return;

  m_isPreviewMode = false;  // явно указываем режим
  m_btnRecordGif->setEnabled(false);
  m_btnRecordGif->setText("Recording... 0%");
  m_btnPreview->setEnabled(false);  // блокируем превью на время записи

  m_gifRecorder.startRecording();
}

void QtView::onSavePreview() {
  if (m_gifRecorder.isRecording()) return;

  // Сохраняем начальный угол
  m_previewStartYaw = m_camera.getYaw();
  m_isPreviewMode = true;

  // Блокируем обе кнопки записи
  m_btnPreview->setEnabled(false);
  m_btnRecordGif->setEnabled(false);

  // Запускаем запись (те же 640x480, 10fps, 5s)
  m_gifRecorder.startRecording();
}

void QtView::setupGifRecorder() {
  auto* provider = new GLWidgetFrameProvider(m_glWidget);
  m_gifRecorder.setFrameProvider(provider);

  connect(&m_gifRecorder, &GifRecorder::progress, this, [this](int pct) {
    if (m_isPreviewMode) {
      // Вращаем камеру: 0% -> 0°, 100% -> 360°
      float angle = (pct / 100.0f) * 360.0f;
      m_camera.setYaw(m_previewStartYaw + angle);
      m_glWidget->update();
      m_btnPreview->setText(QString("Preview %1%").arg(pct));
    } else {
      m_btnRecordGif->setText(QString("Recording... %1%").arg(pct));
    }
  });

  connect(&m_gifRecorder, &GifRecorder::finished, this, [this]() {
    // Возвращаем камеру в исходное положение (для превью)
    if (m_isPreviewMode) {
      m_camera.setYaw(m_previewStartYaw);
      m_glWidget->update();
    }

    // Диалог сохранения (общий для обоих режимов)
    QString title = m_isPreviewMode ? "Save Preview GIF" : "Save GIF";

    QString file = QFileDialog::getSaveFileName(
        m_window.get(), title, QDir::homePath(), "GIF (*.gif)", nullptr,
        QFileDialog::DontUseNativeDialog);

    bool ok = false;
    if (!file.isEmpty()) {
      if (!file.endsWith(".gif")) file += ".gif";
      ok = m_gifRecorder.saveToFile(file);
    }

    // Восстановление UI
    m_btnPreview->setEnabled(true);
    m_btnRecordGif->setEnabled(true);

    if (m_isPreviewMode) {
      m_btnPreview->setText(ok ? "Saved!"
                               : (file.isEmpty() ? "Save Preview" : "Error!"));
      if (ok)
        QTimer::singleShot(2000,
                           [this]() { m_btnPreview->setText("Save Preview"); });
      m_isPreviewMode = false;
    } else {
      m_btnRecordGif->setText(ok ? "Record GIF" : "Error!");
    }
  });

  connect(&m_gifRecorder, &GifRecorder::error, this,
          [this](const QString& msg) {
            if (m_isPreviewMode) {
              m_camera.setYaw(m_previewStartYaw);
              m_glWidget->update();
              m_isPreviewMode = false;
              m_btnPreview->setText("Save Preview");
              m_btnPreview->setEnabled(true);
            }
            m_btnRecordGif->setText("Record GIF");
            m_btnPreview->setEnabled(true);
            m_btnRecordGif->setEnabled(true);
          });
}

void QtView::onLoadTextureClicked() {
  QString file =
      QFileDialog::getOpenFileName(m_window.get(), "Open BMP Texture",
                                   QDir::homePath(), "BMP Images (*.bmp)");
  if (!file.isEmpty()) {
    QImage img(file);
    if (img.isNull()) {
      m_lblTextureStatus->setText("Failed to load");
      return;
    }

    // Не может превышать 1024x1024
    if (img.width() > 1024 || img.height() > 1024) {
      m_lblTextureStatus->setText("Incurrect resolution");
      return;
    }

    m_settings.textureImage = img.mirrored(false, true);
    m_settings.useTexture = true;

    m_btnUnloadTexture->setEnabled(true);
    m_btnSaveUVMap->setEnabled(true);
    m_lblTextureStatus->setText("Texture: " + QFileInfo(file).fileName());

    onSettingsChanged();
  }
}

void QtView::onUnloadTextureClicked() {
  m_settings.textureImage = QImage();
  m_settings.useTexture = false;

  m_btnUnloadTexture->setEnabled(false);
  m_btnSaveUVMap->setEnabled(false);
  m_lblTextureStatus->setText("Texture removed.");

  onSettingsChanged();
}

void QtView::onSaveUVMapClicked() {
  if (!m_settings.useTexture || m_settings.textureImage.isNull() ||
      !m_controller.hasUvs()) {
    return;
  }

  QString filepath = QFileDialog::getSaveFileName(
      m_window.get(), "Save UV Map", QDir::homePath(), "BMP Images (*.bmp)",
      nullptr, QFileDialog::DontUseNativeDialog);

  if (filepath.isEmpty()) return;
  if (!filepath.endsWith(".bmp", Qt::CaseInsensitive)) filepath += ".bmp";

  // Переворачиваем обнратно
  QImage uvImage = m_settings.textureImage.mirrored(false, true);

  // Рисуем прямо на картинке
  QPainter painter(&uvImage);
  painter.setRenderHint(QPainter::Antialiasing);

  // Используем цвет ребер из настроек
  QPen pen(m_settings.edgeColor);
  pen.setWidth(1);  // Толщина линии в пикселях
  painter.setPen(pen);

  int width = uvImage.width();
  int height = uvImage.height();

  auto snapshot = m_controller.getSceneSnapshot();
  size_t activeIdx = m_controller.getActiveObjectIndex();

  if (activeIdx >= snapshot.objects.size()) return;

  MeshData data = snapshot.objects[activeIdx].mesh->extractMeshData();

  // Проходим по всем треугольникам и рисуем их UV-координаты
  for (size_t i = 0; i + 2 < data.solidIndices.size(); i += 3) {
    int i0 = data.solidIndices[i];
    int i1 = data.solidIndices[i + 1];
    int i2 = data.solidIndices[i + 2];

    // Достаем U и V
    float u0 = data.uvs[i0 * 2];
    float v0 = data.uvs[i0 * 2 + 1];
    float u1 = data.uvs[i1 * 2];
    float v1 = data.uvs[i1 * 2 + 1];
    float u2 = data.uvs[i2 * 2];
    float v2 = data.uvs[i2 * 2 + 1];

    QPointF p0(u0 * width, (1.0f - v0) * height);
    QPointF p1(u1 * width, (1.0f - v1) * height);
    QPointF p2(u2 * width, (1.0f - v2) * height);

    // Рисуем линии треугольника
    painter.drawLine(p0, p1);
    painter.drawLine(p1, p2);
    painter.drawLine(p2, p0);
  }

  painter.end();

  // Сохраняем в BMP
  if (uvImage.save(filepath, "BMP")) {
    m_lblTextureStatus->setText("UV Map saved successfully!");
  } else {
    m_lblTextureStatus->setText("Error saving UV Map!");
  }
}

void QtView::onActiveObjectChanged(int index) {
  if (index < 0) return;

  m_controller.setActiveObject(static_cast<size_t>(index));

  // Сбрасываем ползунки в интерфейсе
  m_transformState.reset();
  m_moveSliders.x->blockSignals(true);
  m_moveSliders.x->setValue(50);
  m_moveSliders.x->blockSignals(false);
  m_moveSliders.y->blockSignals(true);
  m_moveSliders.y->setValue(50);
  m_moveSliders.y->blockSignals(false);
  m_moveSliders.z->blockSignals(true);
  m_moveSliders.z->setValue(50);
  m_moveSliders.z->blockSignals(false);

  syncMaterialUI();
  m_btnLoadTexture->setEnabled(m_controller.hasUvs());
  m_glWidget->update();
}

void QtView::onRemoveObjectClicked() {
  m_controller.removeActiveObject();
  refreshObjectList();

  if (m_controller.getObjectCount() == 0) {
    m_btnRemoveObject->setEnabled(false);
    m_btnLoadTexture->setEnabled(false);
  }

  m_glWidget->update();
}

void QtView::run() {
  m_window->show();
  m_app->exec();
}

}  // namespace s21