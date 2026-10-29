#include <gtest/gtest.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <numeric>
#include <sstream>

#include "../Model/AnalyticMesh.h"
#include "../Model/Model.h"
#include "../Model/ObjParser.h"
#include "../Model/TypesModel.h"

using namespace s21;

class AffineTransformerTeste : public ::testing::Test {
 protected:
  const double EPSILON = 1e-6;

  bool MatrixEquals(const Matrix& a, const Matrix& b) {
    for (int i = 0; i < 4; ++i) {
      for (int j = 0; j < 4; ++j) {
        if (std::abs(a(i, j) - b(i, j)) > EPSILON) {
          return false;
        }
      }
    }
    return true;
  }
};

TEST_F(AffineTransformerTeste, CreateIdentityMatrix) {
  Matrix identity = AffineTransformer::createIdentityMatrix();

  EXPECT_DOUBLE_EQ(identity(0, 0), 1.0);
  EXPECT_DOUBLE_EQ(identity(1, 1), 1.0);
  EXPECT_DOUBLE_EQ(identity(2, 2), 1.0);
  EXPECT_DOUBLE_EQ(identity(3, 3), 1.0);

  EXPECT_DOUBLE_EQ(identity(0, 1), 0.0);
  EXPECT_DOUBLE_EQ(identity(0, 2), 0.0);
  EXPECT_DOUBLE_EQ(identity(0, 3), 0.0);
  EXPECT_DOUBLE_EQ(identity(1, 0), 0.0);
  EXPECT_DOUBLE_EQ(identity(1, 2), 0.0);
  EXPECT_DOUBLE_EQ(identity(1, 3), 0.0);
  EXPECT_DOUBLE_EQ(identity(2, 0), 0.0);
  EXPECT_DOUBLE_EQ(identity(2, 1), 0.0);
  EXPECT_DOUBLE_EQ(identity(2, 3), 0.0);
  EXPECT_DOUBLE_EQ(identity(3, 0), 0.0);
  EXPECT_DOUBLE_EQ(identity(3, 1), 0.0);
  EXPECT_DOUBLE_EQ(identity(3, 2), 0.0);
}

TEST_F(AffineTransformerTeste, CreateTranslationMatrix) {
  Matrix translation =
      AffineTransformer::createTranslationMatrix(5.0, -3.0, 2.0);

  EXPECT_DOUBLE_EQ(translation(0, 0), 1.0);
  EXPECT_DOUBLE_EQ(translation(1, 1), 1.0);
  EXPECT_DOUBLE_EQ(translation(2, 2), 1.0);
  EXPECT_DOUBLE_EQ(translation(3, 3), 1.0);

  EXPECT_DOUBLE_EQ(translation(0, 3), 5.0);
  EXPECT_DOUBLE_EQ(translation(1, 3), -3.0);
  EXPECT_DOUBLE_EQ(translation(2, 3), 2.0);
}

TEST_F(AffineTransformerTeste, CreateRotationXMatrix_90Degrees) {
  double angle = M_PI / 2.0;
  Matrix rotX = AffineTransformer::createRotationXMatrix(angle);

  EXPECT_DOUBLE_EQ(rotX(0, 0), 1.0);
  EXPECT_NEAR(rotX(1, 1), std::cos(angle), EPSILON);
  EXPECT_NEAR(rotX(1, 2), -std::sin(angle), EPSILON);
  EXPECT_NEAR(rotX(2, 1), std::sin(angle), EPSILON);
  EXPECT_NEAR(rotX(2, 2), std::cos(angle), EPSILON);
}

TEST_F(AffineTransformerTeste, CreateRotationYMatrix_90Degrees) {
  double angle = M_PI / 2.0;
  Matrix rotY = AffineTransformer::createRotationYMatrix(angle);

  EXPECT_NEAR(rotY(0, 0), std::cos(angle), EPSILON);
  EXPECT_NEAR(rotY(0, 2), std::sin(angle), EPSILON);
  EXPECT_NEAR(rotY(2, 0), -std::sin(angle), EPSILON);
  EXPECT_NEAR(rotY(2, 2), std::cos(angle), EPSILON);
}

TEST_F(AffineTransformerTeste, CreateRotationZMatrix_90Degrees) {
  double angle = M_PI / 2.0;
  Matrix rotZ = AffineTransformer::createRotationZMatrix(angle);

  EXPECT_NEAR(rotZ(0, 0), std::cos(angle), EPSILON);
  EXPECT_NEAR(rotZ(0, 1), -std::sin(angle), EPSILON);
  EXPECT_NEAR(rotZ(1, 0), std::sin(angle), EPSILON);
  EXPECT_NEAR(rotZ(1, 1), std::cos(angle), EPSILON);
}

TEST_F(AffineTransformerTeste, CreateScaleMatrix) {
  Matrix scale = AffineTransformer::createScaleMatrix(2.0, 3.0, 0.5);

  EXPECT_DOUBLE_EQ(scale(0, 0), 2.0);
  EXPECT_DOUBLE_EQ(scale(1, 1), 3.0);
  EXPECT_DOUBLE_EQ(scale(2, 2), 0.5);
  EXPECT_DOUBLE_EQ(scale(3, 3), 1.0);
}

TEST_F(AffineTransformerTeste, Translate) {
  Matrix current = AffineTransformer::createIdentityMatrix();
  Matrix result = AffineTransformer::translate(current, 10.0, 20.0, 30.0);

  EXPECT_DOUBLE_EQ(result(0, 3), 10.0);
  EXPECT_DOUBLE_EQ(result(1, 3), 20.0);
  EXPECT_DOUBLE_EQ(result(2, 3), 30.0);
}

TEST_F(AffineTransformerTeste, Rotate) {
  Matrix current = AffineTransformer::createIdentityMatrix();
  double angle = M_PI / 4.0;
  Matrix result = AffineTransformer::rotate(current, angle, 0.0, 0.0);

  EXPECT_NEAR(result(1, 1), std::cos(angle), EPSILON);
  EXPECT_NEAR(result(1, 2), -std::sin(angle), EPSILON);
}

TEST_F(AffineTransformerTeste, Scale) {
  Matrix current = AffineTransformer::createIdentityMatrix();
  Matrix result = AffineTransformer::scale(current, 5.0, 5.0, 5.0);

  EXPECT_DOUBLE_EQ(result(0, 0), 5.0);
  EXPECT_DOUBLE_EQ(result(1, 1), 5.0);
  EXPECT_DOUBLE_EQ(result(2, 2), 5.0);
}

TEST_F(AffineTransformerTeste, Reset) {
  Matrix modified = AffineTransformer::translate(
      AffineTransformer::createIdentityMatrix(), 100.0, 200.0, 300.0);
  Matrix reset = AffineTransformer::reset();

  EXPECT_TRUE(MatrixEquals(reset, AffineTransformer::createIdentityMatrix()));
}

TEST_F(AffineTransformerTeste, CombinedTransformations) {
  Matrix transform = AffineTransformer::createIdentityMatrix();
  transform = AffineTransformer::scale(transform, 2.0, 2.0, 2.0);
  transform = AffineTransformer::translate(transform, 5.0, 0.0, 0.0);

  EXPECT_DOUBLE_EQ(transform(0, 0), 2.0);
  EXPECT_DOUBLE_EQ(transform(1, 1), 2.0);
  EXPECT_DOUBLE_EQ(transform(2, 2), 2.0);
}

// Тесты для Mesh

