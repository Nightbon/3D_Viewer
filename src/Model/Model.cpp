#include "Model.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

namespace s21 {

Matrix AffineTransformer::createIdentityMatrix() {
  Matrix result(4);
  for (int i = 0; i < 4; ++i) {
    result(i, i) = 1.0;
  }
  return result;
}

Matrix AffineTransformer::createTranslationMatrix(double dx, double dy,
                                                  double dz) {
  Matrix result = createIdentityMatrix();
  result(0, 3) = dx;
  result(1, 3) = dy;
  result(2, 3) = dz;
  return result;
}

Matrix AffineTransformer::createRotationXMatrix(double angle) {
  Matrix result = createIdentityMatrix();
  double c = std::cos(angle);
  double s = std::sin(angle);
  result(1, 1) = c;
  result(1, 2) = -s;
  result(2, 1) = s;
  result(2, 2) = c;
  return result;
}

Matrix AffineTransformer::createRotationYMatrix(double angle) {
  Matrix result = createIdentityMatrix();
  double c = std::cos(angle);
  double s = std::sin(angle);
  result(0, 0) = c;
  result(0, 2) = s;
  result(2, 0) = -s;
  result(2, 2) = c;
  return result;
}

Matrix AffineTransformer::createRotationZMatrix(double angle) {
  Matrix result = createIdentityMatrix();
  double c = std::cos(angle);
  double s = std::sin(angle);
  result(0, 0) = c;
  result(0, 1) = -s;
  result(1, 0) = s;
  result(1, 1) = c;
  return result;
}

Matrix AffineTransformer::createScaleMatrix(double sx, double sy, double sz) {
  Matrix result = createIdentityMatrix();
  result(0, 0) = sx;
  result(1, 1) = sy;
  result(2, 2) = sz;
  return result;
}

Matrix AffineTransformer::translate(const Matrix& current, double dx, double dy,
                                    double dz) {
  return current * createTranslationMatrix(dx, dy, dz);
}

Matrix AffineTransformer::rotate(const Matrix& current, double angleX,
                                 double angleY, double angleZ) {
  Matrix result = current;
  result = result * createRotationXMatrix(angleX);
  result = result * createRotationYMatrix(angleY);
  result = result * createRotationZMatrix(angleZ);
  return result;
}

Matrix AffineTransformer::scale(const Matrix& current, double sx, double sy,
                                double sz) {
  return current * createScaleMatrix(sx, sy, sz);
}

Matrix AffineTransformer::reset() { return createIdentityMatrix(); }

MeshData Mesh::extractMeshData() const {
  MeshData data;

  fillWireframeData(data);
  fillSolidData(data);

  data.hasData = true;
  return data;
}

void Mesh::fillWireframeData(MeshData& data) const {
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
}

void Mesh::fillSolidData(MeshData& data) const {
  std::unordered_map<VertexDataKey, unsigned int, VertexDataHash> indexMap;

  data.solidVertices.reserve(m_faces.size() * 3 * 3);
  data.normals.reserve(m_faces.size() * 3 * 3);
  data.uvs.reserve(m_faces.size() * 3 * 2);
  data.solidIndices.reserve(m_faces.size() * 3 * 3);

  for (const auto& face : m_faces) {
    if (face.vertexIndices.size() < 3) continue;

    std::vector<unsigned int> faceIndices;
    faceIndices.reserve(face.vertexIndices.size());

    for (size_t i = 0; i < face.vertexIndices.size(); ++i) {
      VertexDataKey key = getVertexKey(face, i);

      auto [it, inserted] = indexMap.try_emplace(
          key, static_cast<unsigned int>(data.solidVertices.size() / 3));

      if (inserted) {
        // Добавление новой уникальной вершины
        const auto& v = m_vertices[key.vertexIdx];
        data.solidVertices.insert(data.solidVertices.end(), {v.x, v.y, v.z});

        // Добавление нормали
        if (hasNormals() && key.normalIdx < m_normals.size()) {
          const auto& n = m_normals[key.normalIdx];
          data.normals.insert(data.normals.end(), {n.nx, n.ny, n.nz});
        } else {
          data.normals.insert(data.normals.end(), {0.0f, 0.0f, 1.0f});
        }

        // Добавление UV
        if (hasTexCoords() && key.texCoordIdx < m_texCoords.size()) {
          const auto& t = m_texCoords[key.texCoordIdx];
          data.uvs.insert(data.uvs.end(), {t.u, t.v});
        } else {
          data.uvs.insert(data.uvs.end(), {0.0f, 0.0f});
        }
      }
      faceIndices.push_back(it->second);
    }

    // Триангуляция веером
    for (size_t i = 1; i + 1 < faceIndices.size(); ++i) {
      data.solidIndices.insert(
          data.solidIndices.end(),
          {faceIndices[0], faceIndices[i], faceIndices[i + 1]});
    }
  }

  data.hasNormals = hasNormals();
  data.hasUvs = hasTexCoords();
}

VertexDataKey Mesh::getVertexKey(const Face& face, size_t vertIdx) const {
  unsigned int vIdx = face.vertexIndices[vertIdx];
  unsigned int nIdx =
      (vertIdx < face.normalIndices.size()) ? face.normalIndices[vertIdx] : 0;
  unsigned int tIdx = (vertIdx < face.texCoordIndices.size())
                          ? face.texCoordIndices[vertIdx]
                          : 0;

  if (hasNormals() && nIdx >= m_normals.size()) nIdx = 0;
  if (hasTexCoords() && tIdx >= m_texCoords.size()) tIdx = 0;

  return {vIdx, nIdx, tIdx};
}

}  // namespace s21