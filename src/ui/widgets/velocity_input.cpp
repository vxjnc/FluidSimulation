#include "velocity_input.hpp"

#include <cmath>
#include <numbers>

#include <imgui.h>

#include "src/ui/ui_settings.hpp"

bool Widgets::VelocityInput(const char* label, float& vx, float& vy, VelocityInputMode mode) {
    ImGui::PushID(label);
    bool changed = false;

    if (mode == VelocityInputMode::XY) {
        ImGui::SetNextItemWidth(ImGui::CalcItemWidth() * 0.5f - 2.f);
        changed |= ImGui::DragFloat("##vx", &vx, 0.01f, -50.f, 50.f, "%.2f");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::CalcItemWidth() * 0.5f - 2.f);
        changed |= ImGui::DragFloat("##vy", &vy, 0.01f, -50.f, 50.f, "%.2f");
        ImGui::SameLine();
        ImGui::TextUnformatted(label);
    }
    else {
        float angle = std::atan2(vy, vx) * 180.f / std::numbers::pi_v<float>;
        angle = std::fmod(angle + 360.f, 360.f);
        float magnitude = std::hypot(vx, vy);

        ImGui::SetNextItemWidth(ImGui::CalcItemWidth() * 0.5f - 2.f);
        bool angleChanged = ImGui::DragFloat("##angle", &angle, 1.f, 0.f, 360.f, "%.1f°");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::CalcItemWidth() * 0.5f - 2.f);
        bool magChanged = ImGui::DragFloat("##mag", &magnitude, 0.1f, 0.f, 50.f, "%.2f");
        ImGui::SameLine();
        ImGui::TextUnformatted(label);

        if (angleChanged || magChanged) {
            float rad = angle * std::numbers::pi_v<float> / 180.f;
            vx = magnitude * std::cos(rad);
            vy = magnitude * std::sin(rad);
            changed = true;
        }
    }

    ImGui::PopID();
    return changed;
}
