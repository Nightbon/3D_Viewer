#pragma once

#include <memory>
#include <string>
#include <vector>

#include "TypesModel.h"

namespace s21 {

class ObjParser {
 public:
  std::shared_ptr<IMesh> load(const std::string& filepath);
  const std::string& getLastError() const { return m_error; }

 private:
  std::string m_error;

  bool readFileToBuffer(const std::string& filepath, std::vector<char>& buffer);

  static void parseVertex(char*& ptr, std::vector<Vertex>& vertices);
  static void parseNormal(char*& ptr, std::vector<Normal>& normals);
  static void parseTexCoord(char*& ptr, std::vector<TexCoord>& texCoords);
  static void parseFace(char*& ptr, const char* end, std::vector<Face>& faces,
                        std::vector<Edge>& edges, size_t vSize, size_t tSize,
                        size_t nSize);

  static void normalizeModel(std::vector<Vertex>& vertices);
  static void computeNormals(const std::vector<Vertex>& vertices,
                             std::vector<Face>& faces,
                             std::vector<Normal>& outNormals);
};

}  // namespace s21