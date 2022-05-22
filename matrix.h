#pragma once

#include <math.h>

#include <cstdio>

template <int M, int N>
struct ConstantMatrix;

struct Point3D {
    double x, y, z;
    Point3D() : x(0), y(0), z(0) {}
    Point3D(double a, double b, double c) {
        x = a;
        y = b;
        z = c;
    }
    Point3D(const Point3D &other) : x(other.x), y(other.y), z(other.z) {}
};

struct Matrix {
    int row, col;
    double *values;

    Matrix(int r, int c) {
        row = r;
        col = c;
        values = (double *)malloc(sizeof(double) * (row * col));
    }

    Matrix() {
        row = col = 0;
        values = NULL;
    }

    template <typename... T>
    Matrix(int r, int c, const T &...newvals) : Matrix(r, c) {
        assign(values, 0, newvals...);
    }

    template <typename... T>
    constexpr static void assign(double *values, int index, const double &val,
                                 const T &...next) {
        values[index] = val;
        assign(values, index + 1, next...);
    }

    constexpr static void assign(double *values, int index) {
        (void)values;
        (void)index;
    }

    template <typename... T>
    void appendRow(const T &...val) {
        values = (double *)realloc(values, sizeof(double) * (row + 1) * col);
        assign(values, row * col, val...);
        row++;
    }

    static constexpr void multiply(const int row1, const int col1,
                                   const int col2, const double *left_values,
                                   const double *right_values, double *result) {
        int left_pointer = 0;
        int result_pointer = 0;
        for (int i = 0; i < row1; ++i) {
            for (int j = 0; j < col2; ++j) {
                int right_pointer = 0;
                result[result_pointer + j] = 0;
                for (int k = 0; k < col1; ++k) {
                    result[result_pointer + j] +=
                        left_values[left_pointer + k] *
                        right_values[right_pointer + j];
                    right_pointer += col2;
                }
            }
            left_pointer += col1;
            result_pointer += col2;
        }
    }

    double &at(int i) const { return values[i]; }
    double &at(int i, int j) const { return values[i * col + j]; }

    Matrix operator*(const Matrix &newmat) {
        Matrix res(row, newmat.col);
        multiply(row, col, newmat.col, values, newmat.values, res.values);
        return res;
    }

    void normalizeAndCutoffAt(int i, int offset) {
        values[i + offset] /= values[i + 3];
        if (values[i + offset] > 1 || values[i + offset] < -1) {
            values[i + offset] = 0;
        }
    }

    void normalizeAndCutoff() {
        for (int i = 0; i < row * col; i += col) {
            normalizeAndCutoffAt(i, 0);
            normalizeAndCutoffAt(i, 1);
            normalizeAndCutoffAt(i, 2);
            values[i + 3] = 1;
        }
    }

    static bool print_values(double *values, int row, int col) {
        for (int i = 0; i < row; i++) {
            for (int j = 0; j < col; j++) {
                printf("%g ", values[i * col + j]);
            }
            printf("\n");
        }
        printf("\n");
        return true;
    }

    bool print() { return print_values(values, row, col); }

    void destroy() { free(values); }

    // ~Matrix() { free(values); }
};

template <int M, int N>
struct ConstantMatrix {
    const int row = M, col = N;
    double values[M * N];

    template <typename... T>
    constexpr ConstantMatrix(const T &...newval) {
        Matrix::assign(values, 0, newval...);
    }

    ConstantMatrix(const ConstantMatrix<M, N> &other) {
        for (int i = 0; i < M * N; i++) {
            values[i] = other.values[i];
        }
    }

    ConstantMatrix<M, N> &operator=(const ConstantMatrix<M, N> &other) {
        for (int i = 0; i < M * N; i++) {
            values[i] = other.values[i];
        }
        return *this;
    }

    template <typename... T>
    void fill(const T &...newval) {
        Matrix::assign(values, 0, newval...);
    }

