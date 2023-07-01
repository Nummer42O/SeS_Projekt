#include "helpers.hpp"

Vec3d rotateYCenter(Vec3d vec, int16_t angle_idx) {
    return Vec3d{
         cos_values[angle_idx] * vec.x + sin_values[angle_idx] * vec.z,
         vec.y,
        -sin_values[angle_idx] * vec.x + cos_values[angle_idx] * vec.z,
    };
}

Vec3d rotateXCenter(Vec3d vec, int16_t angle_idx) {
    return Vec3d{
         vec.x,
         cos_values[angle_idx] * vec.y - sin_values[angle_idx] * vec.z,
         sin_values[angle_idx] * vec.y + cos_values[angle_idx] * vec.z,
    };
}

Vec3d move(Vec3d vec, float dx, float dy, float dz) {
    vec.x += dx;
    vec.y += dy;
    vec.z += dz;

    return vec;
}

Vec2d getScreenCoordsFrom3d(Vec3d vec) {
    if (vec.z >= 0.0) // behind/in viewing plane
    {
        return Vec2d{-1.0f, -1.0f};
    }

    // get normalized centered 2D coords
    Vec2d vec_2d{vec.x / vec.z, vec.y / vec.z};

    // expand and move to screen coordinates
    vec_2d.x = ((vec_2d.x + 1) / 2) * SCREEN_WIDTH;
    vec_2d.y = ((vec_2d.y + 1) / 2) * SCREEN_HEIGHT;

    return vec_2d;
}

bool checkInBounds(Vec2d vec) {
    return (
        vec.x >= 0.0f && vec.x < SCREEN_WIDTH &&
        vec.y >= 0.0f && vec.y < SCREEN_HEIGHT
    );
}