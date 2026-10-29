#include "s21_matrix_oop.h"

namespace s21 {

// Вспомогательные методы
void Matrix::AllocateMemory() {
  if (rows_ <= 0 || cols_ <= 0) {
    throw std::invalid_argument("Matrix dimensions must be positive");
  }

  matrix_ = new double*[rows_];
  for (int i = 0; i < rows_; i++) {
    matrix_[i] = new double[cols_]();
  }
}

void Matrix::FreeMemory() {
  if (matrix_) {
    for (int i = 0; i < rows_; i++) {
      delete[] matrix_[i];
    }
    delete[] matrix_;
    matrix_ = nullptr;
  }
  rows_ = 0;
  cols_ = 0;
}

void Matrix::CopyMatrix(const Matrix& other) {
  for (int i = 0; i < rows_; i++) {
    for (int j = 0; j < cols_; j++) {
      matrix_[i][j] = other.matrix_[i][j];
    }
  }
}

bool Matrix::CheckSize(const Matrix& other) const noexcept {
  return rows_ == other.rows_ && cols_ == other.cols_;
}

// Конструкторы и деструктор
Matrix::Matrix() : rows_(3), cols_(3), matrix_(nullptr) { AllocateMemory(); }

Matrix::Matrix(int size) : rows_(size), cols_(size), matrix_(nullptr) {
  AllocateMemory();
}

Matrix::Matrix(int rows, int cols)
    : rows_(rows), cols_(cols), matrix_(nullptr) {
  AllocateMemory();
}

Matrix::Matrix(const Matrix& other)
    : rows_(other.rows_), cols_(other.cols_), matrix_(nullptr) {
  AllocateMemory();
  CopyMatrix(other);
}

Matrix::Matrix(Matrix&& other) noexcept
    : rows_(other.rows_), cols_(other.cols_), matrix_(other.matrix_) {
  other.rows_ = 0;
  other.cols_ = 0;
  other.matrix_ = nullptr;
}

Matrix::~Matrix() { FreeMemory(); }

// Accessors и Mutators
int Matrix::GetRows() const { return rows_; }

int Matrix::GetCols() const { return cols_; }

void Matrix::SetRows(int rows) {
  if (rows <= 0) {
    throw std::invalid_argument("Rows must be positive");
  }

  if (rows == rows_) {
    return;
  }

  Matrix temp(rows, cols_);
  int min_rows = std::min(rows, rows_);
  for (int i = 0; i < min_rows; i++) {
    for (int j = 0; j < cols_; j++) {
      temp.matrix_[i][j] = matrix_[i][j];
    }
  }
  *this = std::move(temp);
}

void Matrix::SetCols(int cols) {
  if (cols <= 0) {
    throw std::invalid_argument("Columns must be positive");
  }

  if (cols == cols_) {
    return;
  }

  Matrix temp(rows_, cols);
  int min_cols = std::min(cols, cols_);
  for (int i = 0; i < rows_; i++) {
    for (int j = 0; j < min_cols; j++) {
      temp.matrix_[i][j] = matrix_[i][j];
    }
  }
  *this = std::move(temp);
}

// Основные операции
bool Matrix::EqMatrix(const Matrix& other) const noexcept {
  if (!CheckSize(other)) {
    return false;
  }

  for (int i = 0; i < rows_; i++) {
    for (int j = 0; j < cols_; j++) {
      if (std::fabs(matrix_[i][j] - other.matrix_[i][j]) > kEpsilon) {
        return false;
      }
    }
  }
  return true;
}

void Matrix::SumMatrix(const Matrix& other) {
  if (!CheckSize(other)) {
    throw std::invalid_argument("Matrix sizes must be equal for addition");
  }

  for (int i = 0; i < rows_; i++) {
    for (int j = 0; j < cols_; j++) {
      matrix_[i][j] += other.matrix_[i][j];
      if (std::isinf(matrix_[i][j]) || std::isnan(matrix_[i][j])) {
        throw std::runtime_error("Calculation error: inf or nan");
      }
    }
  }
}

void Matrix::SubMatrix(const Matrix& other) {
  if (!CheckSize(other)) {
    throw std::invalid_argument("Matrix sizes must be equal for subtraction");
  }

  for (int i = 0; i < rows_; i++) {
    for (int j = 0; j < cols_; j++) {
      matrix_[i][j] -= other.matrix_[i][j];
      if (std::isinf(matrix_[i][j]) || std::isnan(matrix_[i][j])) {
        throw std::runtime_error("Calculation error: inf or nan");
      }
    }
  }
}

void Matrix::MulNumber(double num) {
  for (int i = 0; i < rows_; i++) {
    for (int j = 0; j < cols_; j++) {
      matrix_[i][j] *= num;
      if (std::isinf(matrix_[i][j]) || std::isnan(matrix_[i][j])) {
        throw std::runtime_error("Calculation error: inf or nan");
      }
    }
  }
}

void Matrix::MulMatrix(const Matrix& other) {
  if (cols_ != other.rows_) {
    throw std::invalid_argument(
        "Columns of first matrix must equal rows of second matrix");
  }

  Matrix result(rows_, other.cols_);

  for (int i = 0; i < rows_; i++) {
    for (int j = 0; j < other.cols_; j++) {
      result.matrix_[i][j] = 0;
      for (int k = 0; k < cols_; k++) {
        result.matrix_[i][j] += matrix_[i][k] * other.matrix_[k][j];
        if (std::isinf(result.matrix_[i][j]) ||
            std::isnan(result.matrix_[i][j])) {
          throw std::runtime_error("Calculation error: inf or nan");
        }
      }
    }
  }
  *this = std::move(result);
}

Matrix Matrix::Transpose() const {
  Matrix result(cols_, rows_);

  for (int i = 0; i < rows_; i++) {
    for (int j = 0; j < cols_; j++) {
      result.matrix_[j][i] = matrix_[i][j];
    }
  }
  return result;
}

Matrix Matrix::GetMinor(int skip_row, int skip_col) const {
  Matrix minor(rows_ - 1, cols_ - 1);
  int minor_i = 0;

  for (int i = 0; i < rows_; i++) {
    if (i == skip_row) {
      continue;
    }

    int minor_j = 0;
    for (int j = 0; j < cols_; j++) {
      if (j == skip_col) {
        continue;
      }
      minor.matrix_[minor_i][minor_j] = matrix_[i][j];
      minor_j++;
    }
    minor_i++;
  }
  return minor;
}

double Matrix::Determinant() const {
  if (rows_ != cols_) {
    throw std::invalid_argument("Matrix must be square");
  }

  if (rows_ == 1) {
    return matrix_[0][0];
  }

  if (rows_ == 2) {
    return matrix_[0][0] * matrix_[1][1] - matrix_[0][1] * matrix_[1][0];
  }

  double det = 0;
  double sign = 1;

  for (int j = 0; j < cols_; j++) {
    Matrix minor = GetMinor(0, j);
    det += sign * matrix_[0][j] * minor.Determinant();
    sign = -sign;
  }
  return det;
}

Matrix Matrix::CalcComplements() const {
  if (rows_ != cols_) {
    throw std::invalid_argument("Matrix must be square");
  }

  Matrix result(rows_, cols_);

  if (rows_ == 1) {
    result.matrix_[0][0] = 1;
    return result;
  }

  for (int i = 0; i < rows_; i++) {
    double sign = (i % 2 == 0) ? 1 : -1;
    for (int j = 0; j < cols_; j++) {
      Matrix minor = GetMinor(i, j);
      result.matrix_[i][j] = sign * minor.Determinant();
      sign = -sign;
    }
  }
  return result;
}

Matrix Matrix::InverseMatrix() const {
  double det = Determinant();
  if (std::fabs(det) < kEpsilon) {
    throw std::runtime_error(
        "Matrix determinant is zero, inverse doesn't exist");
  }

  Matrix complements = CalcComplements();
  Matrix transposed = complements.Transpose();
  transposed.MulNumber(1.0 / det);

  return transposed;
}

// Перегрузка операторов
Matrix Matrix::operator+(const Matrix& other) const {
  Matrix result(*this);
  result.SumMatrix(other);
  return result;
}

Matrix Matrix::operator-(const Matrix& other) const {
  Matrix result(*this);
  result.SubMatrix(other);
  return result;
}

Matrix Matrix::operator*(const Matrix& other) const {
  Matrix result(*this);
  result.MulMatrix(other);
  return result;
}

Matrix Matrix::operator*(double num) const {
  Matrix result(*this);
  result.MulNumber(num);
  return result;
}

Matrix operator*(double num, const Matrix& matrix) { return matrix * num; }

bool Matrix::operator==(const Matrix& other) const noexcept {
  return EqMatrix(other);
}

Matrix& Matrix::operator=(const Matrix& other) {
  if (this == &other) {
    return *this;
  }

  FreeMemory();
  rows_ = other.rows_;
  cols_ = other.cols_;
  AllocateMemory();
  CopyMatrix(other);
  return *this;
}

Matrix& Matrix::operator=(Matrix&& other) noexcept {
  if (this == &other) {
    return *this;
  }

  FreeMemory();
  rows_ = other.rows_;
  cols_ = other.cols_;
  matrix_ = other.matrix_;
  other.rows_ = 0;
  other.cols_ = 0;
  other.matrix_ = nullptr;
  return *this;
}

Matrix& Matrix::operator+=(const Matrix& other) {
  SumMatrix(other);
  return *this;
}

Matrix& Matrix::operator-=(const Matrix& other) {
  SubMatrix(other);
  return *this;
}

Matrix& Matrix::operator*=(const Matrix& other) {
  MulMatrix(other);
  return *this;
}

Matrix& Matrix::operator*=(double num) {
  MulNumber(num);
  return *this;
}

double& Matrix::operator()(int row, int col) {
  if (row < 0 || row >= rows_ || col < 0 || col >= cols_) {
    throw std::out_of_range("Index out of range");
  }
  return matrix_[row][col];
}

const double& Matrix::operator()(int row, int col) const {
  if (row < 0 || row >= rows_ || col < 0 || col >= cols_) {
    throw std::out_of_range("Index out of range");
  }
  return matrix_[row][col];
}

}  // namespace s21