#pragma once
#include <cstdint>

enum class VelocityInputMode : uint8_t {
    XY,
    Polar,
};

struct UISettings {
    VelocityInputMode velocityMode = VelocityInputMode::Polar;
};