class MeshTeste : public ::testing::Test {
 protected:
  static std::vector<Vertex> createTestVertices() {
    return {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}};
  }

  static std::vector<Edge> createTestEdges() {
    std::vector<Edge> edges;
    edges.push_back(Edge(std::make_pair(0, 1)));
    edges.push_back(Edge(std::make_pair(1, 2)));
    edges.push_back(Edge(std::make_pair(2, 0)));
    return edges;
  }

  static std::shared_ptr<IMesh> createTestMesh() {
    std::vector<Vertex> vertices = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}};
    std::vector<Edge> edges;
    edges.push_back(Edge(std::make_pair(0, 1)));
    return std::make_shared<Mesh>(vertices, std::vector<Normal>(),
                                  std::vector<TexCoord>(), edges,
                                  std::vector<Face>());
  }
};

TEST_F(MeshTeste, ConstructorAndGetters) {
  auto vertices = createTestVertices();
  auto edges = createTestEdges();

  Mesh mesh(vertices, std::vector<Normal>(), std::vector<TexCoord>(), edges,
            std::vector<Face>());

  EXPECT_EQ(mesh.getVertexCount(), 3u);
  EXPECT_EQ(mesh.getEdgeCount(), 3u);

  const auto& meshVertices = mesh.getVertices();
  EXPECT_EQ(meshVertices.size(), 3u);
  EXPECT_DOUBLE_EQ(meshVertices[0].x, 0.0);
  EXPECT_DOUBLE_EQ(meshVertices[0].y, 0.0);
  EXPECT_DOUBLE_EQ(meshVertices[0].z, 0.0);

  const auto& meshEdges = mesh.getEdges();
  EXPECT_EQ(meshEdges.size(), 3u);
  EXPECT_EQ(meshEdges[0].v1, 0u);
  EXPECT_EQ(meshEdges[0].v2, 1u);
}

TEST_F(MeshTeste, ConstructorWithTexCoords) {
  std::vector<Vertex> vertices = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}};
  std::vector<TexCoord> texCoords = {{0.0f, 0.0f}, {1.0f, 0.0f}};
  std::vector<Edge> edges;
  edges.push_back(Edge(std::make_pair(0, 1)));

  Mesh mesh(vertices, std::vector<Normal>(), texCoords, edges,
            std::vector<Face>());

  EXPECT_TRUE(mesh.hasTexCoords());
  EXPECT_EQ(mesh.getTexCoords().size(), 2u);
  EXPECT_FLOAT_EQ(mesh.getTexCoords()[0].u, 0.0f);
  EXPECT_FLOAT_EQ(mesh.getTexCoords()[0].v, 0.0f);
}

TEST_F(MeshTeste, ConstructorWithNormals) {
  std::vector<Vertex> vertices = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}};
  std::vector<Normal> normals = {{0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}};
  std::vector<Edge> edges;
  edges.push_back(Edge(std::make_pair(0, 1)));

  Mesh mesh(vertices, normals, std::vector<TexCoord>(), edges,
            std::vector<Face>());

  EXPECT_TRUE(mesh.hasNormals());
  EXPECT_EQ(mesh.getNormalCount(), 2u);
}

TEST_F(MeshTeste, ConstructorWithFaces) {
  std::vector<Vertex> vertices = {
      {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}};
  std::vector<Edge> edges;
  edges.push_back(Edge(std::make_pair(0, 1)));
  edges.push_back(Edge(std::make_pair(1, 2)));
  edges.push_back(Edge(std::make_pair(2, 0)));

  Face face;
  face.vertexIndices = {0, 1, 2};
  std::vector<Face> faces = {face};

  Mesh mesh(vertices, std::vector<Normal>(), std::vector<TexCoord>(), edges,
            faces);

  EXPECT_EQ(mesh.getFaceCount(), 1u);
  EXPECT_EQ(mesh.getFaces()[0].vertexIndices.size(), 3u);
}

TEST_F(MeshTeste, DefaultConstructor) {
  Mesh mesh;
  EXPECT_EQ(mesh.getVertexCount(), 0u);
  EXPECT_EQ(mesh.getEdgeCount(), 0u);
  EXPECT_EQ(mesh.getFaceCount(), 0u);
  EXPECT_FALSE(mesh.hasNormals());
  EXPECT_FALSE(mesh.hasTexCoords());
}

// Тесты для Scene

// Тесты для ObjParser

class ObjParserTeste : public ::testing::Test {};

TEST_F(ObjParserTeste, LoadCubeFile) {
  ObjParser parser;
  const std::string testDataPath = "../src/Objects/";
  auto mesh = parser.load(testDataPath + "dragon.obj");

  ASSERT_NE(mesh, nullptr);
  EXPECT_EQ(mesh->getVertexCount(), 5205u);
  EXPECT_GE(mesh->getEdgeCount(), 5205u);
}

TEST_F(ObjParserTeste, LoadEmptyFile_ReturnsNullptr) {
  ObjParser parser;
  const std::string testDataPath = "../src/Objects/";
  auto mesh = parser.load(testDataPath + "empty.obj");

  EXPECT_EQ(mesh, nullptr);
  EXPECT_FALSE(parser.getLastError().empty());
}

TEST_F(ObjParserTeste, LoadNonExistentFile_ReturnsNullptr) {
  ObjParser parser;
  auto mesh = parser.load("non_existent_file.obj");

  EXPECT_EQ(mesh, nullptr);
  EXPECT_FALSE(parser.getLastError().empty());
}

TEST_F(ObjParserTeste, LoadVerticesOnlyFile) {
  ObjParser parser;
  const std::string testDataPath = "../src/Objects/";
  auto mesh = parser.load(testDataPath + "vertices_only.obj");

  ASSERT_NE(mesh, nullptr);
  EXPECT_EQ(mesh->getVertexCount(), 5205u);
}

class TempObjFile {
 public:
  explicit TempObjFile(const std::string& content)
      : filepath_(std::filesystem::temp_directory_path() / "test_model.obj") {
    std::ofstream file(filepath_);
    file << content;
    file.close();
  }

  ~TempObjFile() { std::filesystem::remove(filepath_); }

  std::string path() const { return filepath_.string(); }

 private:
  std::filesystem::path filepath_;
};

TEST(EdgeTest, ConstructorFromPair) {
  std::pair<int, int> p = {5, 10};
  Edge e(p);
  EXPECT_EQ(e.v1, 5u);
  EXPECT_EQ(e.v2, 10u);
}

TEST(EdgeTest, ConstructorFromPairNegative) {
  // Проверяем приведение отрицательных к unsigned
  std::pair<int, int> p = {-1, -2};
  Edge e(p);
  EXPECT_EQ(e.v1, static_cast<unsigned int>(-1));
  EXPECT_EQ(e.v2, static_cast<unsigned int>(-2));
}

TEST(EdgeTest, ConstructorFromPairZero) {
  std::pair<int, int> p = {0, 0};
  Edge e(p);
  EXPECT_EQ(e.v1, 0u);
  EXPECT_EQ(e.v2, 0u);
}

TEST(EdgeTest, ConstructorFromPairLargeValues) {
  std::pair<int, int> p = {1000000, 2000000};
  Edge e(p);
  EXPECT_EQ(e.v1, 1000000u);
  EXPECT_EQ(e.v2, 2000000u);
}

TEST(EdgeTest, EqualityOperator) {
  Edge e1(std::make_pair(1, 2));
  Edge e2(std::make_pair(1, 2));
  Edge e3(std::make_pair(2, 1));

  EXPECT_TRUE(e1 == e2);
  EXPECT_FALSE(e1 == e3);
}

