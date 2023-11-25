#include "matrix.h"
#include <cublas_v2.h>

struct GPUBuffer {
    int dim;
    double *values;
    bool inuse;
    GPUBuffer *next;
};

GPUBuffer *buffers = NULL;

GPUBuffer *getBuffer(const int dim, const double *values) {
    GPUBuffer **buff = &buffers;
    while (*buff) {
        GPUBuffer *buffer = *buff;
        if (!buffer->inuse && buffer->dim == dim) {
            buffer->inuse = true;
            cudaMemcpy(buffer->values, values, sizeof(double) * dim,
                       cudaMemcpyHostToDevice);
            return buffer;
        }
        buff = &(buffer->next);
    }
    GPUBuffer *nbuff = (GPUBuffer *)malloc(sizeof(GPUBuffer));
    nbuff->dim = dim;
    nbuff->inuse = true;
    cudaMalloc(&nbuff->values, sizeof(double) * dim);
    cudaMemcpy(nbuff->values, values, sizeof(double) * dim,
               cudaMemcpyHostToDevice);
    nbuff->next = NULL;
    *buff = nbuff;
    return nbuff;
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
    GPUBuffer *m1buffer = NULL, *m2buffer = NULL, *outbuffer = NULL;

    double *newout = out;

    if(!v1OnGpu) {
        m1buffer = getBuffer(row1 * col1, v1);
        v1 = m1buffer->values;
    }

    if(!v2OnGpu) {
        m2buffer = getBuffer(col1 * col2, v2);
        v2 = m2buffer->values;
    }

    if(!outOnGpu) {
        outbuffer = getBuffer(row1 * col2, out);
        newout = outbuffer->values;
    }

    dim3 dimBlock(16, 64);
    dim3 dimGrid((col2 + dimBlock.x - 1) / dimBlock.x,
                 (row1 + dimBlock.y - 1) / dimBlock.y);

    // cudaMultiply<<<dimGrid, dimBlock>>>(row1, col1, col2, v1,
    //                        v2, newout);

    cublasHandle_t handle;
    cublasCreate(&handle);

    const double alpha = 1.0f, beta = 0.0f;
    cublasDgemm(handle, CUBLAS_OP_N, CUBLAS_OP_N, col1, row1, col2, &alpha, v2, col1, v1, col2, &beta, newout, col1);

    cublasDestroy(handle);

    if(!outOnGpu) {
        cudaMemcpy(out, newout, sizeof(double) * row1 * col2,
                   cudaMemcpyDeviceToHost);
    }

    if(m1buffer)
        m1buffer->inuse = false;
    if(m2buffer)
        m2buffer->inuse = false;
    if(outbuffer)
        outbuffer->inuse = false;
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
    GPUBuffer *buffer = NULL;
    double *newmat = mat;

    if(!onGpu) {
        buffer = getBuffer(row1 * col1, mat);
        newmat = buffer->values;
    }

    int threadsPerBlock = 512;
    int numBlocks = (row1 * col1 + threadsPerBlock - 1) / threadsPerBlock;

    cudaNormalize<<<numBlocks, threadsPerBlock>>>(newmat, row1 * col1 / 4);
    cudaCutOff<<<numBlocks, threadsPerBlock>>>(newmat, row1 * col1);

    if(!onGpu) {
        cudaMemcpy(mat, newmat, sizeof(double) * row1 * col1,
               cudaMemcpyDeviceToHost);

        buffer->inuse = false;
    }
}

void* GPU::malloc(size_t size) {
    void *ret;
    cudaMalloc(&ret, size);
    return ret;
}

void GPU::free(void *mem) {
    cudaFree(mem);
}

void GPU::memcpy(void *dst, void *src, size_t siz, bool reverse) {
    if(reverse) {
        cudaMemcpy(dst, src, siz,
                cudaMemcpyDeviceToHost);
    } else {
        cudaMemcpy(dst, src, siz,
                cudaMemcpyHostToDevice);
    }
}

void* GPU::realloc(void *ptr, size_t os, size_t ns) {
    void *newPtr;
    cudaMalloc(&newPtr, ns);
    size_t lt = ns < os ? ns : os;
    cudaMemcpy(newPtr, ptr, lt, cudaMemcpyDeviceToDevice);
    cudaFree(ptr);
    return newPtr;
}
