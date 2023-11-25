#include <cublas_v2.h>
#include <cuda.h>
#include <cuda_runtime.h>

#include "matrix.h"

GPU::Buffer *buffers = NULL;

cublasHandle_t BLAShandle = NULL;

void GPU::init() {
    cublasCreate(&BLAShandle);
}

GPU::Buffer* GPU::Buffer::alloc(size_t size) {
    GPU::Buffer **buff = &buffers;
    while (*buff) {
        GPU::Buffer *buffer = *buff;
        if (!buffer->inuse && buffer->dim == size) {
            buffer->inuse = true;
            return buffer;
        }
        buff = &(buffer->next);
    }

    GPU::Buffer *nbuff = (GPU::Buffer *)std::malloc(sizeof(GPU::Buffer));
    nbuff->dim = size;
    nbuff->inuse = true;
    cudaMalloc((void**)&nbuff->values, sizeof(double) * size);
    nbuff->next = NULL;
    *buff = nbuff;
    return nbuff;
}

void GPU::Buffer::free() {
    inuse = false;
}

GPU::Buffer *getBuffer(const size_t dim, const double *values) {
    GPU::Buffer* buff = GPU::Buffer::alloc(dim);
    cudaMemcpy(buff->values, values, sizeof(double) * dim,
               cudaMemcpyHostToDevice);
    return buff;
}

__global__ void cudaMultiply(const int rowsA, const int colsA, const int colsB,
                             const double *a, const double *b, double *c) {
    int row = blockIdx.y * blockDim.y + threadIdx.y;
    int col = blockIdx.x * blockDim.x + threadIdx.x;

    if (row < rowsA && col < colsB) {
        double value = 0;
        for (int k = 0; k < colsA; ++k) {
            value += a[row * colsA + k] * b[k * colsB + col];
        }
        c[row * colsB + col] = value;
    }
}

void GPU::multiply(const int row1, const int col1, const int col2,
                   const double *v1, const double *v2, double *out,
                   bool v1OnGpu, bool v2OnGpu, bool outOnGpu) {
    GPU::Buffer *m1buffer = NULL, *m2buffer = NULL, *outbuffer = NULL;

    double *newout = out;

    if (!v1OnGpu) {
        m1buffer = getBuffer(row1 * col1, v1);
        v1 = m1buffer->values;
    }

    if (!v2OnGpu) {
        m2buffer = getBuffer(col1 * col2, v2);
        v2 = m2buffer->values;
    }

    if (!outOnGpu) {
        outbuffer = getBuffer(row1 * col2, out);
        newout = outbuffer->values;
    }

    dim3 dimBlock(16, 64);
    dim3 dimGrid((col2 + dimBlock.x - 1) / dimBlock.x,
                 (row1 + dimBlock.y - 1) / dimBlock.y);

    // cudaMultiply<<<dimGrid, dimBlock>>>(row1, col1, col2, v1,
    //                        v2, newout);

    const double alpha = 1.0f, beta = 0.0f;
    cublasDgemm(BLAShandle, CUBLAS_OP_N, CUBLAS_OP_N, col1, row1, col2, &alpha, v2,
                    col1, v1, col2, &beta, newout, col1);

    if (!outOnGpu) {
        cudaMemcpy(out, newout, sizeof(double) * row1 * col2,
                   cudaMemcpyDeviceToHost);
    }

    if (m1buffer) m1buffer->inuse = false;
    if (m2buffer) m2buffer->inuse = false;
    if (outbuffer) outbuffer->inuse = false;
}

__global__ void cudaNormalize(double *values, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    if (idx < n) {
        int baseidx = idx * 4;
        values[baseidx] /= values[baseidx + 3];
        values[baseidx + 1] /= values[baseidx + 3];
        values[baseidx + 2] /= values[baseidx + 3];
        values[baseidx + 3] = 1;
    }
}

__global__ void cudaCutOff(double *values, int n) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    if (idx < n) {
        if (values[idx] > 1 || values[idx] < -1) {
            values[idx] = 0;
        }
    }
}

void GPU::normalizeAndCutOff(int row1, int col1, double *mat, bool onGpu) {
    GPU::Buffer *buffer = NULL;
    double *newmat = mat;

    if (!onGpu) {
        buffer = getBuffer(row1 * col1, mat);
        newmat = buffer->values;
    }

    int threadsPerBlock = 512;
    int numBlocks = (row1 * col1 + threadsPerBlock - 1) / threadsPerBlock;

    cudaNormalize<<<numBlocks, threadsPerBlock>>>(newmat, row1 * col1 / 4);
    cudaCutOff<<<numBlocks, threadsPerBlock>>>(newmat, row1 * col1);

    if (!onGpu) {
        cudaMemcpy(mat, newmat, sizeof(double) * row1 * col1,
                   cudaMemcpyDeviceToHost);

        buffer->inuse = false;
    }
}

void GPU::multiply_add(double *b, const double *a, double x, int size) {

    // Perform the operation b[i] += a[i] * x using cuBLAS
    cublasDaxpy(BLAShandle, size, &x, a, 1, b, 1);
}

void *GPU::malloc(size_t size) {
    void *ret;
    cudaMalloc(&ret, size);
    return ret;
}

void GPU::free(void *mem) { cudaFree(mem); }

void GPU::memcpy(void *dst, void *src, size_t siz, bool reverse) {
    if (reverse) {
        cudaMemcpy(dst, src, siz, cudaMemcpyDeviceToHost);
    } else {
        cudaMemcpy(dst, src, siz, cudaMemcpyHostToDevice);
    }
}

void *GPU::realloc(void *ptr, size_t os, size_t ns) {
    void *newPtr;
    cudaMalloc(&newPtr, ns);
    size_t lt = ns < os ? ns : os;
    cudaMemcpy(newPtr, ptr, lt, cudaMemcpyDeviceToDevice);
    cudaFree(ptr);
    return newPtr;
}
