#include "source_widget.hpp"

#include <imgui.h>

#include "src/app_settings.hpp"
#include "src/compute/fluid_source.hpp"
#include "src/ui/widgets/velocity_input.hpp"

bool SourceWidget::render(FluidSource& s, size_t idx, AppSettings& settings) {
    ImGui::PushID(static_cast<int>(idx));

    ImGui::Checkbox("##active", &s.active);
    ImGui::SameLine();
    ImGui::Text("Source %zu", idx + 1);

    int current_form = static_cast<int>(s.form);
    const char* forms[] = {"Circle", "Line"};
    if (ImGui::Combo("Form", &current_form, forms, 2)) {
        s.form = static_cast<FluidSource::Form>(current_form);
    }

    bool has_velocity = s.mode_mask & FluidSource::Mode::VELOCITY;
    bool has_dye = s.mode_mask & (FluidSource::Mode::DYE_ADDITIVE | FluidSource::Mode::DYE_REPLACE);

    ImGui::Text("Inject:");
    ImGui::SameLine();
    if (ImGui::Checkbox("Velocity", &has_velocity)) {
        if (has_velocity) {
            s.mode_mask |= FluidSource::Mode::VELOCITY;
        }
        else {
            s.mode_mask &= ~FluidSource::Mode::VELOCITY;
        }
    }
    ImGui::SameLine();
    if (ImGui::Checkbox("Dye", &has_dye)) {
        if (has_dye) {
            s.mode_mask |= FluidSource::Mode::DYE_ADDITIVE;
            s.mode_mask &= ~FluidSource::Mode::DYE_REPLACE;
        }
        else {
            s.mode_mask &= ~(FluidSource::Mode::DYE_ADDITIVE | FluidSource::Mode::DYE_REPLACE);
        }
    }

    if (has_dye) {
        bool additive = s.mode_mask & FluidSource::Mode::DYE_ADDITIVE;
        if (ImGui::RadioButton("Additive", additive)) {
            s.mode_mask &= ~FluidSource::Mode::DYE_REPLACE;
            s.mode_mask |= FluidSource::Mode::DYE_ADDITIVE;
        }
        ImGui::SameLine();
        if (ImGui::RadioButton("Replace", !additive)) {
            s.mode_mask &= ~FluidSource::Mode::DYE_ADDITIVE;
            s.mode_mask |= FluidSource::Mode::DYE_REPLACE;
        }
    }

    ImGui::SliderFloat("X", &s.x, 0.0f, 1.0f);
    ImGui::SliderFloat("Y", &s.y, 0.0f, 1.0f);

    if (has_velocity || s.form == FluidSource::Form::LINE) {
        Widgets::VelocityInput("Velocity", s.vx, s.vy, settings.ui.velocityMode);
    }

    const char* radius_label = (s.form == FluidSource::Form::LINE) ? "Length" : "Radius";
    ImGui::SliderFloat(radius_label, &s.radius, 0.f, 1.f);

    ImGui::ColorEdit3("Color", s.color.data());

    bool remove = ImGui::Button("Remove");

    ImGui::PopID();
    ImGui::Separator();

    return !remove;
}
