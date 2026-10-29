#pragma once
#include <memory>

#include "TypesModel.h"

namespace s21 {

class AnalyticMesh : public IMesh {
 public:
  explicit AnalyticMesh(std::shared_ptr<AnalyticPrimitive> primitive,
                        int segments = 32);

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
  MeshData extractMeshData() const override;
  const std::vector<TexCoord>& getTexCoords() const override {
    return m_texCoords;
  }
  QString name() const override {
    return m_primitive ? m_primitive->name() : "Mesh";
  }
  std::shared_ptr<AnalyticPrimitive> getAnalyticPrimitive() const override {
    return m_primitive;
  }

 private:
  void triangulate(int segments);
  void addSphere(int seg);
  void addCylinder(int seg);
  void addCone(int seg);
  void addCube();
  void addPyramid();
  void deduplicateEdges();

  std::shared_ptr<AnalyticPrimitive> m_primitive;
  std::vector<Vertex> m_vertices;
  std::vector<Normal> m_normals;
  std::vector<TexCoord> m_texCoords;
  std::vector<Edge> m_edges;
  std::vector<Face> m_faces;
};

}  // namespace s21