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
            bool vel = settings.brushModeMask & FluidSource::Mode::VELOCITY;
            bool dye = settings.brushModeMask & FluidSource::Mode::DYE;
            ImGui::Text("Inject:");
            ImGui::SameLine();
            if (ImGui::Checkbox("Velocity", &vel)) {
                settings.brushModeMask ^= FluidSource::Mode::VELOCITY;
            }
            ImGui::SameLine();
            if (ImGui::Checkbox("Dye", &dye)) {
                settings.brushModeMask ^= FluidSource::Mode::DYE;
            }
        }
    }
};