TEST(EdgeTest, LessThanOperator) {
  Edge e1(std::make_pair(1, 2));
  Edge e2(std::make_pair(1, 3));
  Edge e3(std::make_pair(2, 1));

  EXPECT_TRUE(e1 < e2);
  EXPECT_TRUE(e1 < e3);
  EXPECT_FALSE(e2 < e1);
}

TEST(EdgeTest, SortingAndUnique) {
  // Проверяем что сортировка работает с новым конструктором
  std::vector<Edge> edges;
  edges.push_back(Edge(std::make_pair(2, 3)));
  edges.push_back(Edge(std::make_pair(1, 2)));
  edges.push_back(Edge(std::make_pair(2, 3)));  // дубликат
  edges.push_back(Edge(std::make_pair(0, 1)));

  std::sort(edges.begin(), edges.end());
  auto last = std::unique(edges.begin(), edges.end());
  edges.erase(last, edges.end());

  EXPECT_EQ(edges.size(), 3u);
  EXPECT_EQ(edges[0].v1, 0u);
  EXPECT_EQ(edges[1].v1, 1u);
  EXPECT_EQ(edges[2].v1, 2u);
}

TEST(ObjParserTest, LoadSimpleCube) {
  std::string objContent = R"(
v 0 0 0
v 1 0 0
v 1 1 0
v 0 1 0
v 0 0 1
v 1 0 1
v 1 1 1
v 0 1 1
f 1 2 3 4
f 5 6 7 8
f 1 2 6 5
f 2 3 7 6
f 3 4 8 7
f 4 1 5 8
)";

  TempObjFile tempFile(objContent);
  ObjParser parser;

  auto mesh = parser.load(tempFile.path());

  ASSERT_NE(mesh, nullptr);
  EXPECT_EQ(mesh->getVertexCount(), 8u);
  // 12 рёбер куба
  EXPECT_EQ(mesh->getEdgeCount(), 12u);
}

TEST(ObjParserTest, LoadNonExistentFile) {
  ObjParser parser;
  auto mesh = parser.load("/non/existent/file.obj");

  EXPECT_EQ(mesh, nullptr);
  EXPECT_FALSE(parser.getLastError().empty());
  EXPECT_NE(parser.getLastError().find("Cannot open file"), std::string::npos);
}

TEST(ObjParserTest, LoadEmptyFile) {
  TempObjFile tempFile("");
  ObjParser parser;

  auto mesh = parser.load(tempFile.path());

  EXPECT_EQ(mesh, nullptr);
  EXPECT_FALSE(parser.getLastError().empty());
  EXPECT_NE(parser.getLastError().find("No vertices"), std::string::npos);
}

TEST(ObjParserTest, LoadOnlyVertices) {
  std::string objContent = R"(
v 0 0 0
v 1 0 0
v 0 1 0
)";

  TempObjFile tempFile(objContent);
  ObjParser parser;

  auto mesh = parser.load(tempFile.path());

  ASSERT_NE(mesh, nullptr);
  EXPECT_EQ(mesh->getVertexCount(), 3u);
  EXPECT_EQ(mesh->getEdgeCount(), 0u);
}

TEST(ObjParserTest, LoadTriangle) {
  std::string objContent = R"(
v 0 0 0
v 1 0 0
v 0.5 1 0
f 1 2 3
)";

  TempObjFile tempFile(objContent);
  ObjParser parser;

  auto mesh = parser.load(tempFile.path());

  ASSERT_NE(mesh, nullptr);
  EXPECT_EQ(mesh->getVertexCount(), 3u);
  EXPECT_EQ(mesh->getEdgeCount(), 3u);

  // Проверяем нормализацию
  auto& vertices = mesh->getVertices();

  float minX = vertices[0].x, maxX = vertices[0].x;
  float minY = vertices[0].y, maxY = vertices[0].y;
  for (const auto& v : vertices) {
    minX = std::min(minX, v.x);
    maxX = std::max(maxX, v.x);
    minY = std::min(minY, v.y);
    maxY = std::max(maxY, v.y);
  }

  float centerX = (minX + maxX) / 2;
  float centerY = (minY + maxY) / 2;
  EXPECT_NEAR(centerX, 0.0f, 1e-6);
  EXPECT_NEAR(centerY, 0.0f, 1e-6);
}

TEST(ObjParserTest, LoadWithTextureAndNormalIndices) {
  // Парсер должен корректно извлекать только vertex index
  std::string objContent = R"(
v 0 0 0
v 1 0 0
v 0.5 1 0
vt 0 0
vt 1 0
vt 0.5 1
vn 0 0 1
f 1/1/1 2/2/1 3/3/1
)";

  TempObjFile tempFile(objContent);
  ObjParser parser;

  auto mesh = parser.load(tempFile.path());

  ASSERT_NE(mesh, nullptr);
  EXPECT_EQ(mesh->getVertexCount(), 3u);
  EXPECT_EQ(mesh->getEdgeCount(), 3u);
}

TEST(ObjParserTest, LoadWithTextureCoordinatesOnly) {
  std::string objContent = R"(
v 0 0 0
v 1 0 0
v 0.5 1 0
vt 0 0
vt 1 0
vt 0.5 1
f 1/1 2/2 3/3
)";

  TempObjFile tempFile(objContent);
  ObjParser parser;

  auto mesh = parser.load(tempFile.path());

  ASSERT_NE(mesh, nullptr);
  EXPECT_EQ(mesh->getVertexCount(), 3u);
  EXPECT_TRUE(mesh->hasTexCoords());
  EXPECT_EQ(mesh->getTexCoords().size(), 3u);
}

TEST(ObjParserTest, LoadWithNegativeTextureIndices) {
  // Отрицательные индексы текстур должны обрабатываться
  std::string objContent = R"(
v 0 0 0
v 1 0 0
v 0.5 1 0
vt 0 0
vt 1 0
vt 0.5 1
f 1/-1 2/-2 3/-3
)";

  TempObjFile tempFile(objContent);
  ObjParser parser;

  auto mesh = parser.load(tempFile.path());

  ASSERT_NE(mesh, nullptr);
  EXPECT_EQ(mesh->getVertexCount(), 3u);
  EXPECT_EQ(mesh->getEdgeCount(), 3u);
}

TEST(ObjParserTest, LoadWithMixedIndices) {
  // Смешанные индексы: v/vt/vn
  std::string objContent = R"(
v 0 0 0
v 1 0 0
v 0.5 1 0
vt 0 0
vt 1 0
vt 0.5 1
vn 0 0 1
vn 0 0 -1
f 1/1/1 2/2/1 3/3/2
)";

  TempObjFile tempFile(objContent);
  ObjParser parser;

  auto mesh = parser.load(tempFile.path());

  ASSERT_NE(mesh, nullptr);
  EXPECT_EQ(mesh->getVertexCount(), 3u);
  EXPECT_TRUE(mesh->hasTexCoords());
  EXPECT_TRUE(mesh->hasNormals());
}

// ==================== КРИТИЧЕСКИЕ: Тесты проверки границ индексов
// ====================

TEST(ObjParserTest, InvalidIndexOutOfBoundsPositive) {
  // Индекс больше количества вершин - должен быть отброшен
  std::string objContent = R"(
v 0 0 0
v 1 0 0
f 1 2 999
)";

  TempObjFile tempFile(objContent);
  ObjParser parser;

  auto mesh = parser.load(tempFile.path());

  ASSERT_NE(mesh, nullptr);
  // Индексы: 1->0, 2->1 (валидные), 999->998 (невалидный, >= vSize)
  // Рёбра: (0,1) создаётся, (1,998) и (998,0) отбрасываются
  EXPECT_EQ(mesh->getEdgeCount(), 1u);
}

