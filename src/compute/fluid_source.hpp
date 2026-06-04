#pragma once

#include <array>
#include <cstdint>
#include <span>

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
    FluidSource(float x, float y, float vx, float vy, float radius, std::span<const float, 3> color,
                Form form = Form::CIRCLE)
        : x(x), y(y), vx(vx), vy(vy), radius(radius), form(form) {
        this->color[0] = color[0];
        this->color[1] = color[1];
        this->color[2] = color[2];
    }

    std::array<float, 3> color{1.0f, 1.0f, 1.0f};

    float x = 0.0f, y = 0.0f;
    float vx = 0.0f, vy = 100.0f;
    float radius = 4.0f;
    bool active = true;

    int mode_mask = Mode::VELOCITY | Mode::DYE;
    Form form = Form::CIRCLE;
};
