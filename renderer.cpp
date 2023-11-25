// for initializing and shutdown functions
#include <SDL2/SDL.h>
// for rendering images and graphics on screen
#include <SDL2/SDL_image.h>
// for using SDL_Delay() functions
#include <SDL2/SDL_timer.h>

#include "renderer.h"

const char* objectList[] = {
    "obj/apartment.obj", "obj/basketball.obj", "obj/cat.obj", "obj/deer.obj",
    "obj/house.obj",     "obj/tank.obj",       "obj/wolf.obj"};

Renderer::Renderer(int argc, char** argv) {
    // returns zero on success else non-zero
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("error initializing SDL: %s\n", SDL_GetError());
    }
    GPU::init();
    window = SDL_CreateWindow("GAME", SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    renderer = SDL_CreateRenderer(window, -1, RENDER_FLAGS);

    camera.init(this, {0, 0, 0});
    projection.init(this);
    object = Object3D::loadObj(argc > 1 ? argv[1] : NULL, this);
    // object.translate({0.2, 0.4, 0.2});
    // object.rotate_y(M_PI / 6);
}

Renderer::~Renderer() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Renderer::draw(bool dumpMatrices) { object.draw(dumpMatrices); }

void Renderer::run() {
    int close = 0;
    bool keys[322] = {false};
    bool dumpVertices = true;
    Uint64 lastTick = SDL_GetTicks64();
    int objectCount = 0;
    while (!close) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    close = 1;
                    break;
                case SDL_KEYDOWN: {
                    if (event.key.keysym.sym < 322)
                        keys[event.key.keysym.sym] = true;
                    if (event.key.keysym.sym == SDLK_x) {
                        dumpVertices = true;
                    }
                    break;
                };
                case SDL_KEYUP: {
                    if (event.key.keysym.sym < 322)
                        keys[event.key.keysym.sym] = false;
                    break;
                };
            }
        }
        if (keys[SDLK_n]) {
            object.destroy();
            camera.init(this, {0, 0, 0});
            projection.init(this);
            object = Object3D::loadObj(objectList[objectCount], this);
            objectCount = (objectCount + 1) % 7;
            keys[SDLK_n] = false;
        }
        camera.control(keys);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        draw(dumpVertices);
        MEASURE(SDL_RenderPresent(renderer));
        Uint64 currentTick = SDL_GetTicks64();
        printf("FrameTime: %3ldms\r", currentTick - lastTick);
        fflush(stdout);
        lastTick = currentTick;

        // SDL_Delay(1000 / FPS);
    }
}