TEST(ObjParserTest, InvalidIndexOutOfBoundsNegative) {
  // Отрицательный индекс после вычитания 1 становится ещё отрицательнее
  std::string objContent = R"(
v 0 0 0
v 1 0 0
f -5 -6 -7
)";

  TempObjFile tempFile(objContent);
  ObjParser parser;

  auto mesh = parser.load(tempFile.path());

  ASSERT_NE(mesh, nullptr);
  // -5 - 1 = -6, что < 0, поэтому все индексы невалидны
  EXPECT_EQ(mesh->getEdgeCount(), 0u);
}

TEST(ObjParserTest, InvalidIndexZero) {
  // Индекс 0 в OBJ означает -1 после вычитания
  std::string objContent = R"(
v 0 0 0
v 1 0 0
f 0 1 2
)";

  TempObjFile tempFile(objContent);
  ObjParser parser;

  auto mesh = parser.load(tempFile.path());

  ASSERT_NE(mesh, nullptr);
  EXPECT_EQ(mesh->getEdgeCount(), 1u);
}

TEST(ObjParserTest, MixedValidAndInvalidIndices) {
  // Смесь валидных и невалидных индексов
  std::string objContent = R"(
v 0 0 0
v 1 0 0
v 2 0 0
f 1 2 100
)";

  TempObjFile tempFile(objContent);
  ObjParser parser;

  auto mesh = parser.load(tempFile.path());

  ASSERT_NE(mesh, nullptr);
  EXPECT_EQ(mesh->getEdgeCount(), 1u);
}

TEST(ObjParserTest, EmptyFace) {
  // Грань без индексов
  std::string objContent = R"(
v 0 0 0
v 1 0 0
f
)";

  TempObjFile tempFile(objContent);
  ObjParser parser;

  auto mesh = parser.load(tempFile.path());

  ASSERT_NE(mesh, nullptr);
  EXPECT_EQ(mesh->getEdgeCount(), 0u);
}

TEST(ObjParserTest, FaceWithSingleVertex) {
  // Грань с одной вершиной - не создаёт рёбер
  std::string objContent = R"(
v 0 0 0
f 1
)";

  TempObjFile tempFile(objContent);
  ObjParser parser;

  auto mesh = parser.load(tempFile.path());

  ASSERT_NE(mesh, nullptr);
  EXPECT_EQ(mesh->getEdgeCount(), 0u);
}

TEST(ObjParserTest, FaceWithTwoVertices) {
  // Грань с двумя вершинами - одно ребро
  std::string objContent = R"(
v 0 0 0
v 1 0 0
f 1 2
)";

  TempObjFile tempFile(objContent);
  ObjParser parser;

  auto mesh = parser.load(tempFile.path());

  ASSERT_NE(mesh, nullptr);
  EXPECT_EQ(mesh->getEdgeCount(), 1u);
  // Проверяем что ребро создано правильно
  auto meshData = mesh->extractMeshData();
  EXPECT_EQ(meshData.wireframeIndices[0], 0u);
  EXPECT_EQ(meshData.wireframeIndices[1], 1u);
}

TEST(ObjParserTest, DuplicateEdgesRemoved) {
  // Два треугольника sharing edge
  std::string objContent = R"(
v 0 0 0
v 1 0 0
v 0.5 1 0
v 0.5 0.5 1
f 1 2 3
f 1 2 4
)";

  TempObjFile tempFile(objContent);
  ObjParser parser;

  auto mesh = parser.load(tempFile.path());

  ASSERT_NE(mesh, nullptr);
  // 6 рёбер, но (0,1) общее, поэтому 5 уникальных
  EXPECT_EQ(mesh->getEdgeCount(), 5u);
}

TEST(ObjParserTest, EdgeSortingInParser) {
  // Проверяем что v1 > v2 swap работает
  std::string objContent = R"(
v 0 0 0
v 1 0 0
v 2 0 0
f 3 2 1
)";

  TempObjFile tempFile(objContent);
  ObjParser parser;

  auto mesh = parser.load(tempFile.path());

  ASSERT_NE(mesh, nullptr);
  auto meshData = mesh->extractMeshData();

  // Рёбра должны быть отсортированы: (0,1), (1,2), (0,2)
  // После swap в парсере
  EXPECT_EQ(meshData.wireframeIndices[0], 0u);  // min(2,0) = 0
  EXPECT_EQ(meshData.wireframeIndices[1], 1u);
}

TEST(ObjParserTest, NormalizationScalesModel) {
  std::string objContent = R"(
v -100 -100 -100
v 100 -100 -100
v 100 100 -100
v -100 100 -100
f 1 2 3 4
)";

  TempObjFile tempFile(objContent);
  ObjParser parser;

  auto mesh = parser.load(tempFile.path());

  ASSERT_NE(mesh, nullptr);

  auto& vertices = mesh->getVertices();

  float minX = vertices[0].x, maxX = vertices[0].x;
  for (const auto& v : vertices) {
    minX = std::min(minX, v.x);
    maxX = std::max(maxX, v.x);
  }

  float size = maxX - minX;
  EXPECT_NEAR(size, 2.0f, 1e-5);
}

TEST(ObjParserTest, NormalizationCentering) {
  std::string objContent = R"(
v 10 20 30
v 20 20 30
v 15 30 30
f 1 2 3
)";

  TempObjFile tempFile(objContent);
  ObjParser parser;

  auto mesh = parser.load(tempFile.path());

  ASSERT_NE(mesh, nullptr);

  auto& vertices = mesh->getVertices();

  float sumX =
      std::accumulate(vertices.begin(), vertices.end(), 0.0f,
                      [](float sum, const Vertex& v) { return sum + v.x; });
  float centerX = sumX / vertices.size();

  EXPECT_NEAR(centerX, 0.0f, 1e-6);
}

TEST(MeshTest, Constructor) {
  std::vector<Vertex> vertices = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
  std::vector<Edge> edges;
  edges.push_back(Edge(std::make_pair(0, 1)));
  edges.push_back(Edge(std::make_pair(1, 2)));
  edges.push_back(Edge(std::make_pair(2, 0)));

  Mesh mesh(vertices, std::vector<Normal>(), std::vector<TexCoord>(), edges,
            std::vector<Face>());

  EXPECT_EQ(mesh.getVertexCount(), 3u);
  EXPECT_EQ(mesh.getEdgeCount(), 3u);
}

TEST(MeshTest, GetFlatVertices) {
  std::vector<Vertex> vertices = {{1.0f, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f}};
  std::vector<Edge> edges;

  Mesh mesh(vertices, std::vector<Normal>(), std::vector<TexCoord>(), edges,
            std::vector<Face>());
  auto meshData = mesh.extractMeshData();

  EXPECT_EQ(meshData.wireframeVertices.size(), 6u);
  EXPECT_FLOAT_EQ(meshData.wireframeVertices[0], 1.0f);
  EXPECT_FLOAT_EQ(meshData.wireframeVertices[1], 2.0f);
  EXPECT_FLOAT_EQ(meshData.wireframeVertices[2], 3.0f);
  EXPECT_FLOAT_EQ(meshData.wireframeVertices[3], 4.0f);
  EXPECT_FLOAT_EQ(meshData.wireframeVertices[4], 5.0f);
  EXPECT_FLOAT_EQ(meshData.wireframeVertices[5], 6.0f);
}

