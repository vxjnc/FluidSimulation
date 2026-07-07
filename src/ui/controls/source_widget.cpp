#include "source_widget.hpp"

#include <print>

#include <imgui.h>
#include <pfr.hpp>

#include "src/app_settings.hpp"
#include "src/compute/fluid_source.hpp"
#include "src/ui/widgets/velocity_input.hpp"

bool SourceWidget::render(FluidSource& s, size_t idx, AppSettings& settings) {
    ImGui::PushID(static_cast<int>(idx));

    ImGui::Checkbox("##active", &s.active);
    ImGui::SameLine();
    ImGui::Text("Source %zu", idx + 1);

    int current_form = static_cast<int>(s.form);
    const char* forms[] = {"Circle", "Line", "Radial"};
    if (ImGui::Combo("Form", &current_form, forms, sizeof(forms) / sizeof(forms[0]))) {
        s.form = static_cast<FluidSource::Form>(current_form);
    }

    bool has_velocity = static_cast<bool>(s.mode_mask & FluidSource::Mode::Velocity);
    bool has_dye =
        static_cast<bool>(s.mode_mask & (FluidSource::Mode::DyeAdditive | FluidSource::Mode::DyeReplace));

    ImGui::Text("Inject:");
    ImGui::SameLine();
    if (ImGui::Checkbox("Velocity", &has_velocity)) {
        if (has_velocity) {
            s.mode_mask |= FluidSource::Mode::Velocity;
        }
        else {
            s.mode_mask &= ~FluidSource::Mode::Velocity;
        }
    }
    ImGui::SameLine();
    if (ImGui::Checkbox("Dye", &has_dye)) {
        if (has_dye) {
            s.mode_mask |= FluidSource::Mode::DyeAdditive;
            s.mode_mask &= ~FluidSource::Mode::DyeReplace;
        }
        else {
            s.mode_mask &= ~(FluidSource::Mode::DyeAdditive | FluidSource::Mode::DyeReplace);
        }
    }

    if (has_dye) {
        bool additive = static_cast<bool>(s.mode_mask & FluidSource::Mode::DyeAdditive);
        if (ImGui::RadioButton("Additive", additive)) {
            s.mode_mask &= ~FluidSource::Mode::DyeReplace;
            s.mode_mask |= FluidSource::Mode::DyeAdditive;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Replace", !additive)) {
            s.mode_mask &= ~FluidSource::Mode::DyeAdditive;
            s.mode_mask |= FluidSource::Mode::DyeReplace;
        }
    }

    ImGui::SliderFloat("X", &s.x, 0.0f, 1.0f);
    ImGui::SliderFloat("Y", &s.y, 0.0f, 1.0f);

    if (has_velocity || s.form == FluidSource::Form::Line) {
        Widgets::VelocityInput("Velocity", s.vx, s.vy, settings.ui.velocityMode);
    }

    const char* radius_label = (s.form == FluidSource::Form::Line) ? "Length" : "Radius";
    ImGui::SliderFloat(radius_label, &s.radius, 0.f, 1.f);

    ImGui::ColorEdit3("Color", s.color.data());

    bool remove = ImGui::Button("Remove");

    ImGui::PopID();
    ImGui::Separator();

    return !remove;
}
