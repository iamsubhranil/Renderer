#pragma once
#include "matrix.h"

struct Renderer;

struct Camera {
   private:
    ConstantMatrix<1, 4> forward = ConstantMatrix<1, 4>(0, 0, 1, 1);
    ConstantMatrix<1, 4> up = ConstantMatrix<1, 4>(0, 1, 0, 1);
    ConstantMatrix<1, 4> right = ConstantMatrix<1, 4>(1, 0, 0, 1);
    constexpr static double H_FOV = M_PI / 3;
    constexpr static double NEAR_PLANE = 0.1;
    constexpr static double FAR_PLANE = 200;
    constexpr static double MOVING_SPEED = 1.5;
    constexpr static double ROTATION_SPEED = 0.01;

    Renderer *renderer;
    ConstantMatrix<1, 4> position;
    double v_fov;

   public:
    Camera() {}
    void init(Renderer *r, Point3D p);
    void control(const bool *keys);
    void cameraYaw(double angle);
    void cameraPitch(double angle);

    ConstantMatrix<4, 4> translateMatrix() {
        return ConstantMatrix<4, 4>(1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0,
                                    -position.at(0), -position.at(1),
                                    -position.at(2), 1);
    }
    ConstantMatrix<4, 4> rotateMatrix() {
        return ConstantMatrix<4, 4>(right.at(0), up.at(0), forward.at(0), 0,
                                    right.at(1), up.at(1), forward.at(1), 0,
                                    right.at(2), up.at(2), forward.at(2), 0, 0,
                                    0, 0, 1);
    }
    ConstantMatrix<4, 4> cameraMatrix() {
        return translateMatrix() * rotateMatrix();
    }

    static constexpr double getFarPlane() { return FAR_PLANE; }
    static constexpr double getNearPlane() { return NEAR_PLANE; }
    static constexpr double getHFOV() { return H_FOV; }
    double getVFOV() { return v_fov; }
};
