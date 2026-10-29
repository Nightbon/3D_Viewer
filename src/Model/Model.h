#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "TypesModel.h"

namespace s21 {

class AffineTransformer {
 public:
  static Matrix createIdentityMatrix();
  static Matrix createTranslationMatrix(double dx, double dy, double dz);
  static Matrix createRotationXMatrix(double angle);
  static Matrix createRotationYMatrix(double angle);
  static Matrix createRotationZMatrix(double angle);
  static Matrix createScaleMatrix(double sx, double sy, double sz);
  static Matrix translate(const Matrix& current, double dx, double dy,
                          double dz);
  static Matrix rotate(const Matrix& current, double angleX, double angleY,
                       double angleZ);
  static Matrix scale(const Matrix& current, double sx, double sy, double sz);
  static Matrix reset();
};

class Mesh : public IMesh {
 public:
  explicit Mesh(const std::vector<Vertex>& v = {},
                const std::vector<Normal>& n = {},
                const std::vector<TexCoord>& t = {},
                const std::vector<Edge>& e = {},
                const std::vector<Face>& f = {})
      : m_vertices(v), m_normals(n), m_texCoords(t), m_edges(e), m_faces(f) {}

  const std::vector<Vertex>& getVertices() const override { return m_vertices; }
  const std::vector<Normal>& getNormals() const override { return m_normals; }
  const std::vector<Edge>& getEdges() const override { return m_edges; }
  const std::vector<Face>& getFaces() const override { return m_faces; }
  size_t getVertexCount() const override { return m_vertices.size(); }
  size_t getEdgeCount() const override { return m_edges.size(); }
  size_t getFaceCount() const override { return m_faces.size(); }
  size_t getNormalCount() const override { return m_normals.size(); }
  bool hasNormals() const override { return !m_normals.empty(); }
  bool hasTexCoords() const override { return !m_texCoords.empty(); }
  const std::vector<TexCoord>& getTexCoords() const override {
    return m_texCoords;
  }

  MeshData extractMeshData() const override;

 private:
  void fillWireframeData(MeshData& data) const;
  void fillSolidData(MeshData& data) const;
  VertexDataKey getVertexKey(const Face& face, size_t vertIdx) const;

  std::vector<Vertex> m_vertices;
  std::vector<Normal> m_normals;
  std::vector<TexCoord> m_texCoords;
  std::vector<Edge> m_edges;
  std::vector<Face> m_faces;
};

class SceneObject : public IObject {
 public:
  explicit SceneObject(std::shared_ptr<IMesh> mesh)
      : m_mesh(std::move(mesh)),
        m_transform(AffineTransformer::createIdentityMatrix()) {}

  std::shared_ptr<IMesh> getMesh() const override { return m_mesh; }
  Matrix getTransform() const override { return m_transform; }
  void setTransform(const Matrix& transform) override {
    m_transform = transform;
  }
  Material getMaterial() const override { return m_material; }
  void setMaterial(const Material& mat) override { m_material = mat; }

 private:
  std::shared_ptr<IMesh> m_mesh;
  Matrix m_transform;
  Material m_material;
};

class Scene : public IScene {
 public:
  Scene() : m_globalTransform(AffineTransformer::createIdentityMatrix()) {}

  bool addObject(std::shared_ptr<IObject> obj) override {
    if (m_objects.size() >= 6) return false;
    m_objects.push_back(obj);
    return true;
  }
  void removeObject(size_t index) override {
    if (index < m_objects.size()) m_objects.erase(m_objects.begin() + index);
  }
  std::shared_ptr<IObject> getObject(size_t index) const override {
    return (index < m_objects.size()) ? m_objects[index] : nullptr;
  }
  const std::vector<std::shared_ptr<IObject>>& getObjects() const override {
    return m_objects;
  }
  size_t getObjectCount() const override { return m_objects.size(); }

  void setGlobalTransform(const Matrix& transform) override {
    m_globalTransform = transform;
  }
  Matrix getGlobalTransform() const override { return m_globalTransform; }

 private:
  std::vector<std::shared_ptr<IObject>> m_objects;
  Matrix m_globalTransform;
};

}  // namespace s21