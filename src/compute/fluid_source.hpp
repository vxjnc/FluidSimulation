#pragma once

#include <cstdint>

struct FluidSource {
    enum Mode : uint8_t {
        VELOCITY = 1 << 0,
        DYE = 1 << 1,
    };
    enum class Form : uint8_t {
        CIRCLE,
        LINE,
    };

    FluidSource() = default;
    FluidSource(float x, float y, float vx, float vy, float radius, Form form = Form::CIRCLE)
        : x(x), y(y), vx(vx), vy(vy), radius(radius), form(form) {}

    float x = 0.0f, y = 0.0f;
    float vx = 0.0f, vy = 100.0f;
    float radius = 4.0f;
    bool active = true;

    int mode_mask = Mode::VELOCITY | Mode::DYE;
    Form form = Form::CIRCLE;
};