    template <int O>
    ConstantMatrix<M, O> operator*(const ConstantMatrix<N, O> &newmat) {
        ConstantMatrix<M, O> result;
        Matrix::multiply(M, N, O, values, newmat.values, result.values);
        return result;
    }

    Matrix operator*(const Matrix &newmat) {
        Matrix res(M, newmat.col);
        Matrix::multiply(M, N, newmat.col, values, newmat.values, res.values);
        return res;
    }

    void multiply_add(const ConstantMatrix<M, N> &other, double val) {
        for (int i = 0; i < row * col; i++) {
            values[i] += other.values[i] * val;
        }
    }

    void multiply_sub(const ConstantMatrix<M, N> &other, double val) {
        for (int i = 0; i < row * col; i++) {
            values[i] -= other.values[i] * val;
        }
    }

    const double &at(int i) const { return values[i]; }
    double &at(int i, int j) { return values[i * col + j]; }

    bool print() { return Matrix::print_values(values, row, col); }
};

template <int M, int N>
Matrix operator*(const Matrix &mat1, const ConstantMatrix<M, N> &mat2) {
    Matrix res(mat1.row, mat2.col);
    Matrix::multiply(mat1.row, mat1.col, mat2.col, mat1.values, mat2.values,
                     res.values);
    return res;
}

struct ProjectionMatrix : public Matrix {
    double *swap_buffer;

    ProjectionMatrix() { swap_buffer = NULL; }

    template <typename... T>
    ProjectionMatrix(int r, int c, const T &...newvals)
        : Matrix(r, c, newvals...) {
        swap_buffer = (double *)malloc(sizeof(double) * (row * col));
    }

    ProjectionMatrix(int r, int c) : Matrix(r, c) {
        swap_buffer = (double *)malloc(sizeof(double) * (row * col));
    }

    void multiply_and_assign(const Matrix &mat1,
                             const ConstantMatrix<4, 4> &mat2) {
        Matrix::multiply(mat1.row, mat1.col, mat2.col, mat1.values, mat2.values,
                         values);
    }

    void multiply(const ConstantMatrix<4, 4> &mat2) {
        Matrix::multiply(row, col, mat2.col, values, mat2.values, swap_buffer);
        double *bak = values;
        values = swap_buffer;
        swap_buffer = bak;
    }

    void finalize_dimension() {
        swap_buffer = (double *)malloc(sizeof(double) * (row * col));
    }

    void destroy() {
        free(values);
        free(swap_buffer);
    }

    ProjectionMatrix &operator*(const ConstantMatrix<4, 4> &mat2) {
        multiply(mat2);
        return *this;
    }
};

struct Transform {
    static ConstantMatrix<4, 4> translate(Point3D to) {
        return ConstantMatrix<4, 4>(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, to.x,
                                    to.y, to.z, 1);
    }

    static ConstantMatrix<4, 4> rotate_x(const double angle) {
        return ConstantMatrix<4, 4>(1, 0, 0, 0, 0, cos(angle), sin(angle), 0, 0,
                                    -sin(angle), cos(angle), 0, 0, 0, 0, 1);
    }

    static ConstantMatrix<4, 4> rotate_y(const double angle) {
        return ConstantMatrix<4, 4>(cos(angle), 0, -sin(angle), 0, 0, 1, 0, 0,
                                    sin(angle), 0, cos(angle), 0, 0, 0, 0, 1);
    }

    static ConstantMatrix<4, 4> rotate_z(const double angle) {
        return ConstantMatrix<4, 4>(cos(angle), sin(angle), 0, 0, -sin(angle),
                                    cos(angle), 0, 0, 0, 1, 0, 0, 0, 0, 0, 1);
    }

    static ConstantMatrix<4, 4> scale(const double zoom) {
        return ConstantMatrix<4, 4>(zoom, 0, 0, 0, 0, zoom, 0, 0, 0, 0, 0, zoom,
                                    0, 0, 0, 1);
    }
};
