#include "ObjParser.h"

#include <algorithm>
#include <cmath>
#include <fstream>

#include "Model.h"

namespace s21 {

std::shared_ptr<IMesh> ObjParser::load(const std::string& filepath) {
  std::setlocale(LC_NUMERIC, "C");

  std::vector<char> buffer;
  if (!readFileToBuffer(filepath, buffer)) {
    return nullptr;
  }

  std::vector<Vertex> vertices;
  std::vector<Normal> normals;
  std::vector<TexCoord> texCoords;
  std::vector<Edge> edges;
  std::vector<Face> faces;

  vertices.reserve(100000);
  normals.reserve(100000);
  texCoords.reserve(100000);
  edges.reserve(300000);
  faces.reserve(50000);

  char* ptr = buffer.data();
  const char* end = buffer.data() + buffer.size() - 1;

  // --- ГЛАВНЫЙ ЦИКЛ ПАРСИНГА ---
  while (ptr < end) {
    // Пропускаем пробелы в начале строки
    while (ptr < end && (*ptr == ' ' || *ptr == '\t')) ++ptr;
    if (ptr >= end) break;

    if (ptr[0] == 'v' && ptr[1] == ' ') {
      parseVertex(ptr, vertices);
    } else if (ptr[0] == 'v' && ptr[1] == 'n' && ptr[2] == ' ') {
      parseNormal(ptr, normals);
    } else if (ptr[0] == 'v' && ptr[1] == 't' && ptr[2] == ' ') {
      parseTexCoord(ptr, texCoords);
    } else if (ptr[0] == 'f' && ptr[1] == ' ') {
      parseFace(ptr, end, faces, edges, vertices.size(), texCoords.size(),
                normals.size());
    } else {
      // Пропускаем неизвестные теги
      while (ptr < end && *ptr != '\n') ++ptr;
    }

    // Переход на следующую строку
    while (ptr < end && (*ptr == '\n' || *ptr == '\r')) ++ptr;
  }

  if (vertices.empty()) {
    m_error = "No vertices found in file";
    return nullptr;
  }

  // Очистка дубликатов рёбер
  std::sort(edges.begin(), edges.end());
  auto last = std::unique(edges.begin(), edges.end());
  edges.erase(last, edges.end());

  normalizeModel(vertices);

  if (normals.empty()) {
    computeNormals(vertices, faces, normals);
  }

  return std::make_shared<Mesh>(vertices, normals, texCoords, edges, faces);
}

void ObjParser::normalizeModel(std::vector<Vertex>& vertices) {
  double minX = vertices[0].x, minY = vertices[0].y, minZ = vertices[0].z;
  double maxX = vertices[0].x, maxY = vertices[0].y, maxZ = vertices[0].z;

  for (const auto& v : vertices) {
    if (v.x < minX) minX = v.x;
    if (v.y < minY) minY = v.y;
    if (v.z < minZ) minZ = v.z;
    if (v.x > maxX) maxX = v.x;
    if (v.y > maxY) maxY = v.y;
    if (v.z > maxZ) maxZ = v.z;
  }

  double centerX = (minX + maxX) / 2.0;
  double centerY = (minY + maxY) / 2.0;
  double centerZ = (minZ + maxZ) / 2.0;

  for (auto& v : vertices) {
    v.x -= centerX;
    v.y -= centerY;
    v.z -= centerZ;
  }

  double sizeX = maxX - minX;
  double sizeY = maxY - minY;
  double sizeZ = maxZ - minZ;
  double maxSize = std::max({sizeX, sizeY, sizeZ});

  if (maxSize > 0) {
    double scale = 2.0 / maxSize;
    for (auto& v : vertices) {
      v.x *= scale;
      v.y *= scale;
      v.z *= scale;
    }
  }
}

void ObjParser::computeNormals(const std::vector<Vertex>& vertices,
                               std::vector<Face>& faces,
                               std::vector<Normal>& outNormals) {
  outNormals.assign(vertices.size(), {0.0f, 0.0f, 0.0f});

  // Считаем нормаль каждой грани и прибавляем её к вершинам
  for (auto& face : faces) {
    if (face.vertexIndices.size() < 3) continue;

    int v0 = face.vertexIndices[0];
    int v1 = face.vertexIndices[1];
    int v2 = face.vertexIndices[2];

    float dx1 = vertices[v1].x - vertices[v0].x;
    float dy1 = vertices[v1].y - vertices[v0].y;
    float dz1 = vertices[v1].z - vertices[v0].z;

    float dx2 = vertices[v2].x - vertices[v0].x;
    float dy2 = vertices[v2].y - vertices[v0].y;
    float dz2 = vertices[v2].z - vertices[v0].z;

    // Векторное произведение
    float nx = dy1 * dz2 - dz1 * dy2;
    float ny = dz1 * dx2 - dx1 * dz2;
    float nz = dx1 * dy2 - dy1 * dx2;

    for (unsigned int idx : face.vertexIndices) {
      outNormals[idx].nx += nx;
      outNormals[idx].ny += ny;
      outNormals[idx].nz += nz;
    }

    // Привязываем индекс нормали к индексу вершины
    face.normalIndices = face.vertexIndices;
  }

  // Нормализуем векторы
  for (auto& n : outNormals) {
    float len = std::sqrt(n.nx * n.nx + n.ny * n.ny + n.nz * n.nz);
    if (len > 1e-6f) {
      n.nx /= len;
      n.ny /= len;
      n.nz /= len;
    } else {
      n.nz = 1.0f;  // Дефолтная нормаль, чтобы избежать деления на ноль
    }
  }
}

bool ObjParser::readFileToBuffer(const std::string& filepath,
                                 std::vector<char>& buffer) {
  std::ifstream file(filepath, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    m_error = "Cannot open file: " + filepath;
    return false;
  }

  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  buffer.resize(size + 1);
  if (!file.read(buffer.data(), size)) {
    m_error = "Failed to read file data.";
    return false;
  }
  buffer[size] = '\0';  // Гарантируем нуль-терминатор для безопасного парсинга
  return true;
}

void ObjParser::parseVertex(char*& ptr, std::vector<Vertex>& vertices) {
  ptr += 2;
  float x = std::strtof(ptr, &ptr);
  float y = std::strtof(ptr, &ptr);
  float z = std::strtof(ptr, &ptr);
  vertices.push_back({x, y, z});
}

void ObjParser::parseNormal(char*& ptr, std::vector<Normal>& normals) {
  ptr += 3;
  float nx = std::strtof(ptr, &ptr);
  float ny = std::strtof(ptr, &ptr);
  float nz = std::strtof(ptr, &ptr);
  normals.push_back({nx, ny, nz});
}

void ObjParser::parseTexCoord(char*& ptr, std::vector<TexCoord>& texCoords) {
  ptr += 3;
  float u = std::strtof(ptr, &ptr);
  float v = std::strtof(ptr, &ptr);
  texCoords.push_back({u, v});
}

void ObjParser::parseFace(char*& ptr, const char* end, std::vector<Face>& faces,
                          std::vector<Edge>& edges, size_t vSize, size_t tSize,
                          size_t nSize) {
  ptr += 2;
  Face face;
  face.vertexIndices.reserve(4);
  face.normalIndices.reserve(4);
  face.texCoordIndices.reserve(4);

  while (ptr < end && *ptr != '\n' && *ptr != '\r') {
    if (*ptr == ' ' || *ptr == '\t') {
      ++ptr;
      continue;
    }

    char* nextPtr = nullptr;
    long vIdx = std::strtol(ptr, &nextPtr, 10);

    if (ptr == nextPtr) {
      ++ptr;
      continue;
    }
    ptr = nextPtr;

    // Парсим индекс вершины
    if (vIdx != 0) {
      if (vIdx < 0) vIdx = static_cast<long>(vSize) + vIdx + 1;
      int index = static_cast<int>(vIdx - 1);
      if (index >= 0 && static_cast<size_t>(index) < vSize) {
        face.vertexIndices.push_back(index);
      }
    }

    // Парсим текстуру и нормаль
    if (ptr < end && *ptr == '/') {
      ++ptr;  // пропускаем первый '/'

      // Индекс текстуры (vt)
      if (ptr < end && *ptr != '/' && *ptr != ' ' && *ptr != '\t') {
        long vtIdx = std::strtol(ptr, &nextPtr, 10);
        if (ptr != nextPtr) {
          if (vtIdx < 0) vtIdx = static_cast<long>(tSize) + vtIdx + 1;
          face.texCoordIndices.push_back(static_cast<unsigned int>(vtIdx - 1));
          ptr = nextPtr;
        }
      }

      // Индекс нормали (vn)
      if (ptr < end && *ptr == '/') {
        ++ptr;  // пропускаем второй '/'
        if (ptr < end && *ptr != ' ' && *ptr != '\t' && *ptr != '\n' &&
            *ptr != '\r') {
          long vnIdx = std::strtol(ptr, &nextPtr, 10);
          if (ptr != nextPtr) {
            if (vnIdx < 0) vnIdx = static_cast<long>(nSize) + vnIdx + 1;
            face.normalIndices.push_back(static_cast<unsigned int>(vnIdx - 1));
            ptr = nextPtr;
          }
        }
      }
    }

    // Пропускаем остаток слова до пробела
    while (ptr < end && *ptr != ' ' && *ptr != '\t' && *ptr != '\n' &&
           *ptr != '\r')
      ++ptr;
  }

  // Формируем уникальные рёбра для каркаса
  if (face.vertexIndices.size() >= 2) {
    for (size_t i = 0; i < face.vertexIndices.size(); ++i) {
      int v1 = face.vertexIndices[i];
      int v2 = face.vertexIndices[(i + 1) % face.vertexIndices.size()];
      if (v1 > v2) std::swap(v1, v2);
      edges.emplace_back(std::make_pair(v1, v2));
    }
    faces.push_back(std::move(face));
  }
}

}  // namespace s21