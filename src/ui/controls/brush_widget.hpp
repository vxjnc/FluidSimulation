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
            bool vel = static_cast<bool>(settings.brushModeMask & FluidSource::Mode::Velocity);
            bool dye = static_cast<bool>(settings.brushModeMask &
                                         (FluidSource::Mode::DyeAdditive | FluidSource::Mode::DyeReplace));

            ImGui::Text("Inject:");
            ImGui::SameLine();
            if (ImGui::Checkbox("Velocity", &vel)) {
                settings.brushModeMask ^= FluidSource::Mode::Velocity;
            }
            ImGui::SameLine();
            if (ImGui::Checkbox("Dye", &dye)) {
                if (dye) {
                    settings.brushModeMask |= FluidSource::Mode::DyeAdditive;
                    settings.brushModeMask &= ~FluidSource::Mode::DyeReplace;
                }
                else {
                    settings.brushModeMask &=
                        ~(FluidSource::Mode::DyeAdditive | FluidSource::Mode::DyeReplace);
                }
            }

            if (dye) {
                bool additive = static_cast<bool>(settings.brushModeMask & FluidSource::Mode::DyeAdditive);
                if (ImGui::RadioButton("Additive", additive)) {
                    settings.brushModeMask &= ~FluidSource::Mode::DyeReplace;
                    settings.brushModeMask |= FluidSource::Mode::DyeAdditive;
                }
                ImGui::SameLine();
                if (ImGui::RadioButton("Replace", !additive)) {
                    settings.brushModeMask &= ~FluidSource::Mode::DyeAdditive;
                    settings.brushModeMask |= FluidSource::Mode::DyeReplace;
                }
            }
        }
    }
};
