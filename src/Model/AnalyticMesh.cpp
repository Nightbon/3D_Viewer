#include "AnalyticMesh.h"

#include <QtMath>
#include <algorithm>

#include "../Includes/AnalyticPrimitive.h"

namespace s21 {

AnalyticMesh::AnalyticMesh(std::shared_ptr<AnalyticPrimitive> primitive,
                           int segments)
    : m_primitive(std::move(primitive)) {
  triangulate(segments);
}

MeshData AnalyticMesh::extractMeshData() const {
  MeshData data;
  data.wireframeVertices.reserve(m_vertices.size() * 3);
  for (const auto& v : m_vertices) {
    data.wireframeVertices.push_back(v.x);
    data.wireframeVertices.push_back(v.y);
    data.wireframeVertices.push_back(v.z);
  }
  data.wireframeIndices.reserve(m_edges.size() * 2);
  for (const auto& e : m_edges) {
    data.wireframeIndices.push_back(e.v1);
    data.wireframeIndices.push_back(e.v2);
  }

  data.solidVertices.reserve(m_vertices.size() * 3);
  data.normals.reserve(m_normals.size() * 3);
  data.uvs.reserve(m_texCoords.size() * 2);
  for (size_t i = 0; i < m_vertices.size(); ++i) {
    data.solidVertices.push_back(m_vertices[i].x);
    data.solidVertices.push_back(m_vertices[i].y);
    data.solidVertices.push_back(m_vertices[i].z);
    if (i < m_normals.size()) {
      data.normals.push_back(m_normals[i].nx);
      data.normals.push_back(m_normals[i].ny);
      data.normals.push_back(m_normals[i].nz);
    }
    if (i < m_texCoords.size()) {
      data.uvs.push_back(m_texCoords[i].u);
      data.uvs.push_back(m_texCoords[i].v);
    }
  }
  for (const auto& f : m_faces) {
    for (size_t i = 1; i + 1 < f.vertexIndices.size(); ++i) {
      data.solidIndices.push_back(f.vertexIndices[0]);
      data.solidIndices.push_back(f.vertexIndices[i]);
      data.solidIndices.push_back(f.vertexIndices[i + 1]);
    }
  }
  data.hasNormals = !m_normals.empty();
  data.hasUvs = !m_texCoords.empty();
  data.hasData = true;
  return data;
}

void AnalyticMesh::triangulate(int segments) {
  if (!m_primitive) return;
  QString namePrim = m_primitive->name();
  if (namePrim == "Sphere")
    addSphere(segments);
  else if (namePrim == "Cylinder")
    addCylinder(segments);
  else if (namePrim == "Cone")
    addCone(segments);
  else if (namePrim == "Cube")
    addCube();
  else if (namePrim == "Pyramid")
    addPyramid();
}

void AnalyticMesh::addSphere(int seg) {
  // Параметрическое уравнение сферы в сферических координатах:
  // x = sin(phi) * cos(theta)
  // y = cos(phi)
  // z = sin(phi) * sin(theta)
  // где phi ∈ [0, π] — полярный угол (от полюса к полюсу),
  // theta ∈ [0, 2π] — азимутальный угол (вокруг оси Y)
  int stacks = seg / 2;
  int slices = seg;
  for (int i = 0; i <= stacks; ++i) {
    float phi = M_PI * i / stacks;  // phi изменяется от 0 до π
    for (int j = 0; j <= slices; ++j) {
      float theta = 2 * M_PI * j / slices;  // theta изменяется от 0 до 2π
      // Вычисление точки на единичной сфере по сферическим координатам
      float x = std::sin(phi) * std::cos(theta);
      float y = std::cos(phi);
      float z = std::sin(phi) * std::sin(theta);
      m_vertices.push_back({x, y, z});
      // Нормаль к сфере совпадает с радиус-вектором (для единичной сферы)
      m_normals.push_back({x, y, z});
      // UV-развертка: u — долгота, v — широта
      m_texCoords.push_back(
          {static_cast<float>(j) / slices, static_cast<float>(i) / stacks});
    }
  }
  // Триангуляция сетки: каждый квад разбивается на 2 треугольника
  for (int i = 0; i < stacks; ++i) {
    for (int j = 0; j < slices; ++j) {
      int a = i * (slices + 1) + j;
      int b = a + slices + 1;
      Face f1, f2;
      // Первый треугольник квада: (a, b, a+1)
      f1.vertexIndices = {static_cast<unsigned int>(a),
                          static_cast<unsigned int>(b),
                          static_cast<unsigned int>(a + 1)};
      // Второй треугольник квада: (b, b+1, a+1)
      f2.vertexIndices = {static_cast<unsigned int>(b),
                          static_cast<unsigned int>(b + 1),
                          static_cast<unsigned int>(a + 1)};
      m_faces.push_back(f1);
      m_faces.push_back(f2);
      // Рёбра каркаса: горизонтальное и вертикальное
      m_edges.emplace_back(std::make_pair(a, a + 1));
      m_edges.emplace_back(std::make_pair(a, b));
    }
  }
}

void AnalyticMesh::addCylinder(int seg) {
  // Параметрическое уравнение боковой поверхности цилиндра:
  // x = R * cos(theta), z = R * sin(theta), y ∈ [-H/2, H/2]
  float radius = 1.0f, height = 2.0f;
  int slices = seg;
  for (int i = 0; i <= slices; ++i) {
    float theta = 2 * M_PI * i / slices;  // угол вокруг оси Y
    float x = radius * std::cos(theta);
    float z = radius * std::sin(theta);
    for (int j = 0; j < 2; ++j) {
      // j = 0 — нижнее основание (y = -H/2), j = 1 — верхнее (y = +H/2)
      float y = (j == 0) ? -height * 0.5f : height * 0.5f;
      m_vertices.push_back({x, y, z});
      // Нормаль боковой поверхности: горизонтальная составляющая радиус-вектора
      m_normals.push_back({x / radius, 0.0f, z / radius});
      m_texCoords.push_back(
          {static_cast<float>(i) / slices, static_cast<float>(j)});
    }
  }
  // Треугольники боковой поверхности (каждый сегмент — 2 треугольника)
  for (int i = 0; i < slices; ++i) {
    int base = i * 2;
    Face f1, f2;
    f1.vertexIndices = {static_cast<unsigned int>(base),
                        static_cast<unsigned int>(base + 2),
                        static_cast<unsigned int>(base + 1)};
    f2.vertexIndices = {static_cast<unsigned int>(base + 2),
                        static_cast<unsigned int>(base + 3),
                        static_cast<unsigned int>(base + 1)};
    m_faces.push_back(f1);
    m_faces.push_back(f2);
  }
  // Нижняя крышка (круг): центр + окружность
  int capBase = m_vertices.size();
  m_vertices.push_back({0, -height * 0.5f, 0});  // центр нижней крышки
  m_normals.push_back({0, -1, 0});               // нормаль вниз
  m_texCoords.push_back({0.5f, 0.5f});  // центр UV-пространства
  for (int i = 0; i <= slices; ++i) {
    float theta = 2 * M_PI * i / slices;
    m_vertices.push_back(
        {radius * std::cos(theta), -height * 0.5f, radius * std::sin(theta)});
    m_normals.push_back({0, -1, 0});
    // UV-координаты крышки: проекция на круг [0,1]x[0,1]
    m_texCoords.push_back(
        {0.5f + 0.5f * std::cos(theta), 0.5f + 0.5f * std::sin(theta)});
  }
  // Треугольники нижней крышки (веер от центра)
  for (int i = 0; i < slices; ++i) {
    Face f;
    f.vertexIndices = {static_cast<unsigned int>(capBase),
                       static_cast<unsigned int>(capBase + 1 + i),
                       static_cast<unsigned int>(capBase + 2 + i)};
    m_faces.push_back(f);
  }
  // Верхняя крышка (круг)
  capBase = m_vertices.size();
  m_vertices.push_back({0, height * 0.5f, 0});  // центр верхней крышки
  m_normals.push_back({0, 1, 0});               // нормаль вверх
  m_texCoords.push_back({0.5f, 0.5f});
  for (int i = 0; i <= slices; ++i) {
    float theta = 2 * M_PI * i / slices;
    m_vertices.push_back(
        {radius * std::cos(theta), height * 0.5f, radius * std::sin(theta)});
    m_normals.push_back({0, 1, 0});
    m_texCoords.push_back(
        {0.5f + 0.5f * std::cos(theta), 0.5f + 0.5f * std::sin(theta)});
  }
  // Треугольники верхней крышки (веер от центра, порядок обратный для
  // правильной ориентации)
  for (int i = 0; i < slices; ++i) {
    Face f;
    f.vertexIndices = {static_cast<unsigned int>(capBase),
                       static_cast<unsigned int>(capBase + 2 + i),
                       static_cast<unsigned int>(capBase + 1 + i)};
    m_faces.push_back(f);
  }
  // Генерация рёбер каркаса из всех граней
  for (const auto& f : m_faces) {
    for (size_t i = 0; i < f.vertexIndices.size(); ++i) {
      int a = f.vertexIndices[i];
      int b = f.vertexIndices[(i + 1) % f.vertexIndices.size()];
      if (a > b) std::swap(a, b);
      m_edges.emplace_back(std::make_pair(a, b));
    }
  }
  deduplicateEdges();
}

void AnalyticMesh::addCone(int seg) {
  // Параметрическое уравнение конуса:
  // Основание — круг радиуса R на высоте -H/2,
  // Вершина — точка на высоте +H/2 по оси Y
  float radius = 1.0f, height = 2.0f;
  int slices = seg;
  // Основание конуса (нижняя крышка)
  int baseCenter = m_vertices.size();
  m_vertices.push_back({0, -height * 0.5f, 0});  // центр основания
  m_normals.push_back({0, -1, 0});               // нормаль вниз
  m_texCoords.push_back({0.5f, 0.5f});
  for (int i = 0; i <= slices; ++i) {
    float theta = 2 * M_PI * i / slices;
    m_vertices.push_back(
        {radius * std::cos(theta), -height * 0.5f, radius * std::sin(theta)});
    m_normals.push_back({0, -1, 0});
    m_texCoords.push_back(
        {0.5f + 0.5f * std::cos(theta), 0.5f + 0.5f * std::sin(theta)});
  }
  // Треугольники основания (веер от центра, обратный порядок)
  for (int i = 0; i < slices; ++i) {
    Face f;
    f.vertexIndices = {static_cast<unsigned int>(baseCenter),
                       static_cast<unsigned int>(baseCenter + 2 + i),
                       static_cast<unsigned int>(baseCenter + 1 + i)};
    m_faces.push_back(f);
  }
  // Вершина конуса
  int apexIdx = m_vertices.size();
  m_vertices.push_back({0, height * 0.5f, 0});
  m_normals.push_back({0, 1, 0});
  m_texCoords.push_back({0.5f, 1.0f});
  // Боковая поверхность: точки на окружности основания
  int sideBase = m_vertices.size();
  for (int i = 0; i <= slices; ++i) {
    float theta = 2 * M_PI * i / slices;
    float x = radius * std::cos(theta);
    float z = radius * std::sin(theta);
    m_vertices.push_back({x, -height * 0.5f, z});
    // Вычисление нормали боковой поверхности конуса:
    // Нормаль лежит в плоскости, образованной осью конуса и радиус-вектором
    // точки, и перпендикулярна образующей (линии от вершины к точке основания)
    QVector3D p(x, -height * 0.5f, z);  // точка на основании
    QVector3D a(0, height * 0.5f, 0);   // вершина конуса
    QVector3D axis(0, 1, 0);            // ось конуса
    // Проекция радиус-вектора на плоскость, перпендикулярную оси
    QVector3D perp = p - QVector3D::dotProduct(p, axis) * axis;
    // Нормаль = нормализованное векторное произведение [axis, perp] × (a - p)
    // Это даёт вектор, перпендикулярный образующей и лежащий в плоскости оси
    QVector3D n = QVector3D::crossProduct(
                      QVector3D::crossProduct(axis, perp).normalized(),
                      (a - p).normalized())
                      .normalized();
    m_normals.push_back({static_cast<float>(n.x()), static_cast<float>(n.y()),
                         static_cast<float>(n.z())});
    m_texCoords.push_back({static_cast<float>(i) / slices, 0.0f});
  }
  // Треугольники боковой поверхности (веер от вершины)
  for (int i = 0; i < slices; ++i) {
    Face f;
    f.vertexIndices = {static_cast<unsigned int>(apexIdx),
                       static_cast<unsigned int>(sideBase + i),
                       static_cast<unsigned int>(sideBase + i + 1)};
    m_faces.push_back(f);
  }
  // Генерация рёбер каркаса
  for (const auto& f : m_faces) {
    for (size_t i = 0; i < f.vertexIndices.size(); ++i) {
      int a = f.vertexIndices[i];
      int b = f.vertexIndices[(i + 1) % f.vertexIndices.size()];
      if (a > b) std::swap(a, b);
      m_edges.emplace_back(std::make_pair(a, b));
    }
  }
  deduplicateEdges();
}

void AnalyticMesh::addCube() {
  // Вершины куба: 8 точек с координатами ±s (s = 1.0)
  float s = 1.0f;
  QVector3D verts[8] = {{-s, -s, -s}, {s, -s, -s}, {s, s, -s}, {-s, s, -s},
                        {-s, -s, s},  {s, -s, s},  {s, s, s},  {-s, s, s}};
  // Нормали 6 граней куба (по оси -Z, +Z, -X, +X, -Y, +Y)
  Normal normals[6] = {{0, 0, -1}, {0, 0, 1},  {-1, 0, 0},
                       {1, 0, 0},  {0, -1, 0}, {0, 1, 0}};
  // Индексы вершин для каждой из 6 граней (по 4 вершины, против часовой
  // стрелки)
  int faces[6][4] = {{0, 1, 2, 3}, {4, 7, 6, 5}, {0, 4, 5, 1},
                     {2, 6, 7, 3}, {0, 3, 7, 4}, {1, 5, 6, 2}};
  for (int f = 0; f < 6; ++f) {
    int base = m_vertices.size();
    // Добавляем 4 вершины грани (каждая грань — отдельный набор вершин для
    // разрыва нормалей)
    for (int i = 0; i < 4; ++i) {
      m_vertices.push_back({verts[faces[f][i]].x(), verts[faces[f][i]].y(),
                            verts[faces[f][i]].z()});
      m_normals.push_back(normals[f]);
    }
    // Разбиение четырёхугольной грани на 2 треугольника: (0,1,2) и (0,2,3)
    Face fa, fb;
    fa.vertexIndices = {static_cast<unsigned int>(base),
                        static_cast<unsigned int>(base + 1),
                        static_cast<unsigned int>(base + 2)};
    fb.vertexIndices = {static_cast<unsigned int>(base),
                        static_cast<unsigned int>(base + 2),
                        static_cast<unsigned int>(base + 3)};
    m_faces.push_back(fa);
    m_faces.push_back(fb);
  }
  // Генерация рёбер каркаса
  for (const auto& f : m_faces) {
    for (size_t i = 0; i < f.vertexIndices.size(); ++i) {
      int a = f.vertexIndices[i];
      int b = f.vertexIndices[(i + 1) % f.vertexIndices.size()];
      if (a > b) std::swap(a, b);
      m_edges.emplace_back(std::make_pair(a, b));
    }
  }
  deduplicateEdges();
}

void AnalyticMesh::addPyramid() {
  // Пирамида с квадратным основанием:
  // Основание — квадрат в плоскости Y = -h/2, вершина — точка (0, +h/2, 0)
  float b = 1.0f, h = 1.5f;
  float yBase = -h * 0.5f;  // высота основания
  float yApex = h * 0.5f;   // высота вершины

  QVector3D baseVerts[4] = {QVector3D(-b, yBase, -b), QVector3D(b, yBase, -b),
                            QVector3D(b, yBase, b), QVector3D(-b, yBase, b)};
  QVector3D apexPt = QVector3D(0, yApex, 0);

  int baseIndices[4];
  for (int i = 0; i < 4; ++i) {
    baseIndices[i] = static_cast<int>(m_vertices.size());
    Vertex v;
    v.x = baseVerts[i].x();
    v.y = baseVerts[i].y();
    v.z = baseVerts[i].z();
    m_vertices.push_back(v);
  }

  int apexIndex = static_cast<int>(m_vertices.size());
  Vertex apexVertex;
  apexVertex.x = apexPt.x();
  apexVertex.y = apexPt.y();
  apexVertex.z = apexPt.z();
  m_vertices.push_back(apexVertex);

  // Основание пирамиды — 2 треугольника, покрывающих квадрат
  Face f1, f2;
  f1.vertexIndices = {static_cast<unsigned int>(baseIndices[0]),
                      static_cast<unsigned int>(baseIndices[1]),
                      static_cast<unsigned int>(baseIndices[2])};
  f2.vertexIndices = {static_cast<unsigned int>(baseIndices[0]),
                      static_cast<unsigned int>(baseIndices[2]),
                      static_cast<unsigned int>(baseIndices[3])};
  m_faces.push_back(f1);
  m_faces.push_back(f2);

  // Боковые грани пирамиды (4 треугольника)
  for (int i = 0; i < 4; ++i) {
    int next = (i + 1) % 4;
    // Нормаль боковой грани через векторное произведение двух рёбер:
    // n = (base[next] - base[i]) × (apex - base[i])
    // Порядок вершин обеспечивает направление наружу
    QVector3D n = QVector3D::crossProduct(baseVerts[next] - baseVerts[i],
                                          apexPt - baseVerts[i])
                      .normalized();

    int sideBase = static_cast<int>(m_vertices.size());
    Vertex v0, v1, v2;
    v0.x = baseVerts[i].x();
    v0.y = baseVerts[i].y();
    v0.z = baseVerts[i].z();
    v1.x = baseVerts[next].x();
    v1.y = baseVerts[next].y();
    v1.z = baseVerts[next].z();
    v2.x = apexPt.x();
    v2.y = apexPt.y();
    v2.z = apexPt.z();
    m_vertices.push_back(v0);
    m_vertices.push_back(v1);
    m_vertices.push_back(v2);

    Normal norm;
    norm.nx = static_cast<float>(n.x());
    norm.ny = static_cast<float>(n.y());
    norm.nz = static_cast<float>(n.z());
    m_normals.push_back(norm);
    m_normals.push_back(norm);
    m_normals.push_back(norm);

    Face f;
    f.vertexIndices = {static_cast<unsigned int>(sideBase),
                       static_cast<unsigned int>(sideBase + 1),
                       static_cast<unsigned int>(sideBase + 2)};
    m_faces.push_back(f);
  }

  // Генерация рёбер каркаса
  for (const auto& f : m_faces) {
    for (size_t i = 0; i < f.vertexIndices.size(); ++i) {
      int a = static_cast<int>(f.vertexIndices[i]);
      int b =
          static_cast<int>(f.vertexIndices[(i + 1) % f.vertexIndices.size()]);
      if (a > b) std::swap(a, b);
      m_edges.emplace_back(std::make_pair(a, b));
    }
  }
  deduplicateEdges();
}

void AnalyticMesh::deduplicateEdges() {
  std::sort(m_edges.begin(), m_edges.end());
  auto last = std::unique(m_edges.begin(), m_edges.end());
  m_edges.erase(last, m_edges.end());
}

}  // namespace s21