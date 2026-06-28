#pragma once
#include <filesystem>
#include <span>
#include <vector>

class FluidSim;
struct AppSettings;
struct FluidSource;

namespace SaveManager {
    void save(const std::filesystem::path& path, FluidSim& sim, std::span<const FluidSource> sources,
              const AppSettings& settings);

    void load(const std::filesystem::path& path, FluidSim& sim, std::vector<FluidSource>& sources,
              AppSettings& settings);
}
