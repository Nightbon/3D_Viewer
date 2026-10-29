#pragma once
#include <algorithm>

#include "TypesView.h"

namespace s21 {

class OrbitCamera : public ICamera {
 public:
  OrbitCamera();
  QMatrix4x4 getViewMatrix() const override;
  QMatrix4x4 getProjectionMatrix() const override;
  void move(float dx, float dy, float dz) override;
  void rotate(float yaw, float pitch) override;
  void zoom(float factor) override;
  void setProjectionStrategy(
      std::unique_ptr<IProjectionStrategy> strategy) override final;
  float getYaw() const override { return m_yaw; }
  void setYaw(float y) override {
    m_yaw = y;
    updatePosition();
  }
  QVector3D getPosition() const override { return m_position; }
  QVector3D getTarget() const override { return m_target; }
  float getFov() const override { return m_fov; }

 private:
  void updatePosition();

  QVector3D m_position{0, 0, 5};
  QVector3D m_target{0, 0, 0};
  QVector3D m_up{0, 1, 0};

  float m_fov = 45.0f;
  float m_aspect = 1024.0f / 768.0f;
  float m_near = 0.1f;
  float m_far = 100.0f;

  float m_yaw = 45.0f;
  float m_pitch = 30.0f;
  float m_distance = 5.0f;

  std::unique_ptr<IProjectionStrategy> m_projectionStrategy;
};

}  // namespace s21
