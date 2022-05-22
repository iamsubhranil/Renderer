#pragma once

#include "matrix.h"

struct Renderer;

struct Projection {
    double near, far, left, right, top, bottom;
    ConstantMatrix<4, 4> projection_matrix;
    ConstantMatrix<4, 4> to_screen_matrix;

    Projection() {}
    void init(Renderer *renderer);
};
