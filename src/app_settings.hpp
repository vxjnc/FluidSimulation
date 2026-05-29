#pragma once
#include "src/render/render_settings.hpp"

enum class BrushMode { Inject, PaintWall, EraseWall };

struct AppSettings {
    BrushMode brushMode = BrushMode::Inject;
    float brushRadius = 2.0f;

    RenderSettings renderSettings;
};
