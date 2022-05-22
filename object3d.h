#pragma once

#include "matrix.h"

struct Renderer;
struct SDL_Point;

struct Object3D {
    Renderer *renderer;
    ProjectionMatrix vertices;
    Matrix faces;
    ProjectionMatrix projectionMatrix;  // vertex.row * 4
    SDL_Point *plot_points;
    int *randomFaceColors;

    Object3D(Renderer *r) {
        renderer = r;
        vertices =
            ProjectionMatrix(8, 4, 0, 0, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 0, 0,
                             1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1);
        faces = Matrix(6, 4, 0, 1, 2, 3, 4, 5, 6, 7, 0, 4, 5, 1, 2, 3, 7, 6, 1,
                       2, 6, 5, 0, 3, 7, 4);

        prepare();
    }

    Object3D() {}
    void prepare();

    static Object3D loadObj(const char *file, Renderer *r);

    void draw(bool dumpMatrices) {
        screenProjection(dumpMatrices);
        movement();
    }
    void screenProjection(bool dumpMatrices);
    void movement();

    void translate(Point3D to) { vertices.multiply(Transform::translate(to)); }

    void scale(double by) { vertices.multiply(Transform::scale(by)); }

    void rotate_x(double angle) {
        vertices.multiply(Transform::rotate_x(angle));
    }

    void rotate_y(double angle) {
        vertices.multiply(Transform::rotate_y(angle));
    }

    void rotate_z(double angle) {
        vertices.multiply(Transform::rotate_z(angle));
    }

    void destroy() {
        vertices.destroy();
        faces.destroy();
        projectionMatrix.destroy();
        free(randomFaceColors);
        free(plot_points);
    }
};
