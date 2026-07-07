#pragma once

#include <array>
#include <cstdint>
#include <span>

struct FluidSource {
    enum class Mode : uint8_t {
        Disabled = 0,
        Velocity = 1 << 0,
        DyeAdditive = 1 << 1,
        DyeReplace = 1 << 2,
    };
    friend constexpr Mode operator|(Mode a, Mode b) {
        return static_cast<Mode>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
    }
    friend constexpr Mode operator&(Mode a, Mode b) {
        return static_cast<Mode>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
    }
    friend constexpr Mode operator^(Mode a, Mode b) {
        return static_cast<Mode>(static_cast<uint8_t>(a) ^ static_cast<uint8_t>(b));
    }
    friend constexpr Mode operator~(Mode a) { return static_cast<Mode>(~static_cast<uint8_t>(a)); }
    friend constexpr Mode& operator^=(Mode& a, Mode b) { return a = a ^ b; }
    friend constexpr Mode& operator|=(Mode& a, Mode b) { return a = a | b; }
    friend constexpr Mode& operator&=(Mode& a, Mode b) { return a = a & b; }

    enum class Form : uint8_t {
        Circle,
        Line,
        Radial,
    };

    FluidSource() = default;
    FluidSource(float x, float y, float vx, float vy, float radius, std::span<const float, 3> color,
                Form form = Form::Circle)
        : x(x), y(y), vx(vx), vy(vy), radius(radius), form(form) {
        this->color[0] = color[0];
        this->color[1] = color[1];
        this->color[2] = color[2];
    }

    std::array<float, 3> color{1.0f, 1.0f, 1.0f};

    float x = 0.0f, y = 0.0f;
    float vx = 0.0f, vy = 1.f;
    float radius = 0.05f;
    bool active = true;

    Mode mode_mask = Mode::Velocity | Mode::DyeAdditive;
    Form form = Form::Circle;
};