TEST(MeshTest, GetFlatIndices) {
  std::vector<Vertex> vertices = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {1, 1, 0}};
  std::vector<Edge> edges;
  edges.push_back(Edge(std::make_pair(0, 1)));
  edges.push_back(Edge(std::make_pair(1, 2)));

  Mesh mesh(vertices, std::vector<Normal>(), std::vector<TexCoord>(), edges,
            std::vector<Face>());
  auto meshData = mesh.extractMeshData();

  EXPECT_EQ(meshData.wireframeIndices.size(), 4u);
  EXPECT_EQ(meshData.wireframeIndices[0], 0u);
  EXPECT_EQ(meshData.wireframeIndices[1], 1u);
  EXPECT_EQ(meshData.wireframeIndices[2], 1u);
  EXPECT_EQ(meshData.wireframeIndices[3], 2u);
}

TEST(MeshTest, EmptyMesh) {
  std::vector<Vertex> vertices;
  std::vector<Edge> edges;

  Mesh mesh(vertices, std::vector<Normal>(), std::vector<TexCoord>(), edges,
            std::vector<Face>());

  EXPECT_EQ(mesh.getVertexCount(), 0u);
  EXPECT_EQ(mesh.getEdgeCount(), 0u);

  auto meshData = mesh.extractMeshData();
  EXPECT_TRUE(meshData.wireframeVertices.empty());
  EXPECT_TRUE(meshData.wireframeIndices.empty());
}

// ==================== Тесты Scene ====================

// ==================== Тесты AffineTransformer ====================

TEST(AffineTransformerTest, CreateIdentityMatrix) {
  Matrix m = AffineTransformer::createIdentityMatrix();

  EXPECT_EQ(m.GetRows(), 4);
  EXPECT_EQ(m.GetCols(), 4);

  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      if (i == j) {
        EXPECT_DOUBLE_EQ(m(i, j), 1.0);
      } else {
        EXPECT_DOUBLE_EQ(m(i, j), 0.0);
      }
    }
  }
}

TEST(AffineTransformerTest, CreateTranslationMatrix) {
  Matrix m = AffineTransformer::createTranslationMatrix(1.0, 2.0, 3.0);

  EXPECT_DOUBLE_EQ(m(0, 3), 1.0);
  EXPECT_DOUBLE_EQ(m(1, 3), 2.0);
  EXPECT_DOUBLE_EQ(m(2, 3), 3.0);
}

TEST(AffineTransformerTest, CreateScaleMatrix) {
  Matrix m = AffineTransformer::createScaleMatrix(2.0, 3.0, 4.0);

  EXPECT_DOUBLE_EQ(m(0, 0), 2.0);
  EXPECT_DOUBLE_EQ(m(1, 1), 3.0);
  EXPECT_DOUBLE_EQ(m(2, 2), 4.0);
}

TEST(AffineTransformerTest, CreateRotationXMatrix) {
  double angle = M_PI / 4;
  Matrix m = AffineTransformer::createRotationXMatrix(angle);

  double c = std::cos(angle);
  double s = std::sin(angle);

  EXPECT_DOUBLE_EQ(m(1, 1), c);
  EXPECT_DOUBLE_EQ(m(1, 2), -s);
  EXPECT_DOUBLE_EQ(m(2, 1), s);
  EXPECT_DOUBLE_EQ(m(2, 2), c);
}

TEST(AffineTransformerTest, CreateRotationYMatrix) {
  double angle = M_PI / 6;
  Matrix m = AffineTransformer::createRotationYMatrix(angle);

  double c = std::cos(angle);
  double s = std::sin(angle);

  EXPECT_DOUBLE_EQ(m(0, 0), c);
  EXPECT_DOUBLE_EQ(m(0, 2), s);
  EXPECT_DOUBLE_EQ(m(2, 0), -s);
  EXPECT_DOUBLE_EQ(m(2, 2), c);
}

TEST(AffineTransformerTest, CreateRotationZMatrix) {
  double angle = M_PI / 3;
  Matrix m = AffineTransformer::createRotationZMatrix(angle);

  double c = std::cos(angle);
  double s = std::sin(angle);

  EXPECT_DOUBLE_EQ(m(0, 0), c);
  EXPECT_DOUBLE_EQ(m(0, 1), -s);
  EXPECT_DOUBLE_EQ(m(1, 0), s);
  EXPECT_DOUBLE_EQ(m(1, 1), c);
}

TEST(AffineTransformerTest, TranslateOperation) {
  Matrix identity = AffineTransformer::createIdentityMatrix();
  Matrix translated = AffineTransformer::translate(identity, 5.0, -3.0, 2.0);

  EXPECT_DOUBLE_EQ(translated(0, 3), 5.0);
  EXPECT_DOUBLE_EQ(translated(1, 3), -3.0);
  EXPECT_DOUBLE_EQ(translated(2, 3), 2.0);
}

TEST(AffineTransformerTest, ScaleOperation) {
  Matrix identity = AffineTransformer::createIdentityMatrix();
  Matrix scaled = AffineTransformer::scale(identity, 2.0, 3.0, 4.0);

  EXPECT_DOUBLE_EQ(scaled(0, 0), 2.0);
  EXPECT_DOUBLE_EQ(scaled(1, 1), 3.0);
  EXPECT_DOUBLE_EQ(scaled(2, 2), 4.0);
}

TEST(AffineTransformerTest, RotateOperation) {
  Matrix identity = AffineTransformer::createIdentityMatrix();
  Matrix rotated = AffineTransformer::rotate(identity, 0.0, M_PI / 2, 0.0);

  double c = std::cos(M_PI / 2);
  double s = std::sin(M_PI / 2);

  EXPECT_NEAR(rotated(0, 0), c, 1e-10);
  EXPECT_NEAR(rotated(0, 2), s, 1e-10);
  EXPECT_NEAR(rotated(2, 0), -s, 1e-10);
  EXPECT_NEAR(rotated(2, 2), c, 1e-10);
}

TEST(AffineTransformerTest, ResetReturnsIdentity) {
  Matrix m = AffineTransformer::reset();
  Matrix identity = AffineTransformer::createIdentityMatrix();

  EXPECT_TRUE(m.EqMatrix(identity));
}

TEST(AffineTransformerTest, CombinedTransformations) {
  Matrix m = AffineTransformer::createIdentityMatrix();
  m = AffineTransformer::translate(m, 1.0, 0.0, 0.0);
  m = AffineTransformer::scale(m, 2.0, 2.0, 2.0);

  EXPECT_DOUBLE_EQ(m(0, 0), 2.0);
  EXPECT_DOUBLE_EQ(m(0, 3), 1.0);
}

// ==================== Интеграционные тесты ====================

// ==================== Граничные случаи ====================

TEST(BoundaryTest, VerySmallModel) {
  std::string objContent = R"(
v 0.001 0 0
v 0.002 0 0
v 0.0015 0.001 0
f 1 2 3
)";

  TempObjFile tempFile(objContent);
  ObjParser parser;

  auto mesh = parser.load(tempFile.path());
  ASSERT_NE(mesh, nullptr);
  EXPECT_EQ(mesh->getVertexCount(), 3u);
}

TEST(BoundaryTest, SingleVertex) {
  std::string objContent = "v 1 2 3\n";

  TempObjFile tempFile(objContent);
  ObjParser parser;

  auto mesh = parser.load(tempFile.path());
  ASSERT_NE(mesh, nullptr);
  EXPECT_EQ(mesh->getVertexCount(), 1u);
  EXPECT_EQ(mesh->getEdgeCount(), 0u);
}

