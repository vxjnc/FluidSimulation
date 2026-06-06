#pragma once
#include <optional>
#include <utility>
#include <vector>

#include <imgui.h>

#include "src/compute/fluid_source.hpp"
#include "src/ui/fluid_viewport.hpp"
#include "src/ui/random_splat/splat_settings.hpp"
#include "src/ui/random_splat/splat_widget.hpp"
#include "src/ui/ui_settings.hpp"

class SplatPanel {
public:
    void render(bool& open, SplatSettings& settings, const FluidViewport& viewport,
                VelocityInputMode velMode) {
        ImGui::Begin("Random Splat", &open);
        pendingSplats_ = splatWidget_.render(settings, viewport, velMode);
        ImGui::End();
    }

    std::optional<std::vector<FluidSource>> takeSplats() {
        return std::exchange(pendingSplats_, std::nullopt);
    }

private:
    SplatWidget splatWidget_;
    std::optional<std::vector<FluidSource>> pendingSplats_;
};
