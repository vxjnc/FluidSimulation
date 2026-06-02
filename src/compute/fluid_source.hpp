#pragma once

struct FluidSource {
    FluidSource() = default;
    FluidSource(float x, float y, float vx, float vy, float radius)
        : x(x), y(y), vx(vx), vy(vy), radius(radius) {}

    float x = 0.0f, y = 0.0f;
    float vx = 0.0f, vy = 100.0f;
    float radius = 4.0f;
    bool active = true;
};
