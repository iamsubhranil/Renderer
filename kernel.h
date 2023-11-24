#pragma once

struct GPU {
    static void *malloc(size_t size);
    static void *realloc(void *ptr, size_t os, size_t ns);
    static void memcpy(void *dst, void *src, size_t size, bool reverse = false);
    static void free(void *mem);

    static void multiply(const int row1, const int col1, const int col2,
                         const double *v1, const double *v2, double *out,
                         bool leftOnGpu, bool rightOnGpu, bool resultOnGpu);

    static void normalizeAndCutOff(int row, int col, double *mat, bool onGpu);
};
