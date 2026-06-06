#pragma once
#include <optional>
#include <random>
#include <vector>

#include <imgui.h>

#include "src/color_generator.hpp"
#include "src/compute/fluid_source.hpp"
#include "src/ui/fluid_viewport.hpp"
#include "src/ui/random_splat/splat_settings.hpp"

class SplatWidget {
public:
    std::optional<std::vector<FluidSource>> render(SplatSettings& s, const FluidViewport& viewport) {
        ImGui::Text("Random Splat");

        ImGui::DragIntRange2("Count", &s.countRange.min, &s.countRange.max, 1.f, 1, 100);
        ImGui::DragFloatRange2("Radius", &s.radiusRange.min, &s.radiusRange.max, 0.5f, 1.f, 200.f);

        ImGui::Checkbox("Apply Velocity", &s.applyVelocity);
        if (s.applyVelocity) {
            ImGui::DragFloatRange2("VX", &s.vxRange.min, &s.vxRange.max, 1.f, -500.f, 500.f);
            ImGui::DragFloatRange2("VY", &s.vyRange.min, &s.vyRange.max, 1.f, -500.f, 500.f);
        }

        ImGui::Checkbox("Apply Color", &s.applyColor);

        if (!ImGui::Button("Splat!", ImVec2(-1, 0))) {
            return std::nullopt;
        }

        return generate(s, viewport);
    }

private:
    std::mt19937 mt{std::random_device{}()};

    std::vector<FluidSource> generate(const SplatSettings& s, const FluidViewport& viewport) {
        // Безопасно получаем диапазоны (защита от min > max в ImGui)
        auto [countMin, countMax] = std::minmax(s.countRange.min, s.countRange.max);
        std::uniform_int_distribution<int> countDist(countMin, countMax);

        std::uniform_real_distribution<float> xDist(0.0f, static_cast<float>(viewport.w));
        std::uniform_real_distribution<float> yDist(0.0f, static_cast<float>(viewport.h));

        auto [radMin, radMax] = std::minmax(s.radiusRange.min, s.radiusRange.max);
        std::uniform_real_distribution<float> radiusDist(radMin, radMax);

        auto [vxMin, vxMax] = std::minmax(s.vxRange.min, s.vxRange.max);
        std::uniform_real_distribution<float> vxDist(vxMin, vxMax);

        auto [vyMin, vyMax] = std::minmax(s.vyRange.min, s.vyRange.max);
        std::uniform_real_distribution<float> vyDist(vyMin, vyMax);

        const size_t count = static_cast<size_t>(countDist(mt));
        std::vector<FluidSource> sources;
        sources.reserve(count);

        const auto mask =
            (s.applyVelocity ? FluidSource::Mode::VELOCITY : 0) | (s.applyColor ? FluidSource::Mode::DYE : 0);

        for (size_t i = 0; i < count; ++i) {
            const float vx = s.applyVelocity ? vxDist(mt) : 0.0f;
            const float vy = s.applyVelocity ? vyDist(mt) : 0.0f;

            const std::array<float, 3> color =
                s.applyColor ? ColorUtils::Generator::randomVibrant(mt) : std::array{1.0f, 1.0f, 1.0f};

            sources.emplace_back(xDist(mt), yDist(mt), vx, vy, radiusDist(mt), color).mode_mask = mask;
        }

        return sources;
    }
};
