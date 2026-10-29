#pragma once
#include <QColor>
#include <QMatrix4x4>
#include <cstddef>
#include <memory>
#include <vector>

#include "../Includes/AnalyticPrimitive.h"
#include "../Includes/s21_matrix_oop.h"

namespace s21 {

struct Vertex {
  float x, y, z;
};

struct Normal {
  float nx, ny, nz;
};

struct TexCoord {
  float u, v;
};

struct Edge {
  unsigned int v1, v2;
  explicit Edge(const std::pair<int, int>& p)
      : v1(static_cast<unsigned int>(p.first)),
        v2(static_cast<unsigned int>(p.second)) {}
  bool operator==(const Edge& other) const {
    return v1 == other.v1 && v2 == other.v2;
  }

  bool operator<(const Edge& other) const {
    if (v1 != other.v1) return v1 < other.v1;
    return v2 < other.v2;
  }
};

struct Face {
  std::vector<unsigned int> vertexIndices;
  std::vector<unsigned int> normalIndices;
  std::vector<unsigned int> texCoordIndices;
};

struct MeshData {
  // Данные для каркаса
  std::vector<float> wireframeVertices;
  std::vector<unsigned int> wireframeIndices;

  // Данные для solid режимов
  std::vector<float> solidVertices;
  std::vector<float> normals;
  std::vector<unsigned int> solidIndices;
  std::vector<float> uvs;

  bool hasNormals = false;
  bool hasData = false;
  bool hasUvs = false;
};

struct Material {
  QColor baseColor = QColor(200, 200, 200);
  float reflectivity = 0.0f;  // Зеркальность (0.0 - матовый, 1.0 - зеркало)
  float transparency = 0.0f;  // 0.0 - непрозрачный, 1.0 - невидимый
  float ior = 1.0f;  // Коэффициент преломления (1.0 = воздух, 1.5 = стекло)
  float roughness = 0.0f;     // Шероховатость
  bool smoothShading = true;  // Для полигональных моделей
};

class IMesh {
 public:
  virtual ~IMesh() = default;
  virtual const std::vector<Vertex>& getVertices() const = 0;
  virtual const std::vector<Normal>& getNormals() const = 0;
  virtual const std::vector<Edge>& getEdges() const = 0;
  virtual const std::vector<Face>& getFaces() const = 0;
  virtual size_t getVertexCount() const = 0;
  virtual size_t getEdgeCount() const = 0;
  virtual size_t getFaceCount() const = 0;
  virtual size_t getNormalCount() const = 0;
  virtual bool hasNormals() const = 0;
  virtual bool hasTexCoords() const = 0;
  virtual MeshData extractMeshData() const = 0;
  virtual const std::vector<TexCoord>& getTexCoords() const = 0;
  virtual std::shared_ptr<AnalyticPrimitive> getAnalyticPrimitive() const {
    return nullptr;
  }
  virtual QString name() const { return "Mesh"; }
};

class IObject {
 public:
  virtual ~IObject() = default;
  virtual std::shared_ptr<IMesh> getMesh() const = 0;
  virtual Matrix getTransform() const = 0;
  virtual void setTransform(const Matrix& transform) = 0;
  virtual Material getMaterial() const = 0;
  virtual void setMaterial(const Material& mat) = 0;
};

class IScene {
 public:
  virtual ~IScene() = default;
  virtual bool addObject(std::shared_ptr<IObject> obj) = 0;
  virtual void removeObject(size_t index) = 0;
  virtual std::shared_ptr<IObject> getObject(size_t index) const = 0;
  virtual const std::vector<std::shared_ptr<IObject>>& getObjects() const = 0;
  virtual size_t getObjectCount() const = 0;
  virtual void setGlobalTransform(const Matrix& transform) = 0;
  virtual Matrix getGlobalTransform() const = 0;
};

// Ключ для уникальной комбинации вершина + нормаль + текстура
struct VertexDataKey {
  unsigned int vertexIdx;
  unsigned int normalIdx;
  unsigned int texCoordIdx;

  bool operator==(const VertexDataKey& other) const {
    return vertexIdx == other.vertexIdx && normalIdx == other.normalIdx &&
           texCoordIdx == other.texCoordIdx;
  }
};

struct VertexDataHash {
  std::size_t operator()(const VertexDataKey& key) const {
    std::size_t h1 = std::hash<unsigned int>()(key.vertexIdx);
    std::size_t h2 = std::hash<unsigned int>()(key.normalIdx);
    std::size_t h3 = std::hash<unsigned int>()(key.texCoordIdx);
    return h1 ^ (h2 << 1) ^ (h3 << 2);
  }
};

}  // namespace s21