#pragma once

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace s21 {
static constexpr double kEpsilon = 1e-7;

class Matrix {
 private:
  int rows_, cols_;
  double** matrix_;

  // Вспомогательные методы
  void AllocateMemory();
  void FreeMemory();
  void CopyMatrix(const Matrix& other);
  Matrix GetMinor(int skip_row, int skip_col) const;
  bool CheckSize(const Matrix& other) const noexcept;

 public:
  // Конструкторы и деструктор
  Matrix();
  explicit Matrix(int size);
  Matrix(int rows, int cols);
  Matrix(const Matrix& other);
  Matrix(Matrix&& other) noexcept;
  ~Matrix();

  // Accessors и Mutators
  int GetRows() const;
  int GetCols() const;
  void SetRows(int rows);
  void SetCols(int cols);

  // Основные операции
  bool EqMatrix(const Matrix& other) const noexcept;
  void SumMatrix(const Matrix& other);
  void SubMatrix(const Matrix& other);
  void MulNumber(double num);
  void MulMatrix(const Matrix& other);

  Matrix Transpose() const;
  Matrix CalcComplements() const;
  double Determinant() const;
  Matrix InverseMatrix() const;

  // Перегрузка операторов
  Matrix operator+(const Matrix& other) const;
  Matrix operator-(const Matrix& other) const;
  Matrix operator*(const Matrix& other) const;
  Matrix operator*(double num) const;
  friend Matrix operator*(double num, const Matrix& matrix);

  bool operator==(const Matrix& other) const noexcept;
  Matrix& operator=(const Matrix& other);
  Matrix& operator=(Matrix&& other) noexcept;

  Matrix& operator+=(const Matrix& other);
  Matrix& operator-=(const Matrix& other);
  Matrix& operator*=(const Matrix& other);
  Matrix& operator*=(double num);

  double& operator()(int row, int col);
  const double& operator()(int row, int col) const;
};

}  // namespace s21