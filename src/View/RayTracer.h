#pragma once
#include <QImage>
#include <QMatrix4x4>
#include <QVector3D>
#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <numeric>
#include <random>
#include <unordered_map>
#include <vector>

#include "../Includes/AnalyticPrimitive.h"
#include "../Includes/RTTypes.h"
#include "TypesView.h"

namespace s21 {

class RayTracer {
 public:
  RayTracer();
  ~RayTracer();

  void setMeshData(const MeshData& data);
  void setTransform(const QMatrix4x4& transform);
  void setCamera(const ICamera& camera);
  void setSettings(const RenderSettings& renderSettings);
  void setRayTraceSettings(const RayTraceSettings& rtSettings);
  void setMaterial(const RTMaterial& material);

  void setSceneData(const SceneSnapshot& snapshot);
  void setAnalyticPrimitives(
      const std::vector<std::shared_ptr<AnalyticPrimitive>>& primitives);
  void clearAnalyticPrimitives();

  QImage render();

 private:
  void buildTriangles(const MeshData& data);
  KDNode* buildKDTree(std::vector<int>& triIndices, int depth);
  void intersectKDTree(KDNode* node, const Ray& ray, float tmin, float tmax,
                       RTHit& hit);
  bool intersectKDTreeShadow(KDNode* node, const Ray& ray, float tmin,
                             float tmax, float maxDist);
  bool intersectAnalyticShadow(const Ray& ray, float tmin, float tmax);
  QVector3D traceRay(const Ray& ray, int depth);
  QVector3D traceRayRecursive(const Ray& ray, int depth);
  // --- Подфункции traceRayRecursive ---
  bool findClosestHit(const Ray& ray, float tmin, float tmax, RTHit& outHit);
  RTMaterial resolveMaterial(const RTHit& hit) const;
  QVector3D shadeSurface(const RTHit& hit, const RTMaterial& mat,
                         const Ray& ray);
  QVector3D traceReflection(const RTHit& hit, const RTMaterial& mat,
                            const Ray& ray, int depth);
  QVector3D traceRefraction(const RTHit& hit, const RTMaterial& mat,
                            const Ray& ray, int depth);
  bool isInShadow(const QVector3D& point, const QVector3D& lightPos,
                  const QVector3D& geomNormal);
  QVector3D computeGeomNormal(const RTHit& hit, const Ray& ray) const;

  // --- Вспомогательные математические функции ---

  // Случайная точка в единичной сфере (rejection sampling)
  QVector3D randomInUnitSphere();

  // Зеркальное отражение вектора: R = V - 2*(V·N)*N
  QVector3D reflect(const QVector3D& v, const QVector3D& n);

  // Преломление вектора (векторная форма закона Снеллиуса)
  QVector3D refract(const QVector3D& uv, const QVector3D& n,
                    float etai_over_etat);

  // Коэффициент Френеля для неполяризованного света
  float fresnel(const QVector3D& I, const QVector3D& N, float ior);

  // Пересечение луча с плоскостью пола Y = -1.0: t = (planeY - origin.y) /
  // dir.y
  bool intersectFloor(const Ray& ray, RTHit& hit);

  // Шахматная текстура пола: цвет зависит от чётности floor(x*2) + floor(z*2)
  QVector3D getFloorColor(const QVector3D& point) const;

  // Рекурсивное удаление KD-дерева
  void deleteKDTree(KDNode* node);

  // --- Данные ---

  std::vector<RTTriangle> m_triangles;  // полигональные треугольники сцены
  KDNode* m_root = nullptr;  // корень KD-дерева ускорения

  RenderSettings m_renderSettings;  // настройки освещения и материалов
  RayTraceSettings m_rtSettings;  // разрешение, глубина рекурсии, пол
  RTMaterial m_material;  // материал
  QMatrix4x4 m_transform;  // глобальная трансформация mesh
  bool m_hasTransform = false;

  std::mt19937 m_rng;  // генератор случайных чисел
  std::uniform_real_distribution<float>
      m_dist;  // равномерное распределение [0,1]

  // Параметры камеры (look-at базис)
  QVector3D m_cameraPos;  // позиция камеры
  QVector3D m_cameraForward;  // направление взгляда (нормализованное)
  QVector3D m_cameraRight;  // вектор вправо (нормализованный)
  QVector3D m_cameraUp;  // вектор вверх (нормализованный)
  float m_cameraFov = 45.0f;  // вертикальный угол обзора (градусы)
  float m_cameraAspect = 1.0f;  // соотношение сторон w/h

  // Аналитические примитивы (сфера, цилиндр, конус, куб, пирамида)
  std::vector<std::shared_ptr<AnalyticPrimitive>> m_analyticPrimitives;
  // Сопоставление аналитических примитивов с их материалами
  std::unordered_map<const AnalyticPrimitive*, Material> m_analyticMaterials;
};

}  // namespace s21