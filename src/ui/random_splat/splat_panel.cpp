#include "splat_panel.hpp"

#include <utility>

#include <imgui.h>

#include "src/compute/fluid_source.hpp"
#include "src/ui/random_splat/splat_settings.hpp"

void SplatPanel::render(bool& open, SplatSettings& settings, VelocityInputMode velMode) {
    ImGui::Begin("Random Splat", &open);
    pendingSplats_ = splatWidget_.render(settings, velMode);
    ImGui::End();
}

std::optional<std::vector<FluidSource>> SplatPanel::takeSplats() {
    return std::exchange(pendingSplats_, std::nullopt);
}
