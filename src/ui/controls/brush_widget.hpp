#pragma once
#include <imgui.h>

#include "src/app_settings.hpp"

class BrushWidget {
public:
    void render(AppSettings& settings) {
        static const char* brushModes[] = {"Inject", "Paint Wall"};
        int bm = static_cast<int>(settings.brushMode);
        if (ImGui::Combo("##brush", &bm, brushModes, 2)) {
            settings.brushMode = static_cast<BrushMode>(bm);
        }

        ImGui::SliderFloat("Radius", &settings.brushRadius, 1.0f, 100.0f);

        if (settings.brushMode == BrushMode::Inject) {
            ImGui::SliderFloat("Strength", &settings.brushStrength, 1.0f, 100.0f);
        }
    }
};
