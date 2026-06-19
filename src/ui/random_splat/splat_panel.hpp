#pragma once
#include <optional>
#include <vector>

#include "src/ui/random_splat/splat_widget.hpp"
#include "src/ui/ui_settings.hpp"

class SplatSettings;

class SplatPanel {
public:
    void render(bool& open, SplatSettings& settings, VelocityInputMode velMode);
    std::optional<std::vector<FluidSource>> takeSplats();

private:
    SplatWidget splatWidget_;
    std::optional<std::vector<FluidSource>> pendingSplats_;
};