TEST(BoundaryTest, LineModel) {
  std::string objContent = R"(
v 0 0 0
v 1 0 0
v 2 0 0
v 3 0 0
f 1 2 3 4
)";

  TempObjFile tempFile(objContent);
  ObjParser parser;

  auto mesh = parser.load(tempFile.path());
  ASSERT_NE(mesh, nullptr);
  EXPECT_EQ(mesh->getVertexCount(), 4u);
}

TEST(EdgeEqualityTest, AllBranchesCoverage) {
  {
    Edge e1(std::make_pair(5, 10));
    Edge e2(std::make_pair(5, 10));
    EXPECT_TRUE(e1 == e2);  // Оба условия истинны
  }

  {
    Edge e1(std::make_pair(5, 10));
    Edge e2(std::make_pair(5, 20));
    EXPECT_FALSE(e1 == e2);  // Первое условие истинно, второе ложно
  }

  {
    Edge e1(std::make_pair(5, 10));
    Edge e2(std::make_pair(6, 10));  // v2 одинаковые, но v1 разные
    EXPECT_FALSE(e1 == e2);          // Первое условие ложно
  }

  {
    Edge e1(std::make_pair(5, 10));
    Edge e2(std::make_pair(6, 20));
    EXPECT_FALSE(e1 == e2);  // Первое условие ложно
  }

  {
    unsigned int max = std::numeric_limits<unsigned int>::max();
    Edge e1(std::make_pair(static_cast<int>(max), static_cast<int>(max)));
    Edge e2(std::make_pair(static_cast<int>(max), static_cast<int>(max)));
    EXPECT_TRUE(e1 == e2);
  }

  {
    Edge e1(std::make_pair(0, 0));
    Edge e2(std::make_pair(0, 0));
    EXPECT_TRUE(e1 == e2);
  }

  {
    Edge e1(std::make_pair(0, 0));
    Edge e2(std::make_pair(
        0, static_cast<int>(std::numeric_limits<unsigned int>::max())));
    EXPECT_FALSE(e1 == e2);
  }
}

TEST(MeshExtractMeshDataTest, WireframeDataBasic) {
  std::vector<Vertex> vertices = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
  std::vector<Edge> edges;
  edges.push_back(Edge(std::make_pair(0, 1)));
  edges.push_back(Edge(std::make_pair(1, 2)));
  edges.push_back(Edge(std::make_pair(2, 0)));

  Mesh mesh(vertices, std::vector<Normal>(), std::vector<TexCoord>(), edges,
            std::vector<Face>());
  auto data = mesh.extractMeshData();

  EXPECT_TRUE(data.hasData);
  EXPECT_EQ(data.wireframeVertices.size(), 9u);  // 3 вершины * 3 координаты
  EXPECT_EQ(data.wireframeIndices.size(), 6u);  // 3 ребра * 2 индекса
}

TEST(MeshExtractMeshDataTest, SolidDataWithNormals) {
  std::string objContent = R"(
v 0 0 0
v 1 0 0
v 0.5 1 0
vn 0 0 1
f 1//1 2//1 3//1
)";

  TempObjFile tempFile(objContent);
  ObjParser parser;

  auto mesh = parser.load(tempFile.path());
  ASSERT_NE(mesh, nullptr);

  auto data = mesh->extractMeshData();

  EXPECT_TRUE(data.hasData);
  EXPECT_TRUE(data.hasNormals);
  EXPECT_GT(data.solidVertices.size(), 0u);
  EXPECT_GT(data.normals.size(), 0u);
  EXPECT_GT(data.solidIndices.size(), 0u);
}

TEST(MeshExtractMeshDataTest, SolidDataWithUVs) {
  std::string objContent = R"(
v 0 0 0
v 1 0 0
v 0.5 1 0
vt 0 0
vt 1 0
vt 0.5 1
f 1/1 2/2 3/3
)";

  TempObjFile tempFile(objContent);
  ObjParser parser;

  auto mesh = parser.load(tempFile.path());
  ASSERT_NE(mesh, nullptr);

  auto data = mesh->extractMeshData();

  EXPECT_TRUE(data.hasData);
  EXPECT_TRUE(data.hasUvs);
  EXPECT_GT(data.uvs.size(), 0u);
  EXPECT_EQ(data.uvs.size() % 2, 0u);  // UV всегда пары
}

TEST(MeshExtractMeshDataTest, SolidDataWithNormalsAndUVs) {
  std::string objContent = R"(
v 0 0 0
v 1 0 0
v 0.5 1 0
vt 0 0
vt 1 0
vt 0.5 1
vn 0 0 1
f 1/1/1 2/2/1 3/3/1
)";

  TempObjFile tempFile(objContent);
  ObjParser parser;

  auto mesh = parser.load(tempFile.path());
  ASSERT_NE(mesh, nullptr);

  auto data = mesh->extractMeshData();

  EXPECT_TRUE(data.hasData);
  EXPECT_TRUE(data.hasNormals);
  EXPECT_TRUE(data.hasUvs);
  EXPECT_GT(data.solidVertices.size(), 0u);
  EXPECT_GT(data.normals.size(), 0u);
  EXPECT_GT(data.uvs.size(), 0u);
  EXPECT_GT(data.solidIndices.size(), 0u);
}

TEST(ObjParserNormalTest, ParseNormalsFromFile) {
  std::string objContent = R"(
v 0 0 0
v 1 0 0
v 0.5 1 0
vn 0 0 1
vn 0 0 -1
f 1//1 2//1 3//1
)";

  TempObjFile tempFile(objContent);
  ObjParser parser;

  auto mesh = parser.load(tempFile.path());
  ASSERT_NE(mesh, nullptr);
  EXPECT_EQ(mesh->getNormalCount(), 2u);
}

TEST(ObjParserNormalTest, ParseNormalsWithIndices) {
  std::string objContent = R"(
v 0 0 0
v 1 0 0
v 0.5 1 0
vn 0 0 1
f 1/1/1 2/2/1 3/3/1
)";

  TempObjFile tempFile(objContent);
  ObjParser parser;

  auto mesh = parser.load(tempFile.path());
  ASSERT_NE(mesh, nullptr);

  auto data = mesh->extractMeshData();
  EXPECT_TRUE(data.hasNormals);
}

TEST(ObjParserNormalTest, MissingNormalsUsesDefault) {
  std::string objContent = R"(
v 0 0 0
v 1 0 0
v 0.5 1 0
f 1 2 3
)";

  TempObjFile tempFile(objContent);
  ObjParser parser;

  auto mesh = parser.load(tempFile.path());
  ASSERT_NE(mesh, nullptr);

  auto data = mesh->extractMeshData();
  EXPECT_GT(data.normals.size(), 0u);
}

TEST(ObjParserNormalTest, NegativeNormalIndices) {
  std::string objContent = R"(
v 0 0 0
v 1 0 0
v 0.5 1 0
vn 0 0 1
f 1//-1 2//-1 3//-1
)";

  TempObjFile tempFile(objContent);
  ObjParser parser;

  auto mesh = parser.load(tempFile.path());
  ASSERT_NE(mesh, nullptr);

  auto data = mesh->extractMeshData();
  EXPECT_TRUE(data.hasData);
}

