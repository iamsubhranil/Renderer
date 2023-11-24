#include "object3d.h"

#include <time.h>

#include "renderer.h"

void Object3D::movement() { rotate_y(fmod((double)SDL_GetTicks64(), 0.005)); }
void Object3D::prepare() {
    projectionMatrix = ProjectionMatrix(vertices.row, 4);
    plot_points = (SDL_Point *)malloc(sizeof(SDL_Point) * (faces.col + 1));
    randomFaceColors = (int *)malloc(sizeof(int) * faces.row * 3);
    srand(time(NULL));
    for (int i = 0; i < faces.row; i++) {
        randomFaceColors[i * 3] = rand();
        randomFaceColors[i * 3 + 1] = rand();
        randomFaceColors[i * 3 + 2] = rand();
    }
    faces.moveToCpu();
}

void Object3D::screenProjection(bool dumpMatrices) {
    (void)dumpMatrices;
#ifdef DEBUG
    if (dumpMatrices) {
        printf("vertices:\n");
        vertices.print();
        printf("cameraMatrix:\n");
        renderer->camera.cameraMatrix().print();
    }
#endif
    projectionMatrix.multiply_and_assign(vertices,
                                         renderer->camera.cameraMatrix());
#ifdef DEBUG
    if (dumpMatrices) {
        printf("vertices * cameraMatrix:\n");
        projectionMatrix.print();
        printf("projection_matrix:\n");
        renderer->projection.projection_matrix.print();
    }
#endif
    projectionMatrix.multiply(renderer->projection.projection_matrix);
#ifdef DEBUG
    if (dumpMatrices) {
        printf("vertices * cameraMatrix * projection_matrix:\n");
        projectionMatrix.print();
    }
#endif
    projectionMatrix.normalizeAndCutoff();
#ifdef DEBUG
    if (dumpMatrices) {
        printf("vertices.normalizeAndCutoff():\n");
        projectionMatrix.print();
        printf("to_screen_matrix:\n");
        renderer->projection.to_screen_matrix.print();
    }
#endif
    projectionMatrix.multiply(renderer->projection.to_screen_matrix);
#ifdef DEBUG
    if (dumpMatrices) {
        printf(
            "(vertices * cameraMatrix * projectionMatrix).normalizeAndCutoff() "
            "* to_screen_matrix:\n");
        projectionMatrix.print();
        faces.print();
    }
#endif
    MEASURE_START(drawLines);
    int *faceColor = randomFaceColors;
    for (int i = 0; i < faces.row; i++) {
        int pointCount = 0;
        double *face = &faces.cpu_values[i * faces.col];
        for (int j = 0; j < faces.col; j++) {
            plot_points[pointCount].x = projectionMatrix.at(face[j], 0);
            plot_points[pointCount].y = projectionMatrix.at(face[j], 1);
            // if (dumpMatrices)
            //    printf("%g %d %d\n", face[j], plot_points[pointCount].x,
            //           plot_points[pointCount].y);
            if (plot_points[pointCount].x == renderer->H_WIDTH ||
                plot_points[pointCount].y == renderer->H_HEIGHT) {
                continue;
            }
            pointCount++;
        }
        SDL_SetRenderDrawColor(renderer->renderer, *faceColor, *(faceColor + 1),
                               *(faceColor + 2), 255);
        faceColor += 3;
        SDL_RenderDrawLines(renderer->renderer, plot_points, pointCount);
    }
    MEASURE_END(drawLines);
    MEASURE_START(drawPoints);
    for (int i = 0; i < projectionMatrix.row; i++) {
        double x = projectionMatrix.at(i, 0);
        double y = projectionMatrix.at(i, 1);
        if (x == renderer->H_WIDTH || y == renderer->H_HEIGHT) continue;
        SDL_RenderDrawPoint(renderer->renderer, x, y);
    }
    MEASURE_END(drawPoints);
}

Object3D Object3D::loadObj(const char *file, Renderer *r) {
    if (file == NULL) {
        return Object3D(r);
    }
    FILE *f = fopen(file, "r");
    if (f == NULL) {
        printf("[Error] Cannot load obj file from: %s", file);
        return Object3D(r);
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    char *buffer = (char *)malloc(size + 1);
    char *bak = buffer;
    fseek(f, 0, SEEK_SET);
    fread(buffer, size, 1, f);
    buffer[size] = 0;
    fclose(f);

    Object3D obj;
    obj.renderer = r;
    obj.vertices.col = 4;
    obj.faces.col = 0;

    // int vcount = 0, fcount = 0;
    while (*buffer) {
        if (*buffer == 'v' && *(buffer + 1) == ' ') {
            buffer++;  // v
            buffer++;  // ' '
            double x = strtod(buffer, &buffer);
            double y = strtod(buffer, &buffer);
            double z = strtod(buffer, &buffer);
            // printf("vertex %d: %f %f %f\n", vcount++, x, y, z);
            obj.vertices.appendRow(x, y, z, 1.0);
        } else if (*buffer == 'f' && *(buffer + 1) == ' ') {
            buffer++;  // f
            buffer++;  // ' '
            int f1 = strtol(buffer, &buffer, 10) - 1;
            // should stop at '/', proceed until next ' '
            while (*buffer++ != ' ')
                ;
            int f2 = strtol(buffer, &buffer, 10) - 1;
            // should stop at '/', proceed until next ' '
            while (*buffer++ != ' ')
                ;
            int f3 = strtol(buffer, &buffer, 10) - 1;
            // should stop at '/', proceed until next ' '
            while (*buffer != ' ' && *buffer != '\n') {
                buffer++;
            }
            // printf("%d\n", *buffer);
            if (*buffer == '\n' || (*buffer == ' ' && *(buffer + 1) == '\n')) {
                // we don't have 4th vertex
                // make sure col is inited with 3
                if (obj.faces.col == 0) obj.faces.col = 3;
                // printf("face %d: %d %d %d\n", fcount++, f1, f2, f3);
                if (obj.faces.col == 4) {
                    obj.faces.appendRow(f1, f2, f3, f3);
                } else
                    obj.faces.appendRow(f1, f2, f3);
            } else {
                if (obj.faces.col == 0) obj.faces.col = 4;
                int f4 = strtol(buffer, &buffer, 10) - 1;
                if (f4 == -1) f4 = f3;
                // printf("face %d: %d %d %d %d\n", fcount++, f1, f2, f3, f4);
                obj.faces.appendRow(f1, f2, f3, f4);
            }
        }
        // skip line
        while (*buffer++ != '\n')
            ;
    }
    free(bak);

    obj.vertices.finalize_dimension();
    obj.faces.finalize_dimension();
    obj.prepare();

    return obj;
}
