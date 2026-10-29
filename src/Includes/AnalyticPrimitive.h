#pragma once
#include <QMatrix4x4>
#include <QString>
#include <QVector3D>
#include <cmath>
#include <limits>
#include <memory>
#include <vector>

#include "RTTypes.h"

namespace s21 {

class AnalyticPrimitive {
 public:
  virtual ~AnalyticPrimitive() = default;
  virtual bool intersect(const Ray& ray, float tmin, float tmax,
                         RTHit& hit) const = 0;
  virtual QString name() const = 0;
};

class TransformedPrimitive : public AnalyticPrimitive {
 public:
  TransformedPrimitive(std::shared_ptr<AnalyticPrimitive> base,
                       const QMatrix4x4& transform);
  bool intersect(const Ray& ray, float tmin, float tmax,
                 RTHit& hit) const override;
  QString name() const override { return m_base->name(); }

 private:
  std::shared_ptr<AnalyticPrimitive> m_base;
  QMatrix4x4 m_transform;
  QMatrix4x4 m_invTransform;
  QMatrix4x4 m_invTranspose;
};

class SpherePrimitive : public AnalyticPrimitive {
 public:
  SpherePrimitive(const QVector3D& center, float radius);
  bool intersect(const Ray& ray, float tmin, float tmax,
                 RTHit& hit) const override;
  QString name() const override { return "Sphere"; }

 private:
  QVector3D m_center;
  float m_radius;
};

class CylinderPrimitive : public AnalyticPrimitive {
 public:
  CylinderPrimitive(const QVector3D& center, const QVector3D& axis,
                    float radius, float height);
  bool intersect(const Ray& ray, float tmin, float tmax,
                 RTHit& hit) const override;
  QString name() const override { return "Cylinder"; }

 private:
  QVector3D m_center;
  QVector3D m_axis;
  float m_radius;
  float m_height;
};

class ConePrimitive : public AnalyticPrimitive {
 public:
  ConePrimitive(const QVector3D& center, const QVector3D& axis, float radius,
                float height);
  bool intersect(const Ray& ray, float tmin, float tmax,
                 RTHit& hit) const override;
  QString name() const override { return "Cone"; }

 private:
  QVector3D m_center;
  QVector3D m_axis;
  float m_radius;
  float m_height;
};

class CubePrimitive : public AnalyticPrimitive {
 public:
  CubePrimitive(const QVector3D& center, float size);
  bool intersect(const Ray& ray, float tmin, float tmax,
                 RTHit& hit) const override;
  QString name() const override { return "Cube"; }

 private:
  QVector3D m_min, m_max;
};

class PyramidPrimitive : public AnalyticPrimitive {
 public:
  PyramidPrimitive(const QVector3D& center, float baseSize, float height);
  bool intersect(const Ray& ray, float tmin, float tmax,
                 RTHit& hit) const override;
  QString name() const override { return "Pyramid"; }

 private:
  struct Triangle {
    QVector3D v0, v1, v2;
    QVector3D n;
  };
  std::vector<Triangle> m_tris;
};

}  // namespace s21