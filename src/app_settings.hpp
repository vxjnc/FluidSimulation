#pragma once
#include <array>

#include "src/render/render_settings.hpp"
#include "src/ui/random_splat/splat_settings.hpp"
#include "src/ui/ui_settings.hpp"

enum class BrushMode { Inject, PaintWall };

struct AppSettings {
    bool paused = false;
    float dt = 0.01f;

    float velDissipation = 0.2f;
    float dyeDissipation = 0.25f;
    float curlStrength = 30.f;

    BrushMode brushMode = BrushMode::Inject;
    float brushRadius = 50.0f;
    float brushStrength = 10.0f;
    std::array<float, 3> brushColor;

    float simScale = 1.0f;
    float dyeScale = 1.0f;

    RenderSettings renderSettings;
    SplatSettings splatSettings;

    UISettings ui;
};
