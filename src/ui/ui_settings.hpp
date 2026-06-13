#pragma once
#include <cstdint>

enum class VelocityInputMode : uint8_t {
    XY,
    Polar,
};

struct UISettings {
    VelocityInputMode velocityMode = VelocityInputMode::Polar;
    bool showSourceOverlay = true;

    bool menuBarVisible = true;
    bool settingsOpen = false;
    bool aboutOpen = false;
    bool dockInitialized = false;
};
