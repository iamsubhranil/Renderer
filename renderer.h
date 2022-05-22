#pragma once

#include <SDL2/SDL.h>

#include "camera.h"
#include "object3d.h"
#include "projection.h"

template <typename A, typename B>
struct Tuple {
    A x;
    B y;
};

struct Renderer {
    const int WIDTH = 1024, HEIGHT = 720;
    const Tuple<int, int> RES = {WIDTH, HEIGHT};
    const int H_HEIGHT = HEIGHT / 2, H_WIDTH = WIDTH / 2;
    const int FPS = 144;

    const Uint32 RENDER_FLAGS = SDL_RENDERER_ACCELERATED;

    SDL_Window *window;
    SDL_Renderer *renderer;

    Object3D object;
    Camera camera;
    Projection projection;

    Renderer(int argc, char **argv);
    ~Renderer();

    void createObjects();
    void draw(bool dumpMatrices);
    void run();
};
