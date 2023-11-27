#include "object3d.h"

#include <time.h>

#include "renderer.h"

void Object3D::movement() { rotate_y(fmod((double)SDL_GetTicks64(), 0.005)); }
void Object3D::prepare() {
    projectionMatrix = ProjectionMatrix(vertices.row, 4);
    plot_points =
        (SDL_Point *)malloc(sizeof(SDL_Point) * (faces_row * FACES_COL));
    sdl_vertices =
        (SDL_Vertex *)malloc(sizeof(SDL_Vertex) * faces_row * FACES_COL);
    randomFaceColors =
        (Uint8 *)malloc(sizeof(Uint8) * faces_row * FACES_COL * 3);
    srand(time(NULL));
    for (int i = 0; i < faces_row; i++) {
        randomFaceColors[i * 3] = rand() % 255;
        randomFaceColors[i * 3 + 1] = rand() % 255;
        randomFaceColors[i * 3 + 2] = rand() % 255;
    }
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
        // faces.print();
    }
#endif
    MEASURE_START(drawLines);
    Uint8 *faceColor = randomFaceColors;
    int pointCount = 0;
    for (int i = 0; i < faces_row; i++) {
        const int *face = &faces.data()[i * FACES_COL];
        int bakPointCount = pointCount;
        for (int j = 0; j < FACES_COL; j++) {
            double px = projectionMatrix.at(face[j], 0);
            double py = projectionMatrix.at(face[j], 1);
            if (px == renderer->H_WIDTH || py == renderer->H_HEIGHT) {
                pointCount = bakPointCount;
                faceColor = randomFaceColors + pointCount;
                break;
            }
            sdl_vertices[pointCount].position = {(float)px, (float)py};
            sdl_vertices[pointCount].color = {*faceColor++, *faceColor++,
                                              *faceColor++, 255};
            sdl_vertices[pointCount].tex_coord = {1.0, 1.0};
            // if (dumpMatrices)
            //    printf("%g %d %d\n", face[j], plot_points[pointCount].x,
            //           plot_points[pointCount].y);
            pointCount++;
        }
    }
    SDL_RenderGeometry(renderer->renderer, NULL, sdl_vertices, pointCount, NULL,
                       0);
    MEASURE_END(drawLines);
    /*
    MEASURE_START(drawPoints);
    for (int i = 0; i < projectionMatrix.row; i++) {
        double x = projectionMatrix.at(i, 0);
        double y = projectionMatrix.at(i, 1);
        if (x == renderer->H_WIDTH || y == renderer->H_HEIGHT) continue;
        SDL_RenderDrawPoint(renderer->renderer, x, y);
    }
    MEASURE_END(drawPoints);
    */
}

Object3D Object3D::loadObj(const char *file, Renderer *r) {
    if (file == NULL) {
        return loadObj("obj/cat.obj", r);
    }
    FILE *f = fopen(file, "r");
    if (f == NULL) {
        printf("[Error] Cannot load obj file from: %s", file);
        return loadObj("obj/cat.obj", r);
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
                // printf("face %d: %d %d %d\n", fcount++, f1, f2, f3);
                obj.faces.push_back(f1);
                obj.faces.push_back(f2);
                obj.faces.push_back(f3);
                // if (obj.faces_col == 4) {
                //    obj.faces.push_back(f3);
                // }
                obj.faces_row++;
            } else {
                // printf("4\n");
                int f4 = strtol(buffer, &buffer, 10) - 1;
                if (f4 == -1) f4 = f3;
                // printf("face %d: %d %d %d %d\n", fcount++, f1, f2, f3, f4);
                obj.faces.push_back(f1);
                obj.faces.push_back(f2);
                obj.faces.push_back(f3);
                // break the quad into triangles
                obj.faces.push_back(f2);
                obj.faces.push_back(f3);
                obj.faces.push_back(f4);
                obj.faces_row++;
                obj.faces_row++;
            }
        }
        // skip line
        while (*buffer++ != '\n')
            ;
    }
    free(bak);

    obj.vertices.finalize_dimension();
    obj.prepare();

    return obj;
}
