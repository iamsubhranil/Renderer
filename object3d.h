#pragma once

#include <vector>

#include "matrix.h"

struct Renderer;
struct SDL_Point;
struct SDL_Vertex;

struct Object3D {
    Renderer *renderer;
    ProjectionMatrix vertices;
    const static int FACES_COL = 3;
    int faces_row;
    std::vector<int> faces;
    ProjectionMatrix projectionMatrix;  // vertex.row * 4
    SDL_Point *plot_points;
    SDL_Vertex *sdl_vertices;
    Uint8 *randomFaceColors;

    Object3D() { faces_row = 0; }
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
        faces_row = 0;
        projectionMatrix.destroy();
        free(randomFaceColors);
        free(plot_points);
    }
};
