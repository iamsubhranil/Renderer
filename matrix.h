#pragma once

#include <SDL2/SDL_timer.h>
#include <math.h>

#include <cstdio>

#include "kernel.h"

#ifdef DEBUG
#define MEASURE(x)                                      \
    do {                                                \
        Uint64 start_step = SDL_GetTicks64();           \
        x;                                              \
        Uint64 end_step = SDL_GetTicks64();             \
        printf(#x ": %3ldms\n", end_step - start_step); \
    } while (0)

#define MEASURE_START(x) Uint64 __measure_start_##x = SDL_GetTicks64();
#define MEASURE_END(x)                           \
    Uint64 __measure_end_##x = SDL_GetTicks64(); \
    printf(#x ": %3ldms\n", __measure_end_##x - __measure_start_##x);
#else
#define MEASURE(x) x;
#define MEASURE_START(x)
#define MEASURE_END(x)
#endif

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
    int row, col, allocated_rows;
    double *values;
    double *cpu_values;

#ifdef DEBUG
    double *print_values_cpy;
#endif

    Matrix(int r, int c) {
        row = r;
        col = c;
        allocated_rows = r;
        values = (double *)GPU::malloc(sizeof(double) * (row * col));
        cpu_values = NULL;

#ifdef DEBUG
        print_values_cpy = NULL;
#endif
    }

    void moveToCpu() {
        cpu_values = (double *)malloc(sizeof(double) * (row * col));
        GPU::memcpy(cpu_values, values, sizeof(double) * (row * col), true);
    }

    double at(int i, int j) { return cpu_values[i * col + j]; }

    Matrix() {
        row = col = allocated_rows = 0;
        values = NULL;
        cpu_values = NULL;
#ifdef DEBUG
        print_values_cpy = NULL;
#endif
    }

    template <typename... T>
    Matrix(int r, int c, const T &...newvals) : Matrix(r, c) {
        double tempValues[r * c];
        assign(tempValues, 0, newvals...);
        GPU::memcpy(values, tempValues, sizeof(double) * r * c);
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
        double newValues[col];
        assign(newValues, 0, val...);
        if (row == allocated_rows) {
            if (allocated_rows == 0)
                allocated_rows = 2;
            else
                allocated_rows *= 2;
            values =
                (double *)GPU::realloc(values, sizeof(double) * row * col,
                                       sizeof(double) * allocated_rows * col);
        }
        GPU::memcpy(&values[row * col], newValues, sizeof(double) * col);
        row++;
    }

    static inline void multiply(const int row1, const int col1, const int col2,
                                const double *left_values,
                                const double *right_values, double *result,
                                bool leftOnGpu, bool rightOnGpu,
                                bool resultOnGpu) {
        GPU::multiply(row1, col1, col2, left_values, right_values, result,
                      leftOnGpu, rightOnGpu, resultOnGpu);
    }

    void finalize_dimension() {
        if (allocated_rows > row) {
            values = (double *)GPU::realloc(
                values, sizeof(double) * allocated_rows * col,
                sizeof(double) * row * col);
            allocated_rows = row;
        }
    }

    Matrix operator*(const Matrix &newmat) {
        Matrix res(row, newmat.col);
        multiply(row, col, newmat.col, values, newmat.values, res.values, true,
                 true, true);
        return res;
    }

    void normalizeAndCutoff() {
        GPU::normalizeAndCutOff(row, col, values, true);
    }

#ifdef DEBUG

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

    bool print() {
        if (!print_values_cpy) {
            print_values_cpy = (double *)malloc(sizeof(double) * row * col);
        }
        GPU::memcpy(print_values_cpy, values, sizeof(double) * row * col, true);
        return Matrix::print_values(print_values_cpy, row, col);
    }

#endif

    void destroy() { GPU::free(values); }

    // ~Matrix() { free(values); }
};

template <int M, int N>
struct ConstantMatrix {
   private:
    double *gpu_values, *cpu_values;
    GPU::Buffer *buffer;
    bool dirty;

    void copyToCPU() {
        if (dirty) {
            if (!cpu_values) {
                cpu_values = (double *)malloc(sizeof(double) * M * N);
            }
            GPU::memcpy(cpu_values, gpu_values, sizeof(double) * M * N, true);
        }
    }

   public:
    ConstantMatrix() {
        buffer = GPU::Buffer::alloc(M * N);
        gpu_values = buffer->values;
        cpu_values = NULL;
        dirty = true;
    }

    template <typename... T>
    ConstantMatrix(const T &...newval) : ConstantMatrix() {
        fill(newval...);
    }

    ConstantMatrix(const ConstantMatrix<M, N> &other) : ConstantMatrix() {
        GPU::memcpy(gpu_values, other.gpu_values, sizeof(double) * M * N);
    }

    ConstantMatrix<M, N> &operator=(const ConstantMatrix<M, N> &other) {
        GPU::memcpy(gpu_values, other.gpu_values, sizeof(double) * M * N);
        dirty = true;
        return *this;
    }

    template <typename... T>
    void fill(const T &...newval) {
        double values[M * N];
        Matrix::assign(values, 0, newval...);
        GPU::memcpy(gpu_values, values, sizeof(double) * M * N);
        dirty = true;
    }

    template <int O>
    ConstantMatrix<M, O> operator*(const ConstantMatrix<N, O> &newmat) {
        ConstantMatrix<M, O> result;

        /*
        int left_pointer = 0;
        int result_pointer = 0;
        for (int i = 0; i < M; ++i) {
            for (int j = 0; j < O; ++j) {
                int right_pointer = 0;
                result.values[result_pointer + j] = 0;
                for (int k = 0; k < N; ++k) {
                    result.values[result_pointer + j] +=
                        values[left_pointer + k] *
                        newmat.values[right_pointer + j];
                    right_pointer += O;
                }
            }
            left_pointer += N;
            result_pointer += O;
        }
        */

        Matrix::multiply(M, N, O, gpu_values, newmat.getGPUValues(),
                         result.getGPUValues(), true, true, true);
        return result;
    }

    Matrix operator*(const Matrix &newmat) {
        Matrix res(M, newmat.col);
        Matrix::multiply(M, N, newmat.col, gpu_values, newmat.values,
                         res.values, true, true, true);
        return res;
    }

    inline void multiply_add(const ConstantMatrix<M, N> &other, double val) {
        GPU::multiply_add(gpu_values, other.gpu_values, val, M * N);
        dirty = true;
    }

    inline void multiply_sub(const ConstantMatrix<M, N> &other, double val) {
        GPU::multiply_add(gpu_values, other.gpu_values, -val, M * N);
        dirty = true;
    }

    inline double *getGPUValues() const { return gpu_values; }

    const double &at(int i) {
        copyToCPU();
        return cpu_values[i];
    }
    double &at(int i, int j) { return at(i * N + j); }

#ifdef DEBUG
    bool print() {
        copyToCPU();
        return Matrix::print_values(cpu_values, M, N);
    }
#endif

    ~ConstantMatrix() { buffer->free(); }
};

template <int M, int N>
Matrix operator*(const Matrix &mat1, const ConstantMatrix<M, N> &mat2) {
    Matrix res(mat1.row, mat2.col);
    Matrix::multiply(mat1.row, mat1.col, N, mat1.values, mat2.getGPUValues(),
                     res.values, true, true, true);
    return res;
}

struct ProjectionMatrix : public Matrix {
    double *swap_buffer;
    double *screen_buffer;
    bool dirty;

    ProjectionMatrix() {
        swap_buffer = NULL;
        screen_buffer = NULL;
        dirty = true;
    }

    template <typename... T>
    ProjectionMatrix(int r, int c, const T &...newvals)
        : Matrix(r, c, newvals...) {
        swap_buffer = (double *)GPU::malloc(sizeof(double) * (row * col));
        screen_buffer = (double *)malloc(sizeof(double) * row * col);
        dirty = true;
    }

    ProjectionMatrix(int r, int c) : Matrix(r, c) {
        swap_buffer = (double *)GPU::malloc(sizeof(double) * (row * col));
        screen_buffer = (double *)malloc(sizeof(double) * row * col);
        dirty = true;
    }

    void multiply_and_assign(const Matrix &mat1,
                             const ConstantMatrix<4, 4> &mat2) {
        Matrix::multiply(mat1.row, mat1.col, 4, mat1.values,
                         mat2.getGPUValues(), values, true, true, true);
        dirty = true;
    }

    void multiply(const ConstantMatrix<4, 4> &mat2) {
        Matrix::multiply(row, col, 4, values, mat2.getGPUValues(), swap_buffer,
                         true, true, true);
        double *bak = values;
        values = swap_buffer;
        swap_buffer = bak;
        dirty = true;
    }

    void finalize_dimension() {
        Matrix::finalize_dimension();
        swap_buffer = (double *)GPU::malloc(sizeof(double) * (row * col));
        screen_buffer = (double *)malloc(sizeof(double) * row * col);
        dirty = true;
    }

    double at(int i, int j) {
        if (dirty) {
            GPU::memcpy(screen_buffer, values, sizeof(double) * (row * col),
                        true);
            dirty = false;
        }
        return screen_buffer[i * col + j];
    }

    void destroy() {
        GPU::free(values);
        GPU::free(swap_buffer);
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
