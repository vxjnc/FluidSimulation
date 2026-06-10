#pragma once
#include <array>
#include <cmath>
#include <numbers>
#include <optional>
#include <random>
#include <vector>

#include <imgui.h>

#include "src/compute/fluid_source.hpp"
#include "src/ui/random_splat/splat_settings.hpp"
#include "src/ui/ui_settings.hpp"
#include "src/utils/color_generator.hpp"

class SplatWidget {
public:
    std::optional<std::vector<FluidSource>> render(SplatSettings& s, VelocityInputMode velMode) {
        ImGui::Text("Random Splat");

        ImGui::DragIntRange2("Count", &s.countMin, &s.countMax, 1.f, 1, 100);
        ImGui::DragFloatRange2("Radius", &s.radiusMin, &s.radiusMax, 0.05f, 0.f, 1.f);

        ImGui::Checkbox("Apply Velocity", &s.applyVelocity);
        if (s.applyVelocity) {
            if (velMode == VelocityInputMode::XY) {
                ImGui::DragFloatRange2("VX", &s.vxMin, &s.vxMax, 1.f, -500.f, 500.f);
                ImGui::DragFloatRange2("VY", &s.vyMin, &s.vyMax, 1.f, -500.f, 500.f);
            }
            else {
                ImGui::DragFloatRange2("Angle", &s.angleMin, &s.angleMax, 1.f, 0.f, 360.f, "%.1f°");
                ImGui::DragFloatRange2("Mag", &s.magMin, &s.magMax, 1.f, 0.f, 500.f);
            }
        }

        ImGui::Checkbox("Apply Color", &s.applyColor);

        if (!ImGui::Button("Splat!", ImVec2(-1, 0))) {
            return std::nullopt;
        }

        return generate(s, velMode);
    }

private:
    std::mt19937 rng_{std::random_device{}()};

    std::vector<FluidSource> generate(const SplatSettings& s, VelocityInputMode velMode) {
        std::uniform_int_distribution<int> countDist(s.countMin, s.countMax);
        std::uniform_real_distribution<float> posDist(0.f, 1.f);
        std::uniform_real_distribution<float> radiusDist(s.radiusMin, s.radiusMax);

        std::uniform_real_distribution<float> vxDist(s.vxMin, s.vxMax);
        std::uniform_real_distribution<float> vyDist(s.vyMin, s.vyMax);
        std::uniform_real_distribution<float> angleDist(s.angleMin, s.angleMax);
        std::uniform_real_distribution<float> magDist(s.magMin, s.magMax);

        int count = countDist(rng_);
        std::vector<FluidSource> sources;
        sources.reserve(static_cast<size_t>(count));

        for (int i = 0; i < count; ++i) {
            float vx = 0.f, vy = 0.f;
            if (s.applyVelocity) {
                if (velMode == VelocityInputMode::XY) {
                    vx = vxDist(rng_);
                    vy = vyDist(rng_);
                }
                else {
                    float rad = angleDist(rng_) * std::numbers::pi_v<float> / 180.f;
                    float mag = magDist(rng_);
                    vx = mag * std::cos(rad);
                    vy = mag * std::sin(rad);
                }
            }

            std::array<float, 3> color =
                s.applyColor ? ColorUtils::Generator::randomPastel(rng_) : std::array{1.f, 1.f, 1.f};

            sources.emplace_back(posDist(rng_), posDist(rng_), vx, vy, radiusDist(rng_), color).mode_mask =
                (s.applyVelocity ? FluidSource::Mode::VELOCITY : 0) |
                (s.applyColor ? FluidSource::Mode::DYE : 0);
            ;
        }

        return sources;
    }
};
