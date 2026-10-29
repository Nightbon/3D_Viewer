#pragma once
#include <QVector3D>

namespace s21 {

struct Transform {
  QVector3D move{0.0f, 0.0f, 0.0f};
  QVector3D rotate{0.0f, 0.0f, 0.0f};
  float scale = 1.0f;
};

class TransformState {
 public:
  bool updateMove(const QVector3D& newMove, QVector3D& outDelta);
  bool updateRotate(const QVector3D& newRotate, QVector3D& outDelta);
  bool updateScale(float newScale, float& outRatio);

  void reset();
  const Transform& current() const { return m_current; }

  static int toSlider(float val, float min, float max);
  static float fromSlider(int val, float min, float max);

 private:
  Transform m_current;
};

}  // namespace s21