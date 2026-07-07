#pragma once
#include <array>

#include "src/compute/fluid_source.hpp"
#include "src/render/render_settings.hpp"
#include "src/scripting/plugin_settings.hpp"
#include "src/scripting/scripting_settings.hpp"
#include "src/ui/random_splat/splat_settings.hpp"
#include "src/ui/ui_settings.hpp"
#include "src/utils/observable.hpp"

enum class BrushMode { Inject, PaintWall };

struct AppSettings {
    bool paused = false;
    Observable<float> dt = 0.01f;

    Observable<float> velDissipation = 0.2f;
    Observable<float> dyeDissipation = 0.25f;
    Observable<float> curlStrength = 30.f;

    BrushMode brushMode = BrushMode::Inject;
    FluidSource::Mode brushModeMask = FluidSource::Mode::Velocity | FluidSource::Mode::DyeAdditive;
    float brushRadius = 0.1f;
    float brushStrength = 10.0f;
    std::array<float, 3> brushColor;

    float simScale = 0.8f;
    float dyeScale = 1.0f;

    RenderSettings renderSettings;
    SplatSettings splatSettings;

    UISettings ui;
    ScriptingSettings scripting;
    PluginSettings plugins;

    bool dockInitialized = false;
};
