#pragma once
#include "../Model/Model.h"
#include "../Model/ObjParser.h"
#include "../Model/TypesModel.h"
#include "../View/TypesView.h"

namespace s21 {

class ViewerController;

class ICommand {
 public:
  virtual ~ICommand() = default;
  virtual void execute() = 0;
};

class BaseCommand : public ICommand {
 public:
  BaseCommand(ViewerController& c, bool g) : m_controller(c), m_global(g) {}

 protected:
  ViewerController& m_controller;
  bool m_global;
};

class TranslateCommand : public BaseCommand {
 public:
  TranslateCommand(ViewerController& c, float dx, float dy, float dz, bool g);
  void execute() override;

 private:
  float m_dx, m_dy, m_dz;
};

class RotateCommand : public BaseCommand {
 public:
  RotateCommand(ViewerController& c, float ax, float ay, float az, bool g);
  void execute() override;

 private:
  float m_ax, m_ay, m_az;
};

class ScaleCommand : public BaseCommand {
 public:
  ScaleCommand(ViewerController& c, float sx, float sy, float sz, bool g);
  void execute() override;

 private:
  float m_sx, m_sy, m_sz;
};

class ViewerController : public ISceneProvider {
 public:
  ViewerController();
  bool loadModel(const std::string& path);
  std::string getLastError() const;

  void setActiveObject(size_t index);
  size_t getActiveObjectIndex() const;
  size_t getObjectCount() const;
  void removeActiveObject();

  size_t getVertexCount() const;
  size_t getEdgeCount() const;
  size_t getFaceCount() const;
  bool hasUvs() const;

  void translate(float dx, float dy, float dz, bool global);
  void rotate(float ax, float ay, float az, bool global);
  void scale(float sx, float sy, float sz, bool global);
  void resetTransform(bool global);

  SceneSnapshot getSceneSnapshot() const override;
  Material getActiveMaterial() const;
  void setActiveMaterial(const Material& mat);
  QString getObjectTypeName(size_t index) const;

  void addSphere(float radius = 1.0f);
  void addCylinder(float radius = 1.0f, float height = 2.0f);
  void addCone(float radius = 1.0f, float height = 2.0f);
  void addCube(float size = 2.0f);
  void addPyramid(float baseSize = 2.0f, float height = 2.0f);

 private:
  std::shared_ptr<IScene> m_scene;
  ObjParser m_parser;
  size_t m_activeIndex = 0;
};

}  // namespace s21