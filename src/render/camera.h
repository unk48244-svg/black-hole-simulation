#pragma once
#include "Rprm.h"

// Simple camera on top of Rprm.h. Looks from `position` at `target`.
struct Camera {
    Vec3 position = Vec3(0, 0, 3);
    Vec3 target   = Vec3(0, 0, 0);
    Vec3 up       = Vec3(0, 1, 0);
    double fovY   = radians(60.0);
    double nearZ  = 0.1;
    double farZ   = 1000.0;

    Mat4 view() const { return lookAt(position, target, up); }
    Mat4 projection(double aspect) const { return perspective(fovY, aspect, nearZ, farZ); }

    // Put the camera on a sphere around `target` (useful for inspecting objects).
    // yaw/pitch in radians, dist = distance from the target.
    void orbit(double yaw, double pitch, double dist) {
        pitch = clamp(pitch, -PI / 2 + 0.01, PI / 2 - 0.01);   // avoid flipping at the poles
        position = target + Vec3(std::cos(pitch) * std::sin(yaw),
                                 std::sin(pitch),
                                 std::cos(pitch) * std::cos(yaw)) * dist;
    }
};