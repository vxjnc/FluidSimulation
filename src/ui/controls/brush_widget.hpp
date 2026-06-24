#pragma once
#include <imgui.h>

#include "src/app_settings.hpp"

class BrushWidget {
public:
    void render(AppSettings& settings) {
        static const char* brushModes[] = {"Inject", "Paint Wall"};
        int bm = static_cast<int>(settings.brushMode);
        if (ImGui::Combo("##brush", &bm, brushModes, sizeof(brushModes) / sizeof(brushModes[0]))) {
            settings.brushMode = static_cast<BrushMode>(bm);
        }

        ImGui::SliderFloat("Radius", &settings.brushRadius, 0.f, 1.f);

        if (settings.brushMode == BrushMode::Inject) {
            ImGui::SliderFloat("Strength", &settings.brushStrength, 1.0f, 100.0f);
            bool vel = settings.brushModeMask & FluidSource::Mode::VELOCITY;
            bool dye =
                settings.brushModeMask & (FluidSource::Mode::DYE_ADDITIVE | FluidSource::Mode::DYE_REPLACE);

            ImGui::Text("Inject:");
            ImGui::SameLine();
            if (ImGui::Checkbox("Velocity", &vel)) {
                settings.brushModeMask ^= FluidSource::Mode::VELOCITY;
            }
            ImGui::SameLine();
            if (ImGui::Checkbox("Dye", &dye)) {
                if (dye) {
                    settings.brushModeMask |= FluidSource::Mode::DYE_ADDITIVE;
                    settings.brushModeMask &= ~FluidSource::Mode::DYE_REPLACE;
                }
                else {
                    settings.brushModeMask &=
                        ~(FluidSource::Mode::DYE_ADDITIVE | FluidSource::Mode::DYE_REPLACE);
                }
            }

            if (dye) {
                bool additive = settings.brushModeMask & FluidSource::Mode::DYE_ADDITIVE;
                if (ImGui::RadioButton("Additive", additive)) {
                    settings.brushModeMask &= ~FluidSource::Mode::DYE_REPLACE;
                    settings.brushModeMask |= FluidSource::Mode::DYE_ADDITIVE;
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("Replace", !additive)) {
                    settings.brushModeMask &= ~FluidSource::Mode::DYE_ADDITIVE;
                    settings.brushModeMask |= FluidSource::Mode::DYE_REPLACE;
                }
            }
        }
    }
};
