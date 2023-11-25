#include "projection.h"

#include "renderer.h"

void Projection::init(Renderer *renderer) {
    near = renderer->camera.getNearPlane();
    far = renderer->camera.getFarPlane();
    right = tan(renderer->camera.getHFOV() / 2);
    left = -right;
    top = tan(renderer->camera.getVFOV() / 2);
    bottom = -top;

    double m00 = 2 / (right - left);
    double m11 = 2 / (top - bottom);
    double m22 = (far + near) / (far - near);
    double m32 = -2 * far * near / (far - near);
    projection_matrix.fill(m00, 0, 0, 0, 0, m11, 0, 0, 0, 0, m22, 1, 0, 0, m32,
                           0);

    double hw = renderer->H_WIDTH, hh = renderer->H_HEIGHT;
    to_screen_matrix.fill(hw, 0, 0, 0, 0, -hh, 0, 0, 0, 0, 1, 0, hw, hh, 0, 1);
}
