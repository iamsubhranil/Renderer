#include "camera.h"

#include <SDL2/SDL_keycode.h>

#include "renderer.h"

void Camera::init(Renderer *r, Point3D p) {
    position.fill(p.x, p.y, p.z, 1);
    renderer = r;
    v_fov = H_FOV * ((double)r->HEIGHT / (double)r->WIDTH);
}

void Camera::control(const bool *keys) {
    if (keys[SDLK_a]) {
        position.multiply_sub(right, MOVING_SPEED);
    }
    if (keys[SDLK_d]) {
        position.multiply_add(right, MOVING_SPEED);
    }
    if (keys[SDLK_w]) {
        position.multiply_add(forward, MOVING_SPEED);
    }
    if (keys[SDLK_s]) {
        position.multiply_sub(forward, MOVING_SPEED);
    }
    if (keys[SDLK_q]) {
        position.multiply_add(up, MOVING_SPEED);
    }
    if (keys[SDLK_e]) {
        position.multiply_sub(up, MOVING_SPEED);
    }

    /*
    if (keys[SDLK_LEFT]) {
        cameraYaw(-ROTATION_SPEED);
    }

    if (keys[SDLK_RIGHT]) {
        cameraYaw(ROTATION_SPEED);
    }

    if (keys[SDLK_LEFT]) {
        cameraPitch(-ROTATION_SPEED);
    }

    if (keys[SDLK_RIGHT]) {
        cameraPitch(ROTATION_SPEED);
    }*/
}

void Camera::cameraYaw(double angle) {
    auto rotate = Transform::rotate_y(angle);
    forward = forward * rotate;
    right = right * rotate;
    up = up * rotate;
}

void Camera::cameraPitch(double angle) {
    auto rotate = Transform::rotate_x(angle);
    forward = forward * rotate;
    right = right * rotate;
    up = up * rotate;
}
