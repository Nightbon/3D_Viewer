#pragma once
#include <QMatrix4x4>
#include <QVector3D>
#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace s21 {

class AnalyticPrimitive;

struct Ray {
  QVector3D origin;
  QVector3D direction;
  Ray() = default;
  Ray(const QVector3D& o, const QVector3D& d) : origin(o), direction(d) {}
  // Параметрическое уравнение луча: point(t) = origin + t * direction
  QVector3D at(float t) const { return origin + direction * t; }
};

struct AABB {
  QVector3D min;
  QVector3D max;
  // Алгоритм пересечения луча с ориентированным по осям ограничивающим боксом
  // (AABB) Slab method: для каждой оси X, Y, Z вычисляем t_near и t_far как t =
  // (min/max - origin) / direction, затем пересекаем интервалы [t0, t1] Если
  // direction[i] < 0, меняем min и max местами (invD < 0)
  bool intersect(const Ray& ray, float& tmin, float& tmax) const {
    float t0 = tmin;
    float t1 = tmax;
    for (int i = 0; i < 3; ++i) {
      // invD = 1 / direction[i] — предвычисленное обратное направление
      float invD = 1.0f / ray.direction[i];
      // tn = t_near для оси i, tf = t_far для оси i
      float tn = (min[i] - ray.origin[i]) * invD;
      float tf = (max[i] - ray.origin[i]) * invD;
      // Если луч направлен в отрицательную сторону, меняем near и far
      if (invD < 0.0f) std::swap(tn, tf);
      // Пересечение интервалов: t0 = max(t0, tn), t1 = min(t1, tf)
      t0 = std::max(t0, tn);
      t1 = std::min(t1, tf);
      // Если интервалы не пересекаются — промах
      if (t1 < t0) return false;
    }
    // t0 может быть отрицательным, если начало луча внутри бокса — берём 0
    if (t0 < 0.0f) t0 = 0.0f;
    tmin = t0;
    tmax = t1;
    return true;
  }
  // Расширение AABB точкой: min = min(min, p), max = max(max, p)
  void expand(const QVector3D& p) {
    min.setX(std::min(min.x(), p.x()));
    min.setY(std::min(min.y(), p.y()));
    min.setZ(std::min(min.z(), p.z()));
    max.setX(std::max(max.x(), p.x()));
    max.setY(std::max(max.y(), p.y()));
    max.setZ(std::max(max.z(), p.z()));
  }
  void expand(const AABB& other) {
    expand(other.min);
    expand(other.max);
  }
};

struct RTMaterial {
  QVector3D color{0.8f, 0.8f, 0.8f};
  float reflectivity = 0.0f;  // коэффициент отражения [0, 1]
  float transparency = 0.0f;  // коэффициент прозрачности [0, 1]
  float ior = 1.5f;  // показатель преломления (index of refraction)
  float roughness = 0.0f;  // шероховатость поверхности [0, 1]
  bool smoothShading = true;  // гладкое (Phong) vs плоское затенение
};

struct RTHit {
  float t = std::numeric_limits<float>::infinity();  // расстояние вдоль луча
  QVector3D point;   // точка пересечения
  QVector3D normal;  // нормаль в точке
  int triangleIndex = -1;  // индекс треугольника (-2 = аналитический)
  float u = 0, v = 0;  // барицентрические координаты
  bool hit = false;    // флаг попадания
  const AnalyticPrimitive* analyticPrimitive =
      nullptr;  // указатель на примитив
};

struct RTLight {
  QVector3D position;
  QVector3D color;
};

struct RTTriangle {
  QVector3D v0, v1, v2;  // вершины треугольника
  QVector3D n0, n1, n2;  // нормали вершин (для Phong shading)
  QVector3D faceNormal;  // нормаль грани (для flat shading)
  RTMaterial material;

  // Алгоритм Мёллера-Трамбора для пересечения луча с треугольником:
  // Использует барицентрические координаты (u, v) и расстояние t.
  // Уравнение: origin + t*direction = v0 + u*(v1-v0) + v*(v2-v0)
  // Решается через векторное произведение для избежания деления до проверки
  // условий.
  bool intersect(const Ray& ray, float& t, float& u, float& v) const {
    QVector3D edge1 = v1 - v0;
    QVector3D edge2 = v2 - v0;
    // h = direction × edge2
    QVector3D h = QVector3D::crossProduct(ray.direction, edge2);
    // a = dot(edge1, h) — определитель матрицы [edge1, edge2, -direction]
    float a = QVector3D::dotProduct(edge1, h);
    // Если |a| < epsilon, луч параллелен треугольнику (вырожденный случай)
    if (a > -1e-6 && a < 1e-6) return false;
    float f = 1.0f / a;  // обратный определитель
    // s = origin - v0 — вектор от вершины к началу луча
    QVector3D s = ray.origin - v0;
    // u = f * dot(s, h) — первая барицентрическая координата
    // Должна быть в [0, 1] для попадания в треугольник
    u = f * QVector3D::dotProduct(s, h);
    if (u < 0.0f || u > 1.0f) return false;
    // q = s × edge1
    QVector3D q = QVector3D::crossProduct(s, edge1);
    // v = f * dot(direction, q) — вторая барицентрическая координата
    // Условие: v >= 0 и u + v <= 1 (внутри треугольника)
    v = f * QVector3D::dotProduct(ray.direction, q);
    if (v < 0.0f || u + v > 1.0f) return false;
    // t = f * dot(edge2, q) — расстояние вдоль луча до точки пересечения
    t = f * QVector3D::dotProduct(edge2, q);
    // t > epsilon — исключаем точки за началом луча
    return t > 1e-4f;
  }

  // Интерполяция нормали по барицентрическим координатам (Phong shading):
  // n = (n0 * w + n1 * u + n2 * v).normalized(), где w = 1 - u - v
  // При flat shading возвращается нормаль грани
  QVector3D getNormal(float u, float v, bool smooth) const {
    if (!smooth) return faceNormal;
    float w = 1.0f - u - v;
    return (n0 * w + n1 * u + n2 * v).normalized();
  }

  // Центроид треугольника: (v0 + v1 + v2) / 3
  QVector3D center() const { return (v0 + v1 + v2) / 3.0f; }

  // AABB треугольника: min/max по всем трём вершинам
  AABB bounds() const {
    AABB box;
    box.min = box.max = v0;
    box.expand(v1);
    box.expand(v2);
    return box;
  }
};

struct KDNode {
  AABB bounds;              // ограничивающий бокс узла
  KDNode* left = nullptr;   // левое поддерево
  KDNode* right = nullptr;  // правое поддерево
  std::vector<int> triangles;  // индексы треугольников в листе
  int axis = -1;               // ось разбиения (0=X, 1=Y, 2=Z)
  bool isLeaf = false;  // флаг листового узла
};

}  // namespace s21