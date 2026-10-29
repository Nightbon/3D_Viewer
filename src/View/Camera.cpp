#include "Camera.h"

#include <QtMath>

namespace s21 {

OrbitCamera::OrbitCamera() {
  setProjectionStrategy(std::make_unique<CentralProjection>());
  updatePosition();
}

QMatrix4x4 OrbitCamera::getViewMatrix() const {
  QMatrix4x4 matrix;
  matrix.lookAt(m_position, m_target, m_up);
  return matrix;
}

QMatrix4x4 OrbitCamera::getProjectionMatrix() const {
  return m_projectionStrategy->getMatrix(m_fov, m_aspect, m_near, m_far,
                                         m_distance);
}

void OrbitCamera::move(float dx, float dy, float dz) {
  m_target += QVector3D(dx, dy, dz);
  updatePosition();
}

void OrbitCamera::rotate(float yaw, float pitch) {
  m_yaw += yaw;
  m_pitch = std::clamp(m_pitch + pitch, -89.0f, 89.0f);
  updatePosition();
}

void OrbitCamera::zoom(float factor) {
  m_distance = std::clamp(m_distance - factor, 1.0f, 120.0f);
  updatePosition();
}

void OrbitCamera::setProjectionStrategy(
    std::unique_ptr<IProjectionStrategy> strategy) {
  m_projectionStrategy = std::move(strategy);
}

void OrbitCamera::updatePosition() {
  float yaw_rad = qDegreesToRadians(m_yaw);
  float pitch_rad = qDegreesToRadians(m_pitch);

  QVector3D dir(std::cos(yaw_rad) * std::cos(pitch_rad), std::sin(pitch_rad),
                std::sin(yaw_rad) * std::cos(pitch_rad));

  m_position = m_target - dir * m_distance;
}

}  // namespace s21