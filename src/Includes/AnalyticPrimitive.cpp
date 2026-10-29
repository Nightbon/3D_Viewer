#include "AnalyticPrimitive.h"

#include <QtMath>

namespace s21 {

// ---------- TransformedPrimitive ----------
TransformedPrimitive::TransformedPrimitive(
    std::shared_ptr<AnalyticPrimitive> base, const QMatrix4x4& transform)
    : m_base(std::move(base)), m_transform(transform) {
  m_invTransform = transform.inverted();
  m_invTranspose = m_invTransform.transposed();
}

bool TransformedPrimitive::intersect(const Ray& ray, float tmin, float tmax,
                                     RTHit& hit) const {
  // Преобразование луча из мирового пространства в локальное пространство
  // примитива: origin' = M^-1 * origin, direction' = normalize(M^-1 *
  // direction)
  Ray localRay;
  localRay.origin = m_invTransform.map(ray.origin);
  localRay.direction = m_invTransform.mapVector(ray.direction).normalized();

  if (!m_base->intersect(localRay, 0.0f, std::numeric_limits<float>::infinity(),
                         hit))
    return false;

  // Обратное преобразование точки пересечения и нормали в мировое пространство
  hit.point = m_transform.map(hit.point);
  // Нормали преобразуются транспонированной обратной матрицей для сохранения
  // ортогональности
  hit.normal = m_invTranspose.mapVector(hit.normal).normalized();
  // Пересчёт t-расстояния вдоль исходного луча: t = dot(hit.point - origin,
  // direction)
  hit.t = QVector3D::dotProduct(hit.point - ray.origin, ray.direction);

  return hit.t >= tmin && hit.t <= tmax;
}

// ---------- Sphere ----------
SpherePrimitive::SpherePrimitive(const QVector3D& center, float radius)
    : m_center(center), m_radius(radius) {}

bool SpherePrimitive::intersect(const Ray& ray, float tmin, float tmax,
                                RTHit& hit) const {
  // Решение квадратного уравнения для пересечения луча со сферой:
  // |ray.origin + t * ray.direction - center|^2 = radius^2
  // Раскрывая: a*t^2 + 2*b*t + c = 0, где:
  // a = |direction|^2, b = dot(oc, direction), c = |oc|^2 - R^2
  // (oc = origin - center)
  QVector3D oc = ray.origin - m_center;
  float a = ray.direction.lengthSquared();
  float half_b = QVector3D::dotProduct(oc, ray.direction);
  float c = oc.lengthSquared() - m_radius * m_radius;
  // Дискриминант: D = b^2 - 4ac = (2*half_b)^2 - 4*a*c = 4*(half_b^2 - a*c)
  float discriminant = half_b * half_b - a * c;
  if (discriminant < 0) return false;
  float sqrtd = std::sqrt(discriminant);
  // Ближайший корень: t = (-b - sqrt(D)) / (2a) = (-half_b - sqrtd) / a
  float root = (-half_b - sqrtd) / a;
  if (root < tmin || tmax < root) {
    // Если ближайший корень вне диапазона, проверяем дальний корень
    root = (-half_b + sqrtd) / a;
    if (root < tmin || tmax < root) return false;
  }
  hit.t = root;
  hit.point = ray.at(root);
  // Нормаль к сфере: (point - center) / radius (единичная нормаль)
  hit.normal = (hit.point - m_center) / m_radius;
  hit.hit = true;
  hit.triangleIndex = -2;
  return true;
}

// ---------- Cylinder ----------
CylinderPrimitive::CylinderPrimitive(const QVector3D& center,
                                     const QVector3D& axis, float radius,
                                     float height)
    : m_center(center),
      m_axis(axis.normalized()),
      m_radius(radius),
      m_height(height) {}

bool CylinderPrimitive::intersect(const Ray& ray, float tmin, float tmax,
                                  RTHit& hit) const {
  // Пересечение с боковой поверхностью цилиндра:
  // Проецируем луч и цилиндр на плоскость, перпендикулярную оси.
  // В этой плоскости задача сводится к пересечению с кругом радиуса R.
  //
  // u = direction - dot(direction, axis) * axis  — проекция направления
  // v = (origin - center) - dot(origin - center, axis) * axis  — проекция
  // начала Уравнение: |origin + t*direction - center|^2 - dot(..., axis)^2 =
  // R^2 Раскрывая: a*t^2 + b*t + c = 0
  QVector3D co = ray.origin - m_center;
  QVector3D u =
      ray.direction - QVector3D::dotProduct(ray.direction, m_axis) * m_axis;
  QVector3D v = co - QVector3D::dotProduct(co, m_axis) * m_axis;
  float a = u.lengthSquared();
  float b = 2.0f * QVector3D::dotProduct(u, v);
  float c = v.lengthSquared() - m_radius * m_radius;
  float disc = b * b - 4 * a * c;
  if (disc < 0 || a < 1e-6f) return false;
  float sqrtd = std::sqrt(disc);
  float t = (-b - sqrtd) / (2 * a);
  if (t < tmin || t > tmax) t = (-b + sqrtd) / (2 * a);
  if (t < tmin || t > tmax) return false;

  QVector3D p = ray.at(t);
  // Проверка, что точка находится в пределах высоты цилиндра:
  // |dot(p - center, axis)| <= H/2
  float proj = QVector3D::dotProduct(p - m_center, m_axis);
  if (std::fabs(proj) > m_height * 0.5f) {
    // Точка за пределами боковой поверхности — проверяем крышки
    float denom = QVector3D::dotProduct(ray.direction, m_axis);
    if (std::fabs(denom) > 1e-6f) {
      // Пересечение с плоскостью крышки: t = dot(capCenter - origin, axis) /
      // dot(direction, axis)
      QVector3D capCenter =
          m_center + (proj > 0 ? m_axis : -m_axis) * (m_height * 0.5f);
      float t_cap =
          QVector3D::dotProduct(capCenter - ray.origin, m_axis) / denom;
      if (t_cap >= tmin && t_cap <= tmax) {
        QVector3D pc = ray.at(t_cap);
        // Проверка, что точка внутри круга крышки: |pc - capCenter|^2 <= R^2
        if ((pc - capCenter).lengthSquared() <= m_radius * m_radius) {
          hit.t = t_cap;
          hit.point = pc;
          hit.normal = proj > 0 ? m_axis : -m_axis;
          hit.hit = true;
          hit.triangleIndex = -2;
          return true;
        }
      }
    }
    return false;
  }
  hit.t = t;
  hit.point = p;
  // Нормаль боковой поверхности: радиальная составляющая (перпендикуляр к оси)
  QVector3D radial = (p - m_center - proj * m_axis).normalized();
  hit.normal = radial;
  hit.hit = true;
  hit.triangleIndex = -2;
  return true;
}

// ---------- Cone ----------
ConePrimitive::ConePrimitive(const QVector3D& center, const QVector3D& axis,
                             float radius, float height)
    : m_center(center),
      m_axis(axis.normalized()),
      m_radius(radius),
      m_height(height) {}

bool ConePrimitive::intersect(const Ray& ray, float tmin, float tmax,
                              RTHit& hit) const {
  // Пересечение с боковой поверхностью конуса:
  // Конус с вершиной в apex = center + axis * height, полу-угол раствора alpha,
  // где cos(alpha) = height / sqrt(R^2 + H^2)
  // Уравнение конуса: dot(v, axis)^2 = cos^2(alpha) * |v|^2, где v = p - apex
  // Подставляя p = origin + t * direction, получаем квадратное уравнение
  QVector3D apex = m_center + m_axis * m_height;
  // cos^2(theta), где theta — половина угла раствора конуса
  float cos2 =
      (m_height * m_height) / (m_radius * m_radius + m_height * m_height);

  QVector3D co = ray.origin - apex;
  // dv = dot(direction, axis), ov = dot(co, axis) — проекции на ось
  float dv = QVector3D::dotProduct(ray.direction, m_axis);
  float ov = QVector3D::dotProduct(co, m_axis);
  // dh = direction - dv*axis, oh = co - ov*axis — горизонтальные составляющие
  QVector3D dh = ray.direction - dv * m_axis;
  QVector3D oh = co - ov * m_axis;
  // Коэффициенты квадратного уравнения для пересечения с конусом:
  // a = dv^2 - cos2*(dv^2 + |dh|^2)
  // b = 2*(dv*ov - cos2*(dv*ov + dot(dh, oh)))
  // c = ov^2 - cos2*(ov^2 + |oh|^2)
  float a = dv * dv - cos2 * (dv * dv + dh.lengthSquared());
  float b = 2 * (dv * ov - cos2 * (dv * ov + QVector3D::dotProduct(dh, oh)));
  float c = ov * ov - cos2 * (ov * ov + oh.lengthSquared());

  float disc = b * b - 4 * a * c;
  if (disc < 0) return false;
  float sqrtd = std::sqrt(disc);
  float t = (-b - sqrtd) / (2 * a);
  if (t < tmin || t > tmax) t = (-b + sqrtd) / (2 * a);
  if (t < tmin || t > tmax) return false;

  QVector3D p = ray.at(t);
  // Проверка, что точка в пределах высоты конуса: 0 <= proj <= H
  float proj = QVector3D::dotProduct(p - m_center, m_axis);
  if (proj < 0 || proj > m_height) {
    // Точка за пределами боковой поверхности — проверяем основание
    float denom = QVector3D::dotProduct(ray.direction, -m_axis);
    if (std::fabs(denom) > 1e-6f) {
      // Пересечение с плоскостью основания: t = dot(center - origin, -axis) /
      // dot(direction, -axis)
      float t_cap =
          QVector3D::dotProduct(m_center - ray.origin, -m_axis) / denom;
      if (t_cap >= tmin && t_cap <= tmax) {
        QVector3D pc = ray.at(t_cap);
        // Проверка, что точка внутри круга основания
        if ((pc - m_center).lengthSquared() <= m_radius * m_radius) {
          hit.t = t_cap;
          hit.point = pc;
          hit.normal = -m_axis;  // нормаль основания направлена вниз
          hit.hit = true;
          hit.triangleIndex = -2;
          return true;
        }
      }
    }
    return false;
  }
  hit.t = t;
  hit.point = p;
  // Корректная нормаль конуса: комбинация радиальной и осевой составляющих
  // Нормаль образует угол (90° - alpha/2) с осью, где alpha — угол раствора
  QVector3D v = p - apex;
  float projOnAxis = QVector3D::dotProduct(v, m_axis);
  QVector3D onAxis = apex + m_axis * projOnAxis;  // проекция на ось
  QVector3D radial =
      (p - onAxis).normalized();  // радиальное направление наружу

  // sin(theta) = R / hyp, cos(theta) = H / hyp, где hyp = sqrt(R^2 + H^2)
  float hyp = std::sqrt(m_radius * m_radius + m_height * m_height);
  float sinTheta = m_radius / hyp;
  float cosTheta = m_height / hyp;

  // Нормаль к боковой поверхности: radial*sin(theta) + axis*cos(theta)
  // Это вектор, перпендикулярный образующей конуса
  QVector3D sideNormal = (radial * sinTheta + m_axis * cosTheta).normalized();
  hit.normal = sideNormal;
  hit.hit = true;
  hit.triangleIndex = -2;
  return true;
}

// ---------- Cube ----------
CubePrimitive::CubePrimitive(const QVector3D& center, float size)
    : m_min(center - QVector3D(size * 0.5f, size * 0.5f, size * 0.5f)),
      m_max(center + QVector3D(size * 0.5f, size * 0.5f, size * 0.5f)) {}

bool CubePrimitive::intersect(const Ray& ray, float tmin, float tmax,
                              RTHit& hit) const {
  AABB box;
  box.min = m_min;
  box.max = m_max;
  // Алгоритм Лай-Барского (Liang-Barsky) / slab method для AABB:
  // Для каждой оси вычисляем t_near и t_far, затем пересекаем интервалы
  float t0 = tmin, t1 = tmax;
  if (!box.intersect(ray, t0, t1)) return false;
  // Выбираем ближайшее положительное пересечение
  float t = (t0 < tmin) ? t1 : t0;
  if (t < tmin || t > tmax) return false;

  hit.t = t;
  hit.point = ray.at(t);
  // Определение нормали грани по доминирующей координате:
  // c = center, p = point - center, d = max - min
  // nx = p.x / (d.x/2) * bias — нормализованная координата относительно центра
  // Грань с максимальным |n| определяет нормаль
  QVector3D c = (m_min + m_max) * 0.5f;
  QVector3D p = hit.point - c;
  QVector3D d = m_max - m_min;
  float bias = 1.0001f;  // небольшой смещение для избежания пограничных случаев
  float nx = p.x() / (std::fabs(d.x()) * 0.5f) * bias;
  float ny = p.y() / (std::fabs(d.y()) * 0.5f) * bias;
  float nz = p.z() / (std::fabs(d.z()) * 0.5f) * bias;
  float maxc = std::max({std::fabs(nx), std::fabs(ny), std::fabs(nz)});
  if (maxc == std::fabs(nx))
    hit.normal = QVector3D(nx > 0 ? 1 : -1, 0, 0);
  else if (maxc == std::fabs(ny))
    hit.normal = QVector3D(0, ny > 0 ? 1 : -1, 0);
  else
    hit.normal = QVector3D(0, 0, nz > 0 ? 1 : -1);
  hit.hit = true;
  hit.triangleIndex = -2;
  return true;
}

// ---------- Pyramid ----------
PyramidPrimitive::PyramidPrimitive(const QVector3D& center, float baseSize,
                                   float height) {
  float h = baseSize * 0.5f;
  // 4 вершины квадратного основания в плоскости Y = center.y
  QVector3D base[4] = {
      center + QVector3D(-h, 0, -h), center + QVector3D(h, 0, -h),
      center + QVector3D(h, 0, h), center + QVector3D(-h, 0, h)};
  // Вершина пирамиды над центром
  QVector3D apex = center + QVector3D(0, height, 0);
  // 2 треугольника основания + 4 боковых треугольника
  // Нормаль каждого треугольника: n = (v1 - v0) × (v2 - v0)
  m_tris.push_back(
      {base[0], base[1], base[2],
       QVector3D::crossProduct(base[1] - base[0], base[2] - base[0])
           .normalized()});
  m_tris.push_back(
      {base[0], base[2], base[3],
       QVector3D::crossProduct(base[2] - base[0], base[3] - base[0])
           .normalized()});
  m_tris.push_back({base[0], base[1], apex,
                    QVector3D::crossProduct(base[1] - base[0], apex - base[0])
                        .normalized()});
  m_tris.push_back({base[1], base[2], apex,
                    QVector3D::crossProduct(base[2] - base[1], apex - base[1])
                        .normalized()});
  m_tris.push_back({base[2], base[3], apex,
                    QVector3D::crossProduct(base[3] - base[2], apex - base[2])
                        .normalized()});
  m_tris.push_back({base[3], base[0], apex,
                    QVector3D::crossProduct(base[0] - base[3], apex - base[3])
                        .normalized()});
}

bool PyramidPrimitive::intersect(const Ray& ray, float tmin, float tmax,
                                 RTHit& hit) const {
  bool found = false;
  // Пересечение луча с каждым треугольником пирамиды (Möller-Trumbore
  // algorithm)
  for (const auto& tri : m_tris) {
    QVector3D edge1 = tri.v1 - tri.v0;
    QVector3D edge2 = tri.v2 - tri.v0;
    // h = direction × edge2
    QVector3D h = QVector3D::crossProduct(ray.direction, edge2);
    // a = dot(edge1, h) — скалярное произведение, определяющее ориентацию
    float a = QVector3D::dotProduct(edge1, h);
    // Если a ≈ 0, луч параллелен треугольнику
    if (a > -1e-6 && a < 1e-6) continue;
    float f = 1.0f / a;
    // s = origin - v0 — вектор от вершины к началу луча
    QVector3D s = ray.origin - tri.v0;
    // u = f * dot(s, h) — барицентрическая координата (должна быть в [0, 1])
    float u = f * QVector3D::dotProduct(s, h);
    if (u < 0.0f || u > 1.0f) continue;
    // q = s × edge1
    QVector3D q = QVector3D::crossProduct(s, edge1);
    // v = f * dot(direction, q) — вторая барицентрическая координата
    float v = f * QVector3D::dotProduct(ray.direction, q);
    if (v < 0.0f || u + v > 1.0f) continue;
    // t = f * dot(edge2, q) — расстояние вдоль луча до точки пересечения
    float t = f * QVector3D::dotProduct(edge2, q);
    if (t > tmin && t < tmax && t < hit.t) {
      hit.t = t;
      hit.point = ray.at(t);
      hit.normal = tri.n;
      hit.hit = true;
      hit.triangleIndex = -2;
      found = true;
    }
  }
  return found;
}

}  // namespace s21