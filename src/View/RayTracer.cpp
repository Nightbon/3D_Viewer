#include "RayTracer.h"

#include <QPainter>
#include <QtMath>

namespace s21 {

constexpr float SHADOW_BIAS =
    5e-2f;  // смещение начала теневого луча от поверхности
constexpr float RAY_BIAS =
    5e-2f;  // смещение начала отражённого/преломлённого луча
constexpr float RAY_TMIN =
    1e-4f;  // минимальное допустимое t (исключает самопересечение)

// Преобразование матрицы из внутреннего формата s21::Matrix в Qt QMatrix4x4
// (построчное копирование 4x4 элементов)
static QMatrix4x4 toQt(const s21::Matrix& m) {
  QMatrix4x4 result;
  for (int r = 0; r < 4; ++r) {
    for (int c = 0; c < 4; ++c) {
      result(r, c) = static_cast<float>(m(r, c));
    }
  }
  return result;
}

/* ---------- RayTracer ---------- */
RayTracer::RayTracer() : m_rng(std::random_device{}()), m_dist(0.0f, 1.0f) {}

RayTracer::~RayTracer() { deleteKDTree(m_root); }

void RayTracer::deleteKDTree(KDNode* node) {
  if (!node) return;
  deleteKDTree(node->left);
  deleteKDTree(node->right);
  delete node;
}

void RayTracer::setMeshData(const MeshData& data) {
  // Оставляем для обратной совместимости, но не используем напрямую
  if (m_root) {
    deleteKDTree(m_root);
    m_root = nullptr;
  }
  buildTriangles(data);
  if (!m_triangles.empty()) {
    std::vector<int> indices(m_triangles.size());
    std::iota(indices.begin(), indices.end(), 0);
    m_root = buildKDTree(indices, 0);
  }
}

void RayTracer::setSceneData(const SceneSnapshot& snapshot) {
  if (m_root) {
    deleteKDTree(m_root);
    m_root = nullptr;
  }
  m_triangles.clear();
  clearAnalyticPrimitives();

  // Глобальная трансформация сцены (например, масштабирование всей модели)
  QMatrix4x4 globalTransform = toQt(snapshot.globalTransform);
  std::vector<std::shared_ptr<AnalyticPrimitive>> analyticPrims;

  for (const auto& obj : snapshot.objects) {
    if (!obj.mesh) continue;

    auto prim = obj.mesh->getAnalyticPrimitive();
    if (prim) {
      // Аналитический примитив: применяем глобальную + локальную трансформацию
      QMatrix4x4 objTransform = globalTransform * toQt(obj.transform);
      auto transformed =
          std::make_shared<TransformedPrimitive>(prim, objTransform);
      m_analyticMaterials[transformed.get()] = obj.material;
      analyticPrims.push_back(transformed);
      continue;
    }

    // Полигональный mesh: извлекаем треугольники и трансформируем вершины
    MeshData data = obj.mesh->extractMeshData();
    if (!data.hasData || data.solidIndices.empty()) continue;

    QMatrix4x4 objTransform = globalTransform * toQt(obj.transform);
    // Нормали трансформируются транспонированной обратной матрицей (M^-1)^T
    // для сохранения ортогональности при неравномерном масштабировании
    QMatrix4x4 normalMatrix = objTransform.inverted().transposed();

    for (size_t i = 0; i + 2 < data.solidIndices.size(); i += 3) {
      unsigned int i0 = data.solidIndices[i];
      unsigned int i1 = data.solidIndices[i + 1];
      unsigned int i2 = data.solidIndices[i + 2];

      if (i0 * 3 + 2 >= data.solidVertices.size() ||
          i1 * 3 + 2 >= data.solidVertices.size() ||
          i2 * 3 + 2 >= data.solidVertices.size())
        continue;

      RTTriangle tri;
      // Чтение локальных координат вершин из плоского массива [x0,y0,z0,
      // x1,y1,z1, ...]
      QVector3D localV0(data.solidVertices[i0 * 3],
                        data.solidVertices[i0 * 3 + 1],
                        data.solidVertices[i0 * 3 + 2]);
      QVector3D localV1(data.solidVertices[i1 * 3],
                        data.solidVertices[i1 * 3 + 1],
                        data.solidVertices[i1 * 3 + 2]);
      QVector3D localV2(data.solidVertices[i2 * 3],
                        data.solidVertices[i2 * 3 + 1],
                        data.solidVertices[i2 * 3 + 2]);

      // Преобразование вершин в мировое пространство: v_world = M * v_local
      tri.v0 = objTransform.map(localV0);
      tri.v1 = objTransform.map(localV1);
      tri.v2 = objTransform.map(localV2);

      // Нормаль грани (flat shading): n = normalize((v1 - v0) × (v2 - v0))
      // Порядок вершин определяет направление нормали (правило правой руки)
      QVector3D edge1 = tri.v1 - tri.v0;
      QVector3D edge2 = tri.v2 - tri.v0;
      tri.faceNormal = QVector3D::crossProduct(edge1, edge2).normalized();

      // Нормали вершин для Phong shading (интерполированное затенение)
      if (data.hasNormals && i0 * 3 + 2 < data.normals.size()) {
        QVector3D localN0(data.normals[i0 * 3], data.normals[i0 * 3 + 1],
                          data.normals[i0 * 3 + 2]);
        QVector3D localN1(data.normals[i1 * 3], data.normals[i1 * 3 + 1],
                          data.normals[i1 * 3 + 2]);
        QVector3D localN2(data.normals[i2 * 3], data.normals[i2 * 3 + 1],
                          data.normals[i2 * 3 + 2]);
        // Трансформация нормалей: n_world = normalize((M^-1)^T * n_local)
        tri.n0 = normalMatrix.map(localN0).normalized();
        tri.n1 = normalMatrix.map(localN1).normalized();
        tri.n2 = normalMatrix.map(localN2).normalized();
      } else {
        tri.n0 = tri.n1 = tri.n2 = tri.faceNormal;
      }

      // Копирование материала объекта в структуру треугольника
      tri.material.color = QVector3D(obj.material.baseColor.redF(),
                                     obj.material.baseColor.greenF(),
                                     obj.material.baseColor.blueF());
      tri.material.reflectivity = obj.material.reflectivity;
      tri.material.transparency = obj.material.transparency;
      tri.material.ior = obj.material.ior;
      tri.material.roughness = obj.material.roughness;
      tri.material.smoothShading = obj.material.smoothShading;

      m_triangles.push_back(tri);
    }
  }

  setAnalyticPrimitives(analyticPrims);

  if (!m_triangles.empty()) {
    std::vector<int> indices(m_triangles.size());
    std::iota(indices.begin(), indices.end(), 0);
    m_root = buildKDTree(indices, 0);
  }
}

void RayTracer::setTransform(const QMatrix4x4& transform) {
  m_transform = transform;
  m_hasTransform = !transform.isIdentity();
}

void RayTracer::buildTriangles(const MeshData& data) {
  m_triangles.clear();
  if (!data.hasData || data.solidIndices.empty()) return;

  QMatrix4x4 normalMatrix =
      m_hasTransform ? m_transform.inverted().transposed() : QMatrix4x4();

  for (size_t i = 0; i + 2 < data.solidIndices.size(); i += 3) {
    unsigned int i0 = data.solidIndices[i];
    unsigned int i1 = data.solidIndices[i + 1];
    unsigned int i2 = data.solidIndices[i + 2];

    if (i0 * 3 + 2 >= data.solidVertices.size() ||
        i1 * 3 + 2 >= data.solidVertices.size() ||
        i2 * 3 + 2 >= data.solidVertices.size())
      continue;

    RTTriangle tri;

    QVector3D localV0(data.solidVertices[i0 * 3],
                      data.solidVertices[i0 * 3 + 1],
                      data.solidVertices[i0 * 3 + 2]);
    QVector3D localV1(data.solidVertices[i1 * 3],
                      data.solidVertices[i1 * 3 + 1],
                      data.solidVertices[i1 * 3 + 2]);
    QVector3D localV2(data.solidVertices[i2 * 3],
                      data.solidVertices[i2 * 3 + 1],
                      data.solidVertices[i2 * 3 + 2]);

    if (m_hasTransform) {
      tri.v0 = m_transform.map(localV0);
      tri.v1 = m_transform.map(localV1);
      tri.v2 = m_transform.map(localV2);
    } else {
      tri.v0 = localV0;
      tri.v1 = localV1;
      tri.v2 = localV2;
    }

    QVector3D edge1 = tri.v1 - tri.v0;
    QVector3D edge2 = tri.v2 - tri.v0;
    tri.faceNormal = QVector3D::crossProduct(edge1, edge2).normalized();

    if (data.hasNormals && i0 * 3 + 2 < data.normals.size()) {
      QVector3D localN0(data.normals[i0 * 3], data.normals[i0 * 3 + 1],
                        data.normals[i0 * 3 + 2]);
      QVector3D localN1(data.normals[i1 * 3], data.normals[i1 * 3 + 1],
                        data.normals[i1 * 3 + 2]);
      QVector3D localN2(data.normals[i2 * 3], data.normals[i2 * 3 + 1],
                        data.normals[i2 * 3 + 2]);

      if (m_hasTransform) {
        tri.n0 = normalMatrix.map(localN0).normalized();
        tri.n1 = normalMatrix.map(localN1).normalized();
        tri.n2 = normalMatrix.map(localN2).normalized();
      } else {
        tri.n0 = localN0.normalized();
        tri.n1 = localN1.normalized();
        tri.n2 = localN2.normalized();
      }
    } else {
      tri.n0 = tri.n1 = tri.n2 = tri.faceNormal;
    }
    m_triangles.push_back(tri);
  }
}

// Построение KD-дерева (пространственное разбиение):
// Рекурсивно разбиваем набор треугольников по оси с наибольшим протяжением.
// На каждом уровне выбираем ось (X/Y/Z) с максимальной длиной AABB,
// сортируем треугольники по центроиду и делим пополам (медиана).
KDNode* RayTracer::buildKDTree(std::vector<int>& triIndices, int depth) {
  KDNode* node = new KDNode();
  if (triIndices.empty()) {
    node->isLeaf = true;
    return node;
  }

  // Вычисление AABB, охватывающего все треугольники в узле
  AABB box;
  bool first = true;
  for (int idx : triIndices) {
    AABB triBox = m_triangles[idx].bounds();
    if (first) {
      box = triBox;
      first = false;
    } else
      box.expand(triBox);
  }
  node->bounds = box;

  // Условие останова: ≤4 треугольника или глубина > 20
  if (triIndices.size() <= 4 || depth > 20) {
    node->isLeaf = true;
    node->triangles = std::move(triIndices);
    return node;
  }

  // Выбор оси разбиения: та, по которой AABB имеет наибольший размер
  int axis = 0;
  QVector3D size = box.max - box.min;
  if (size.y() > size.x()) axis = 1;
  if (size.z() > size[axis]) axis = 2;
  node->axis = axis;

  // Сортировка треугольников по координате центроида вдоль выбранной оси
  std::sort(triIndices.begin(), triIndices.end(), [this, axis](int a, int b) {
    return m_triangles[a].center()[axis] < m_triangles[b].center()[axis];
  });

  // Разбиение по медиане (балансировка дерева)
  size_t mid = triIndices.size() / 2;
  std::vector<int> leftIndices(triIndices.begin(), triIndices.begin() + mid);
  std::vector<int> rightIndices(triIndices.begin() + mid, triIndices.end());

  node->left = buildKDTree(leftIndices, depth + 1);
  node->right = buildKDTree(rightIndices, depth + 1);
  return node;
}

/* ========== PRIMARY RAY: поиск ближайшего пересечения ========== */
// Обход KD-дерева для нахождения ближайшего пересечения с треугольниками.
// Используется оптимизация порядка обхода: сначала тот узел, в который
// луч входит раньше (по координате начала луча относительно плоскости
// разбиения).
void RayTracer::intersectKDTree(KDNode* node, const Ray& ray, float tmin,
                                float tmax, RTHit& hit) {
  if (!node || tmin > tmax || tmin > hit.t) return;

  // Проверка пересечения луча с AABB узла (slab method)
  float nodeTmin = tmin;
  float nodeTmax = tmax;
  if (!node->bounds.intersect(ray, nodeTmin, nodeTmax)) return;

  // Ранний выход: если узел дальше уже найденного пересечения
  if (nodeTmin > hit.t) return;
  if (nodeTmax > hit.t) nodeTmax = hit.t;

  if (node->isLeaf) {
    // Проверка всех треугольников в листе
    for (int idx : node->triangles) {
      float t, u, v;
      if (m_triangles[idx].intersect(ray, t, u, v)) {
        if (t > 1e-4f && t < hit.t) {
          hit.t = t;
          hit.point = ray.at(t);
          hit.normal = m_triangles[idx].getNormal(
              u, v, m_triangles[idx].material.smoothShading);
          hit.u = u;
          hit.v = v;
          hit.triangleIndex = idx;
          hit.hit = true;
        }
      }
    }
    return;
  }

  // Определение порядка обхода дочерних узлов:
  // split — координата плоскости разбиения вдоль axis
  int axis = node->axis;
  float split = (node->bounds.min[axis] + node->bounds.max[axis]) * 0.5f;

  KDNode* first = node->left;
  KDNode* second = node->right;

  // Если начало луча справа от плоскости — обходим справа налево
  if (ray.origin[axis] > split) {
    std::swap(first, second);
  } else if (std::abs(ray.origin[axis] - split) < 1e-4f &&
             ray.direction[axis] > 0) {
    // Если начало на плоскости и луч направлен вправо — сначала правый
    std::swap(first, second);
  }

  intersectKDTree(first, ray, nodeTmin, nodeTmax, hit);
  intersectKDTree(second, ray, nodeTmin, nodeTmax, hit);
}

/* ========== SHADOW RAY: поиск ЛЮБОГО пересечения до maxDist ========== */
// Упрощённый обход KD-дерева для теневых лучей: возвращает true при первом
// пересечении, не требуя нахождения ближайшего (early exit optimization).
bool RayTracer::intersectKDTreeShadow(KDNode* node, const Ray& ray, float tmin,
                                      float tmax, float maxDist) {
  if (!node || tmin > tmax || tmin > maxDist) return false;

  float nodeTmin = tmin;
  float nodeTmax = tmax;
  if (!node->bounds.intersect(ray, nodeTmin, nodeTmax)) return false;
  if (nodeTmin > maxDist) return false;
  if (nodeTmax > maxDist) nodeTmax = maxDist;

  if (node->isLeaf) {
    for (int idx : node->triangles) {
      float t, u, v;
      if (m_triangles[idx].intersect(ray, t, u, v)) {
        if (t > 1e-4f && t < maxDist) {
          return true;  // найдено любое пересечение — точка в тени
        }
      }
    }
    return false;
  }

  // Оптимизация порядка обхода для теневых лучей (аналогично primary ray)
  int axis = node->axis;
  float split = (node->bounds.min[axis] + node->bounds.max[axis]) * 0.5f;

  KDNode* first = node->left;
  KDNode* second = node->right;

  if (ray.origin[axis] > split) {
    std::swap(first, second);
  } else if (std::abs(ray.origin[axis] - split) < 1e-4f &&
             ray.direction[axis] > 0) {
    std::swap(first, second);
  }

  if (intersectKDTreeShadow(first, ray, nodeTmin, nodeTmax, maxDist))
    return true;
  if (intersectKDTreeShadow(second, ray, nodeTmin, nodeTmax, maxDist))
    return true;
  return false;
}

// ---------- Поиск ближайшего пересечения ----------
// Комбинированный поиск: аналитические примитивы + полигональная сетка
// (KD-tree) + пол. Возвращает ближайшее пересечение по t-расстоянию.
bool RayTracer::findClosestHit(const Ray& ray, float tmin, float tmax,
                               RTHit& outHit) {
  outHit.t = std::numeric_limits<float>::infinity();
  outHit.hit = false;

  // Аналитические примитивы (сфера, цилиндр, конус, куб, пирамида)
  for (const auto& prim : m_analyticPrimitives) {
    RTHit h;
    if (prim->intersect(ray, tmin, outHit.t, h) && h.t < outHit.t) {
      outHit = h;
      outHit.analyticPrimitive = prim.get();
      outHit.hit = true;
    }
  }

  // Полигональная сетка через KD-дерево
  if (m_root) {
    intersectKDTree(m_root, ray, tmin, outHit.t, outHit);
  }

  // Пол (плоскость Y = -1.0)
  if (m_rtSettings.showFloor) {
    RTHit floorHit;
    if (intersectFloor(ray, floorHit) && floorHit.t < outHit.t) {
      outHit = floorHit;
    }
  }

  return outHit.hit;
}

// ---------- Выбор материала по типу объекта ----------
RTMaterial RayTracer::resolveMaterial(const RTHit& hit) const {
  RTMaterial mat = m_material;  // fallback-материал по умолчанию

  if (hit.analyticPrimitive) {
    auto it = m_analyticMaterials.find(hit.analyticPrimitive);
    if (it != m_analyticMaterials.end()) {
      const Material& src = it->second;
      mat.color = QVector3D(src.baseColor.redF(), src.baseColor.greenF(),
                            src.baseColor.blueF());
      mat.reflectivity = src.reflectivity;
      mat.transparency = src.transparency;
      mat.ior = src.ior;
      mat.roughness = src.roughness;
      mat.smoothShading = src.smoothShading;
    } else {
      mat.color = QVector3D(m_renderSettings.material.baseColor.redF(),
                            m_renderSettings.material.baseColor.greenF(),
                            m_renderSettings.material.baseColor.blueF());
    }
  } else if (hit.triangleIndex >= 0) {
    mat = m_triangles[hit.triangleIndex].material;
  } else {
    // Пол: шахматная текстура + небольшое отражение и шероховатость
    mat.color = getFloorColor(hit.point);
    mat.reflectivity = 0.1f;
    mat.roughness = 0.8f;
    mat.transparency = 0.0f;
    mat.smoothShading = true;
  }

  return mat;
}

// ---------- Геометрическая нормаль (для bias) ----------
// Возвращает face normal (нормаль грани) для треугольников или hit.normal
// для аналитических примитивов. Инвертирует нормаль, если она смотрит
// в сторону луча (back-face).
QVector3D RayTracer::computeGeomNormal(const RTHit& hit, const Ray& ray) const {
  QVector3D geomN = hit.normal;
  if (hit.triangleIndex >= 0) {
    geomN = m_triangles[hit.triangleIndex].faceNormal;
  }
  // Если нормаль смотрит против луча (внутрь объекта), переворачиваем её
  if (QVector3D::dotProduct(geomN, ray.direction) > 0.0f) {
    geomN = -geomN;
  }
  return geomN;
}

// ---------- Прямое освещение (Фонг + тени) ----------
// Модель освещения Фонга (Phong reflection model):
// color = ambient + Σ [diffuse + specular] по всем источникам
// diffuse  = max(0, N·L) * light_color * material_color
// specular = specular_strength * max(0, V·R)^shininess * light_color
// где R = reflect(-L, N) — отражённый вектор света
QVector3D RayTracer::shadeSurface(const RTHit& hit, const RTMaterial& mat,
                                  const Ray& ray) {
  QVector3D N = hit.normal;
  // Ориентация нормали: всегда против луча (двустороннее освещение)
  if (QVector3D::dotProduct(N, ray.direction) > 0.0f) N = -N;
  QVector3D V = (-ray.direction).normalized();  // вектор взгляда (View)
  QVector3D geomN = computeGeomNormal(hit, ray);

  QVector3D color(0, 0, 0);

  // Ambient (фоновое освещение): Ia = ambient_strength * light_color *
  // material_color
  QVector3D ambient = m_renderSettings.ambientStrength *
                      QVector3D(m_renderSettings.pointLights[0].color.redF(),
                                m_renderSettings.pointLights[0].color.greenF(),
                                m_renderSettings.pointLights[0].color.blueF()) *
                      mat.color;

  // Суммирование вкладов всех источников света
  for (int i = 0; i < RenderSettings::MAX_LIGHTS; ++i) {
    const auto& light = m_renderSettings.pointLights[i];
    if (!light.enabled) continue;

    RTLight rtLight;
    rtLight.position = light.position;
    // Интенсивность света: color * intensity
    rtLight.color = QVector3D(light.color.redF(), light.color.greenF(),
                              light.color.blueF()) *
                    light.intensity;

    // L = normalize(light_pos - hit_point) — направление к источнику
    QVector3D lightDir = (rtLight.position - hit.point).normalized();

    // Проверка тени: луч от точки поверхности к источнику
    bool inShadow = isInShadow(hit.point, rtLight.position, geomN);
    if (inShadow) continue;

    // Diffuse: Id = max(0, N·L) * light_color * material_color
    float diff = std::max(0.0f, QVector3D::dotProduct(N, lightDir));
    QVector3D diffuse = diff * rtLight.color * mat.color;

    // Specular: Is = specular_strength * max(0, V·R)^shininess * light_color
    // R = reflect(-L, N) = 2*(N·L)*N - L — отражённый вектор света
    QVector3D reflectDir = reflect(-lightDir, N);
    float spec = std::pow(std::max(0.0f, QVector3D::dotProduct(V, reflectDir)),
                          m_renderSettings.shininess);
    QVector3D specular =
        m_renderSettings.specularStrength * spec * rtLight.color;

    color += diffuse + specular;
  }

  return color;
}

// ---------- Проверка теней ----------
// Теневой луч: origin = point + geomNormal * SHADOW_BIAS (чтобы не зацепить
// собственную поверхность), direction = normalize(lightPos - point).
// maxDist = |lightPos - point| — дистанция до источника.
bool RayTracer::isInShadow(const QVector3D& point, const QVector3D& lightPos,
                           const QVector3D& geomNormal) {
  QVector3D lightDir = (lightPos - point).normalized();
  Ray shadowRay(point + geomNormal * SHADOW_BIAS, lightDir);
  float maxDist = (lightPos - point).length();

  if (!m_analyticPrimitives.empty()) {
    if (intersectAnalyticShadow(shadowRay, RAY_TMIN, maxDist)) return true;
  }
  if (m_root) {
    if (intersectKDTreeShadow(m_root, shadowRay, 0.0f,
                              std::numeric_limits<float>::infinity(), maxDist))
      return true;
  }
  return false;
}

// ---------- Отражение ----------
// Закон отражения: R = I - 2*(I·N)*N
// где I — входящий луч, N — нормаль (направленная наружу).
// При roughness > 0 добавляется случайное отклонение (perturbation)
// внутри единичной сферы для имитации шероховатой поверхности.
QVector3D RayTracer::traceReflection(const RTHit& hit, const RTMaterial& mat,
                                     const Ray& ray, int depth) {
  if (mat.reflectivity <= 0.0f || depth <= 0) return QVector3D(0, 0, 0);

  QVector3D N = hit.normal;
  if (QVector3D::dotProduct(N, ray.direction) > 0.0f) N = -N;

  QVector3D geomN = computeGeomNormal(hit, ray);

  // Отражённое направление: refl = normalize(ray.direction -
  // 2*dot(ray.direction, N)*N)
  QVector3D reflDir = reflect(ray.direction, N).normalized();
  if (mat.roughness > 0.0f) {
    // Добавление случайного шума для шероховатости: refl += random * roughness
    reflDir += randomInUnitSphere() * mat.roughness;
    reflDir.normalize();
  }

  // Смещение начала отражённого луча от поверхности для избежания
  // самопересечения
  Ray reflRay(hit.point + geomN * RAY_BIAS, reflDir);
  return traceRayRecursive(reflRay, depth - 1);
}

// ---------- Преломление ----------
// Закон Снеллиуса (Snell's law): η₁·sin(θ₁) = η₂·sin(θ₂)
// Векторное решение: T = η*I + (η*cos(θ₁) - √(1 - η²*(1 - cos²(θ₁))))*N
// где η = η₁/η₂, I — входящий луч, N — нормаль.
// Если подкоренное выражение < 0 — полное внутреннее отражение (TIR).
QVector3D RayTracer::traceRefraction(const RTHit& hit, const RTMaterial& mat,
                                     const Ray& ray, int depth) {
  if (mat.transparency <= 0.0f || depth <= 0) return QVector3D(0, 0, 0);

  QVector3D N = hit.normal;
  if (QVector3D::dotProduct(N, ray.direction) > 0.0f) N = -N;

  QVector3D geomN = computeGeomNormal(hit, ray);

  float ior = mat.ior;  // показатель преломления материала
  // cos(θ₁) = dot(ray.direction, N) — косинус угла падения
  float cosi = std::clamp(QVector3D::dotProduct(ray.direction, N), -1.0f, 1.0f);
  QVector3D n = N;
  float etai = 1.0f, etat = ior;  // η₁ = воздух, η₂ = материал

  if (cosi < 0) {
    // Луч входит в материал (снаружи): cos(θ₁) положительный
    cosi = -cosi;
  } else {
    // Луч выходит из материала: меняем η₁ и η₂ местами, инвертируем нормаль
    std::swap(etai, etat);
    n = -N;
  }

  float eta = etai / etat;  // отношение показателей преломления
  // k = 1 - η² * (1 - cos²(θ₁)) — подкоренное выражение для cos(θ₂)
  float k = 1.0f - eta * eta * (1.0f - cosi * cosi);
  if (k < 0.0f) return QVector3D(0, 0, 0);  // полное внутреннее отражение

  // Преломлённое направление: T = η*I + (η*cos(θ₁) - √k)*n
  QVector3D refrDir =
      (ray.direction * eta + n * (eta * cosi - std::sqrt(k))).normalized();
  if (mat.roughness > 0.0f) {
    // Уменьшенное отклонение для преломления (прозрачные материалы менее
    // шероховатые)
    refrDir += randomInUnitSphere() * mat.roughness * 0.5f;
    refrDir.normalize();
  }

  // Смещение начала преломлённого луча внутрь объекта (против нормали)
  Ray refrRay(hit.point - geomN * RAY_BIAS, refrDir);
  return traceRayRecursive(refrRay, depth - 1);
}

// ---------- Главная функция трассировки ----------
// Рекурсивная трассировка луча с учётом отражения, преломления и прямого
// освещения. Формула смешивания: color = shade * (1 - reflectivity) +
// reflection * reflectivity Для прозрачности: color = color * (1 -
// transparency*(1 - fresnel)) + refraction * transparency*(1 - fresnel)
// Fresnel-коэффициент определяет долю отражённого света на границе диэлектрика.
QVector3D RayTracer::traceRayRecursive(const Ray& ray, int depth) {
  if (depth <= 0) return QVector3D(0, 0, 0);

  RTHit hit;
  if (!findClosestHit(ray, RAY_TMIN, std::numeric_limits<float>::infinity(),
                      hit)) {
    // Промах — возвращаем цвет фона
    return QVector3D(m_renderSettings.backgroundColor.redF(),
                     m_renderSettings.backgroundColor.greenF(),
                     m_renderSettings.backgroundColor.blueF());
  }

  RTMaterial mat = resolveMaterial(hit);
  QVector3D color = shadeSurface(hit, mat, ray);

  // Отражение: смешиваем прямое освещение с отражённым цветом
  QVector3D reflColor = traceReflection(hit, mat, ray, depth);
  color = color * (1.0f - mat.reflectivity) + reflColor * mat.reflectivity;

  // Преломление: смешиваем с учётом коэффициента Френеля
  QVector3D refrColor = traceRefraction(hit, mat, ray, depth);
  if (mat.transparency > 0.0f) {
    float fresnelCoeff = fresnel(ray.direction, hit.normal, mat.ior);
    // Формула смешивания: учитываем, что часть света отражается (fresnel),
    // а часть преломляется (1 - fresnel)
    color = color * (1.0f - mat.transparency * (1.0f - fresnelCoeff)) +
            refrColor * mat.transparency * (1.0f - fresnelCoeff);
  }

  return color;
}

// Ограничение цвета диапазоном [0, 1] (tone mapping / clamping)
QVector3D RayTracer::traceRay(const Ray& ray, int depth) {
  QVector3D col = traceRayRecursive(ray, depth);
  col.setX(std::min(1.0f, std::max(0.0f, col.x())));
  col.setY(std::min(1.0f, std::max(0.0f, col.y())));
  col.setZ(std::min(1.0f, std::max(0.0f, col.z())));
  return col;
}

// ---------- Генерация случайной точки в единичной сфере ----------
// Rejection sampling: генерируем точки в кубе [-1,1]³ и отбрасываем те,
// что лежат вне сферы (|p|² ≥ 1). Распределение равномерное по объёму.
QVector3D RayTracer::randomInUnitSphere() {
  while (true) {
    QVector3D p(m_dist(m_rng) * 2.0f - 1.0f, m_dist(m_rng) * 2.0f - 1.0f,
                m_dist(m_rng) * 2.0f - 1.0f);
    if (p.lengthSquared() < 1.0f) return p;
  }
}

// ---------- Отражение вектора относительно нормали ----------
// Формула: R = V - 2*(V·N)*N
// Геометрический смысл: зеркальное отражение вектора V от плоскости с нормалью
// N.
QVector3D RayTracer::reflect(const QVector3D& v, const QVector3D& n) {
  return v - 2 * QVector3D::dotProduct(v, n) * n;
}

// ---------- Преломление вектора (векторная форма закона Снеллиуса) ----------
// Разложение вектора на составляющие:
// r_out_perp  = η * (uv + cos(θ)*n)   — перпендикулярная плоскости падения
// r_out_parallel = -√(1 - |r_out_perp|²) * n — параллельная (вдоль нормали)
// Результат: r_out = r_out_perp + r_out_parallel
QVector3D RayTracer::refract(const QVector3D& uv, const QVector3D& n,
                             float etai_over_etat) {
  float cos_theta = std::min(QVector3D::dotProduct(-uv, n), 1.0f);
  QVector3D r_out_perp = etai_over_etat * (uv + cos_theta * n);
  QVector3D r_out_parallel =
      -std::sqrt(std::fabs(1.0f - r_out_perp.lengthSquared())) * n;
  return r_out_perp + r_out_parallel;
}

// ---------- Коэффициент Френеля (Fresnel equations) ----------
// Вычисляет долю отражённого света на границе двух диэлектриков.
// Используются уравнения Френеля для s- и p-поляризаций:
// Rs = ((η₂*cos(θ₁) - η₁*cos(θ₂)) / (η₂*cos(θ₁) + η₁*cos(θ₂)))²
// Rp = ((η₁*cos(θ₁) - η₂*cos(θ₂)) / (η₁*cos(θ₁) + η₂*cos(θ₂)))²
// Fresnel = (Rs + Rp) / 2 — усреднение по неполяризованному свету.
// При sin(θ₂) ≥ 1 (TIR) возвращается 1.0 (полное отражение).
float RayTracer::fresnel(const QVector3D& I, const QVector3D& N, float ior) {
  float cosi = std::clamp(QVector3D::dotProduct(I, N), -1.0f, 1.0f);
  float etai = 1.0f, etat = ior;
  if (cosi > 0) std::swap(etai, etat);
  // sin(θ₂) = (η₁/η₂) * sin(θ₁) = (η₁/η₂) * √(1 - cos²(θ₁))
  float sint = etai / etat * std::sqrt(std::max(0.0f, 1.0f - cosi * cosi));
  if (sint >= 1.0f) return 1.0f;  // полное внутреннее отражение
  // cos(θ₂) = √(1 - sin²(θ₂))
  float cost = std::sqrt(std::max(0.0f, 1.0f - sint * sint));
  cosi = std::fabs(cosi);
  // Rs и Rp — коэффициенты отражения для s- и p-поляризаций
  float Rs = ((etat * cosi) - (etai * cost)) / ((etat * cosi) + (etai * cost));
  float Rp = ((etai * cosi) - (etat * cost)) / ((etai * cosi) + (etat * cost));
  return (Rs * Rs + Rp * Rp) / 2.0f;
}

// ---------- Пересечение луча с плоскостью пола ----------
// Плоскость пола: Y = -1.0. Уравнение пересечения:
// ray.origin.y + t * ray.direction.y = planeY
// => t = (planeY - origin.y) / direction.y
// direction.y ≈ 0 означает параллельность луча плоскости.
bool RayTracer::intersectFloor(const Ray& ray, RTHit& hit) {
  float planeY = -1.0f;
  if (std::fabs(ray.direction.y()) < 1e-6f) return false;
  float t = (planeY - ray.origin.y()) / ray.direction.y();
  if (t < 1e-4f || t > hit.t) return false;
  hit.t = t;
  hit.point = ray.at(t);
  hit.normal = QVector3D(0, 1, 0);  // нормаль поля вверх
  hit.triangleIndex = -1;
  hit.hit = true;
  return true;
}

// ---------- Шахматная текстура пола ----------
// Определяет цвет клетки по координатам точки:
// x_cell = floor(point.x * 2), z_cell = floor(point.z * 2)
// Цвет чёрный/белый зависит от чётности (x_cell + z_cell) % 2
QVector3D RayTracer::getFloorColor(const QVector3D& point) const {
  int x = static_cast<int>(std::floor(point.x() * 2.0f));
  int z = static_cast<int>(std::floor(point.z() * 2.0f));
  bool check = ((x + z) % 2) == 0;
  return check ? QVector3D(0.9f, 0.9f, 0.9f) : QVector3D(0.3f, 0.3f, 0.3f);
}

void RayTracer::setCamera(const ICamera& camera) {
  m_cameraPos = camera.getPosition();
  QVector3D target = camera.getTarget();
  // forward = normalize(target - position) — направление взгляда
  m_cameraForward = (target - m_cameraPos).normalized();

  // Построение ортонормированного базиса камеры (look-at):
  // right = normalize(forward × worldUp), up = normalize(right × forward)
  QVector3D worldUp(0, 1, 0);
  m_cameraRight =
      QVector3D::crossProduct(m_cameraForward, worldUp).normalized();

  if (m_cameraRight.lengthSquared() < 0.001f) {
    // Защита от вырождения (камера смотрит точно вверх/вниз)
    m_cameraRight = QVector3D(1, 0, 0);
  }

  m_cameraUp =
      QVector3D::crossProduct(m_cameraRight, m_cameraForward).normalized();

  m_cameraFov = camera.getFov();
}

void RayTracer::setSettings(const RenderSettings& settings) {
  m_renderSettings = settings;
}

void RayTracer::setRayTraceSettings(const RayTraceSettings& rtSettings) {
  m_rtSettings = rtSettings;
}

void RayTracer::setMaterial(const RTMaterial& material) {
  m_material = material;
}

// ---------- Основной рендер ----------
// Генерация лучей для каждого пикселя изображения.
// Параметризация плоскости изображения:
// half_height = tan(FOV/2), half_width = aspect * half_height
// offsetX = (u - 0.5) * 2 * half_width, offsetY = (v - 0.5) * 2 * half_height
// direction = normalize(forward + offsetX * right - offsetY * up)
QImage RayTracer::render() {
  int w = std::min(m_rtSettings.width, 1920);
  int h = std::min(m_rtSettings.height, 1440);
  QImage image(w, h, QImage::Format_RGB32);

  m_cameraAspect = float(w) / float(h);

  // Преобразование FOV в высоту плоскости изображения:
  // half_height = tan(θ/2), где θ — вертикальный угол обзора в радианах
  float theta = qDegreesToRadians(m_cameraFov);
  float half_height = std::tan(theta / 2.0f);
  float half_width = m_cameraAspect * half_height;

  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      // Нормализованные координаты пикселя (центр пикселя: +0.5)
      float u = (float(x) + 0.5f) / float(w);
      float v = (float(y) + 0.5f) / float(h);

      // Смещение от центра плоскости изображения [-half_width, +half_width]
      float offsetX = (u - 0.5f) * 2.0f * half_width;
      float offsetY = (v - 0.5f) * 2.0f * half_height;

      // Направление луча: комбинация forward, right и up
      // offsetY вычитается, потому что в экранных координатах Y растёт вниз
      QVector3D dir =
          m_cameraForward + offsetX * m_cameraRight - offsetY * m_cameraUp;
      dir.normalize();

      Ray ray(m_cameraPos, dir);
      QVector3D col = traceRay(ray, m_rtSettings.maxDepth);
      image.setPixelColor(x, y, QColor::fromRgbF(col.x(), col.y(), col.z()));
    }
  }
  return image;
}

void RayTracer::setAnalyticPrimitives(
    const std::vector<std::shared_ptr<AnalyticPrimitive>>& primitives) {
  m_analyticPrimitives = primitives;
}

void RayTracer::clearAnalyticPrimitives() {
  m_analyticPrimitives.clear();
  m_analyticMaterials.clear();
}

// Проверка пересечения теневого луча с аналитическими примитивами
bool RayTracer::intersectAnalyticShadow(const Ray& ray, float tmin,
                                        float tmax) {
  for (const auto& prim : m_analyticPrimitives) {
    RTHit h;
    if (prim->intersect(ray, tmin, tmax, h)) {
      return true;
    }
  }
  return false;
}

}  // namespace s21