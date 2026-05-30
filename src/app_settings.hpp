#pragma once
#include "src/render/render_settings.hpp"

enum class BrushMode { Inject, PaintWall };

struct AppSettings {
    bool paused = false;

    BrushMode brushMode = BrushMode::Inject;
    float brushRadius = 10.0f;
    float brushStrength = 10.0f;

    RenderSettings renderSettings;
};
