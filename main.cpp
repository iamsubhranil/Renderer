#include "renderer.h"

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    Renderer r = Renderer(argc, argv);
    r.run();

    return 0;
}
