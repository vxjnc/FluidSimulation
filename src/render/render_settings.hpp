#pragma once

#include <cstdint>

enum class RenderMode : uint8_t {
    Dye = 0,
    Velocity = 1,
    Pressure = 2,
    Divergence = 3,
    Curl = 4,
};

struct RenderSettings {
    RenderMode mode = RenderMode::Dye;
    bool showObstacles = true;
};
