#pragma once
#include <optional>
#include <utility>
#include <vector>

#include <imgui.h>

#include "src/compute/fluid_source.hpp"
#include "src/ui/fluid_viewport.hpp"
#include "src/ui/random_splat/splat_settings.hpp"
#include "src/ui/random_splat/splat_widget.hpp"

class SplatPanel {
public:
    void render(SplatSettings& settings, const FluidViewport& viewport) {
        ImGui::Begin("Random Splat");
        pendingSplats_ = splatWidget_.render(settings, viewport);
        ImGui::End();
    }

    std::optional<std::vector<FluidSource>> takeSplats() {
        return std::exchange(pendingSplats_, std::nullopt);
    }

private:
    SplatWidget splatWidget_;
    std::optional<std::vector<FluidSource>> pendingSplats_;
};