TEST(IntegrationTest, ExtractMeshDataWorkflow) {
  std::string objContent = R"(
v -1 -1 -1
v 1 -1 -1
v 1 1 -1
v -1 1 -1
v -1 -1 1
v 1 -1 1
v 1 1 1
v -1 1 1
f 1 2 3 4
f 5 6 7 8
f 1 2 6 5
f 2 3 7 6
f 3 4 8 7
f 4 1 5 8
)";

  TempObjFile tempFile(objContent);
  ObjParser parser;

  auto mesh = parser.load(tempFile.path());
  ASSERT_NE(mesh, nullptr);

  auto data = mesh->extractMeshData();

  EXPECT_EQ(data.wireframeVertices.size(), 24u);  // 8 вершин * 3
  EXPECT_GT(data.wireframeIndices.size(), 0u);

  EXPECT_GT(data.solidVertices.size(), 0u);
  EXPECT_GT(data.solidIndices.size(), 0u);
}

TEST(ObjParserReadFileTest, ReadValidFile) {
  std::string objContent = "v 0 0 0\nv 1 0 0\n";
  TempObjFile tempFile(objContent);
  ObjParser parser;

  bool result = parser.load(tempFile.path()) != nullptr;
  EXPECT_TRUE(result);
}

TEST(ObjParserReadFileTest, ReadNonExistentFile) {
  ObjParser parser;
  auto mesh = parser.load("/non/existent/path/file.obj");
  EXPECT_EQ(mesh, nullptr);
  EXPECT_FALSE(parser.getLastError().empty());
  EXPECT_NE(parser.getLastError().find("Cannot open file"), std::string::npos);
}

TEST(MeshTest, GetNormals) {
  std::vector<Vertex> vertices = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}};
  std::vector<Normal> normals = {{0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}};
  std::vector<Edge> edges;
  edges.push_back(Edge(std::make_pair(0, 1)));

  Mesh mesh(vertices, normals, std::vector<TexCoord>(), edges,
            std::vector<Face>());

  const auto& retrievedNormals = mesh.getNormals();
  EXPECT_EQ(retrievedNormals.size(), 2u);
  EXPECT_FLOAT_EQ(retrievedNormals[0].nx, 0.0f);
  EXPECT_FLOAT_EQ(retrievedNormals[0].ny, 0.0f);
  EXPECT_FLOAT_EQ(retrievedNormals[0].nz, 1.0f);
}

// ==================== Тесты AnalyticMesh ====================

class AnalyticMeshTest : public ::testing::Test {
 protected:
  const int SEG = 32;
};

TEST_F(AnalyticMeshTest, NullPrimitive) {
  AnalyticMesh mesh(nullptr, SEG);

  EXPECT_EQ(mesh.getVertexCount(), 0u);
  EXPECT_EQ(mesh.getEdgeCount(), 0u);
  EXPECT_EQ(mesh.getFaceCount(), 0u);
  EXPECT_EQ(mesh.getNormalCount(), 0u);
  EXPECT_FALSE(mesh.hasNormals());
  EXPECT_FALSE(mesh.hasTexCoords());
  EXPECT_EQ(mesh.name(), "Mesh");
  EXPECT_EQ(mesh.getAnalyticPrimitive(), nullptr);

  auto data = mesh.extractMeshData();
  EXPECT_TRUE(data.hasData);
  EXPECT_TRUE(data.wireframeVertices.empty());
  EXPECT_TRUE(data.wireframeIndices.empty());
  EXPECT_TRUE(data.solidVertices.empty());
  EXPECT_TRUE(data.solidIndices.empty());
}

TEST_F(AnalyticMeshTest, SphereZeroSegments) {
  auto prim = std::make_shared<SpherePrimitive>(QVector3D(0, 0, 0), 1.0f);
  AnalyticMesh mesh(prim, 0);

  // stacks=0, slices=0 => цикл i=0..0, j=0..0 создаёт 1 вершину
  EXPECT_EQ(mesh.getVertexCount(), 1u);
  EXPECT_EQ(mesh.getFaceCount(), 0u);
  EXPECT_EQ(mesh.getEdgeCount(), 0u);
}

TEST_F(AnalyticMeshTest, SphereCounts) {
  auto prim = std::make_shared<SpherePrimitive>(QVector3D(0, 0, 0), 1.0f);
  AnalyticMesh mesh(prim, SEG);

  EXPECT_EQ(mesh.name(), "Sphere");
  EXPECT_EQ(mesh.getAnalyticPrimitive(), prim);
  EXPECT_TRUE(mesh.hasNormals());
  EXPECT_TRUE(mesh.hasTexCoords());

  int stacks = SEG / 2;
  int slices = SEG;
  EXPECT_EQ(mesh.getVertexCount(), (stacks + 1) * (slices + 1));
  EXPECT_EQ(mesh.getFaceCount(), stacks * slices * 2u);
  EXPECT_EQ(mesh.getEdgeCount(), stacks * slices * 2u);
  EXPECT_EQ(mesh.getNormalCount(), mesh.getVertexCount());
  EXPECT_EQ(mesh.getTexCoords().size(), mesh.getVertexCount());
}

TEST_F(AnalyticMeshTest, SphereDataConsistency) {
  auto prim = std::make_shared<SpherePrimitive>(QVector3D(0, 0, 0), 1.0f);
  AnalyticMesh mesh(prim, SEG);
  auto data = mesh.extractMeshData();

  EXPECT_TRUE(data.hasData);
  EXPECT_TRUE(data.hasNormals);
  EXPECT_TRUE(data.hasUvs);
  EXPECT_EQ(data.wireframeVertices.size(), mesh.getVertexCount() * 3);
  EXPECT_EQ(data.wireframeIndices.size(), mesh.getEdgeCount() * 2);
  EXPECT_EQ(data.solidVertices.size(), mesh.getVertexCount() * 3);
  EXPECT_EQ(data.solidIndices.size(), mesh.getFaceCount() * 3);
}

TEST_F(AnalyticMeshTest, SphereNormalsAreUnit) {
  auto prim = std::make_shared<SpherePrimitive>(QVector3D(0, 0, 0), 1.0f);
  AnalyticMesh mesh(prim, SEG);
  const auto& normals = mesh.getNormals();

  for (const auto& n : normals) {
    float len = std::sqrt(n.nx * n.nx + n.ny * n.ny + n.nz * n.nz);
    EXPECT_NEAR(len, 1.0f, 1e-5f);
  }
}

TEST_F(AnalyticMeshTest, CylinderCounts) {
  auto prim = std::make_shared<CylinderPrimitive>(
      QVector3D(0, 0, 0), QVector3D(0, 1, 0), 1.0f, 2.0f);
  AnalyticMesh mesh(prim, SEG);

  EXPECT_EQ(mesh.name(), "Cylinder");
  EXPECT_TRUE(mesh.hasNormals());
  EXPECT_TRUE(mesh.hasTexCoords());

  size_t sideVerts = (SEG + 1) * 2;
  size_t capVerts = (SEG + 2);
  EXPECT_EQ(mesh.getVertexCount(), sideVerts + 2 * capVerts);
  EXPECT_EQ(mesh.getFaceCount(), SEG * 4u);
  EXPECT_GT(mesh.getEdgeCount(), 0u);
}

TEST_F(AnalyticMeshTest, CylinderDataConsistency) {
  auto prim = std::make_shared<CylinderPrimitive>(
      QVector3D(0, 0, 0), QVector3D(0, 1, 0), 1.0f, 2.0f);
  AnalyticMesh mesh(prim, SEG);
  auto data = mesh.extractMeshData();

  EXPECT_TRUE(data.hasData);
  EXPECT_TRUE(data.hasNormals);
  EXPECT_TRUE(data.hasUvs);
  EXPECT_EQ(data.wireframeVertices.size(), mesh.getVertexCount() * 3);
  EXPECT_EQ(data.wireframeIndices.size(), mesh.getEdgeCount() * 2);
  EXPECT_EQ(data.solidVertices.size(), mesh.getVertexCount() * 3);
  EXPECT_EQ(data.solidIndices.size(), mesh.getFaceCount() * 3);
}

