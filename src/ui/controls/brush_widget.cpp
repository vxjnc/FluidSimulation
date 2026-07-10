#include "brush_widget.hpp"

#include <imgui.h>
#include <magic_enum/magic_enum.hpp>
#include <magic_enum/magic_enum_flags.hpp>

#include "src/app_settings.hpp"
#include "src/ui/widgets/combo_enum.hpp"

using namespace magic_enum::bitwise_operators;

void BrushWidget::render(AppSettings& settings) {
    Widgets::ComboEnum("##brush", settings.brushMode);

    ImGui::SliderFloat("Radius", &settings.brushRadius, 0.f, 1.f);

    constexpr FluidSource::Mode dyeGroup = FluidSource::Mode::DyeAdditive | FluidSource::Mode::DyeReplace;

    if (settings.brushMode == BrushMode::Inject) {
        ImGui::SliderFloat("Strength", &settings.brushStrength, 1.0f, 100.0f);
        bool vel = static_cast<bool>(settings.brushModeMask & FluidSource::Mode::Velocity);
        bool dye = static_cast<bool>(settings.brushModeMask & dyeGroup);

        ImGui::Text("Inject:");
        ImGui::SameLine();
        if (ImGui::Checkbox("Velocity", &vel)) {
            settings.brushModeMask ^= FluidSource::Mode::Velocity;
        }
        ImGui::SameLine();
        if (ImGui::Checkbox("Dye", &dye)) {
            settings.brushModeMask &= ~dyeGroup;
            if (dye) {
                settings.brushModeMask |= FluidSource::Mode::DyeAdditive;
            }
        }

        if (dye) {
            bool additive = static_cast<bool>(settings.brushModeMask & FluidSource::Mode::DyeAdditive);
            if (ImGui::RadioButton("Additive", additive)) {
                settings.brushModeMask =
                    (settings.brushModeMask & ~dyeGroup) | FluidSource::Mode::DyeAdditive;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Replace", !additive)) {
                settings.brushModeMask = (settings.brushModeMask & ~dyeGroup) | FluidSource::Mode::DyeReplace;
            }
        }
    }
}
