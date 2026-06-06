#pragma once
#include <cmath>
#include <numbers>

#include <imgui.h>

#include "src/ui/ui_settings.hpp"

namespace VelocityInput {
    inline bool render(const char* label, float& vx, float& vy, VelocityInputMode mode) {
        ImGui::PushID(label);
        bool changed = false;

        if (mode == VelocityInputMode::XY) {
            float v[2] = {vx, vy};
            if (ImGui::DragFloat2(label, v, 1.f, -500.f, 500.f)) {
                vx = v[0];
                vy = v[1];
                changed = true;
            }
        }
        else {
            float angle = std::atan2(vy, vx) * 180.f / std::numbers::pi_v<float>;
            angle = std::fmod(angle + 360.f, 360.f);
            float magnitude = std::hypot(vx, vy);
            float v[2] = {angle, magnitude};
            if (ImGui::DragFloat2(label, v, 1.f, 0.f, 500.f, "%.1f")) {
                float rad = v[0] * std::numbers::pi_v<float> / 180.f;
                vx = v[1] * std::cos(rad);
                vy = v[1] * std::sin(rad);
                changed = true;
            }
            ImGui::SameLine();
            ImGui::TextDisabled("angle/mag");
        }

        ImGui::PopID();
        return changed;
    }
};