TEST_F(AnalyticMeshTest, ConeCounts) {
  auto prim = std::make_shared<ConePrimitive>(QVector3D(0, 0, 0),
                                              QVector3D(0, 1, 0), 1.0f, 2.0f);
  AnalyticMesh mesh(prim, SEG);

  EXPECT_EQ(mesh.name(), "Cone");
  EXPECT_TRUE(mesh.hasNormals());
  EXPECT_TRUE(mesh.hasTexCoords());

  size_t baseVerts = SEG + 2;
  size_t sideVerts = SEG + 1;
  EXPECT_EQ(mesh.getVertexCount(), baseVerts + 1 + sideVerts);
  EXPECT_EQ(mesh.getFaceCount(), SEG * 2u);
  EXPECT_GT(mesh.getEdgeCount(), 0u);
}

TEST_F(AnalyticMeshTest, ConeDataConsistency) {
  auto prim = std::make_shared<ConePrimitive>(QVector3D(0, 0, 0),
                                              QVector3D(0, 1, 0), 1.0f, 2.0f);
  AnalyticMesh mesh(prim, SEG);
  auto data = mesh.extractMeshData();

  EXPECT_TRUE(data.hasData);
  EXPECT_TRUE(data.hasNormals);
  EXPECT_TRUE(data.hasUvs);
  EXPECT_EQ(data.wireframeVertices.size(), mesh.getVertexCount() * 3);
  EXPECT_EQ(data.wireframeIndices.size(), mesh.getEdgeCount() * 2);
  EXPECT_EQ(data.solidVertices.size(), mesh.getVertexCount() * 3);
  EXPECT_EQ(data.solidIndices.size(), mesh.getFaceCount() * 3);
}

TEST_F(AnalyticMeshTest, CubeCounts) {
  auto prim = std::make_shared<CubePrimitive>(QVector3D(0, 0, 0), 2.0f);
  AnalyticMesh mesh(prim, SEG);

  EXPECT_EQ(mesh.name(), "Cube");
  EXPECT_TRUE(mesh.hasNormals());
  EXPECT_FALSE(mesh.hasTexCoords());

  EXPECT_EQ(mesh.getVertexCount(), 24u);
  EXPECT_EQ(mesh.getFaceCount(), 12u);
  EXPECT_EQ(mesh.getEdgeCount(), 30u);
  EXPECT_EQ(mesh.getNormalCount(), 24u);
  EXPECT_TRUE(mesh.getTexCoords().empty());
}

TEST_F(AnalyticMeshTest, CubeDataConsistency) {
  auto prim = std::make_shared<CubePrimitive>(QVector3D(0, 0, 0), 2.0f);
  AnalyticMesh mesh(prim, SEG);
  auto data = mesh.extractMeshData();

  EXPECT_TRUE(data.hasData);
  EXPECT_TRUE(data.hasNormals);
  EXPECT_FALSE(data.hasUvs);
  EXPECT_EQ(data.wireframeVertices.size(), 24u * 3);
  EXPECT_EQ(data.wireframeIndices.size(), 30u * 2);
  EXPECT_EQ(data.solidVertices.size(), 24u * 3);
  EXPECT_EQ(data.solidIndices.size(), 12u * 3);
  EXPECT_EQ(data.normals.size(), 24u * 3);
  EXPECT_TRUE(data.uvs.empty());
}

TEST_F(AnalyticMeshTest, PyramidCounts) {
  auto prim =
      std::make_shared<PyramidPrimitive>(QVector3D(0, 0, 0), 2.0f, 1.5f);
  AnalyticMesh mesh(prim, SEG);

  EXPECT_EQ(mesh.name(), "Pyramid");
  EXPECT_TRUE(mesh.hasNormals());
  EXPECT_FALSE(mesh.hasTexCoords());

  // 4 основание + 1 вершина + 4 боковые грани по 3 вершины (с дублями)
  EXPECT_EQ(mesh.getVertexCount(), 17u);
  EXPECT_EQ(mesh.getFaceCount(), 6u);
  // 5 рёбер основания + 12 рёбер боковых граней (все уникальны)
  EXPECT_EQ(mesh.getEdgeCount(), 17u);
  EXPECT_EQ(mesh.getNormalCount(), 12u);
  EXPECT_TRUE(mesh.getTexCoords().empty());
}

TEST_F(AnalyticMeshTest, PyramidDataConsistency) {
  auto prim =
      std::make_shared<PyramidPrimitive>(QVector3D(0, 0, 0), 2.0f, 1.5f);
  AnalyticMesh mesh(prim, SEG);
  auto data = mesh.extractMeshData();

  EXPECT_TRUE(data.hasData);
  EXPECT_TRUE(data.hasNormals);
  EXPECT_FALSE(data.hasUvs);
  EXPECT_EQ(data.wireframeVertices.size(), 17u * 3);
  EXPECT_EQ(data.wireframeIndices.size(), 17u * 2);
  EXPECT_EQ(data.solidVertices.size(), 17u * 3);
  EXPECT_EQ(data.solidIndices.size(), 6u * 3);
  EXPECT_EQ(data.normals.size(), 12u * 3);
  EXPECT_TRUE(data.uvs.empty());
}

TEST_F(AnalyticMeshTest, WireframeIndicesInRange) {
  auto prim = std::make_shared<SpherePrimitive>(QVector3D(0, 0, 0), 1.0f);
  AnalyticMesh mesh(prim, SEG);
  auto data = mesh.extractMeshData();

  for (size_t i = 0; i < data.wireframeIndices.size(); ++i) {
    EXPECT_LT(data.wireframeIndices[i], mesh.getVertexCount())
        << "wireframe index out of range at position " << i;
  }
}

TEST_F(AnalyticMeshTest, SolidIndicesInRange) {
  auto prim = std::make_shared<CubePrimitive>(QVector3D(0, 0, 0), 2.0f);
  AnalyticMesh mesh(prim, SEG);
  auto data = mesh.extractMeshData();

  for (size_t i = 0; i < data.solidIndices.size(); ++i) {
    EXPECT_LT(data.solidIndices[i], mesh.getVertexCount())
        << "solid index out of range at position " << i;
  }
}

TEST_F(AnalyticMeshTest, AllPrimitiveTypesExtractMeshData) {
  std::vector<std::shared_ptr<AnalyticPrimitive>> primitives = {
      std::make_shared<SpherePrimitive>(QVector3D(0, 0, 0), 1.0f),
      std::make_shared<CylinderPrimitive>(QVector3D(0, 0, 0),
                                          QVector3D(0, 1, 0), 1.0f, 2.0f),
      std::make_shared<ConePrimitive>(QVector3D(0, 0, 0), QVector3D(0, 1, 0),
                                      1.0f, 2.0f),
      std::make_shared<CubePrimitive>(QVector3D(0, 0, 0), 2.0f),
      std::make_shared<PyramidPrimitive>(QVector3D(0, 0, 0), 2.0f, 1.5f)};

  for (auto& prim : primitives) {
    AnalyticMesh mesh(prim, SEG);
    EXPECT_NO_THROW(mesh.extractMeshData());
    auto data = mesh.extractMeshData();
    EXPECT_TRUE(data.hasData);
    EXPECT_FALSE(data.wireframeVertices.empty());
    EXPECT_FALSE(data.solidVertices.empty());
  }
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}