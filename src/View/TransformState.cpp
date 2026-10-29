#include "TransformState.h"

#include <QtMath>

namespace s21 {

bool TransformState::updateMove(const QVector3D& newMove, QVector3D& outDelta) {
  if (newMove == m_current.move) return false;
  outDelta = newMove - m_current.move;
  m_current.move = newMove;
  return true;
}

bool TransformState::updateRotate(const QVector3D& newRotate,
                                  QVector3D& outDelta) {
  if (newRotate == m_current.rotate) return false;
  outDelta = newRotate - m_current.rotate;
  m_current.rotate = newRotate;
  return true;
}

bool TransformState::updateScale(float newScale, float& outRatio) {
  if (qFuzzyCompare(newScale, m_current.scale)) return false;
  outRatio = newScale / m_current.scale;
  m_current.scale = newScale;
  return true;
}

void TransformState::reset() { m_current = Transform{}; }

int TransformState::toSlider(float val, float min, float max) {
  return qRound((val - min) / (max - min) * 100);
}

float TransformState::fromSlider(int val, float min, float max) {
  return min + val * (max - min) / 100.0f;
}

}  // namespace s21