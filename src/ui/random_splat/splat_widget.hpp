#pragma once
#include <optional>
#include <random>
#include <vector>

#include "src/ui/ui_settings.hpp"

struct FluidSource;
struct SplatSettings;

class SplatWidget {
public:
    std::optional<std::vector<FluidSource>> render(SplatSettings& s, VelocityInputMode velMode);

private:
    std::mt19937 rng_{std::random_device{}()};

    std::vector<FluidSource> generate(const SplatSettings& s, VelocityInputMode velMode);
};
