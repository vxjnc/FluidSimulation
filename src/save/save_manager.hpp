#pragma once
#include <filesystem>
#include <ranges>
#include <span>

#include "src/app_settings.hpp"
#include "src/capture/gpu_readback.hpp"
#include "src/compute/fluid_sim.hpp"
#include "src/save/fsim_file.hpp"
#include "src/save/fsim_serializer.hpp"

namespace SaveManager {

    inline void save(const std::filesystem::path& path, FluidSim& sim, std::span<const FluidSource> sources,
                     const AppSettings& settings) {
        FsimFile file;
        file.width = sim.state.width;
        file.height = sim.state.height;
        file.dyeWidth = sim.state.dye_width;
        file.dyeHeight = sim.state.dye_height;
        file.dt = settings.dt;
        file.velDissipation = settings.velDissipation;
        file.dyeDissipation = settings.dyeDissipation;
        file.curlStrength = settings.curlStrength;
        file.simScale = settings.simScale;
        file.dyeScale = settings.dyeScale;

        file.sources =
            std::ranges::to<std::vector>(sources | std::views::transform(&FsimFluidSource::fromFluidSource));

        auto state = std::make_shared<FsimFile>(std::move(file));
        auto pending = std::make_shared<int>(3);

        auto tryFinish = [state, pending, path]() {
            if (--(*pending) == 0) {
                FsimSerializer::save(path, *state);
            }
        };

        uint64_t velSize = sim.state.width * sim.state.height * 2 * sizeof(float);
        uint64_t dyeSize = sim.state.dye_width * sim.state.dye_height * 4 * sizeof(float);
        uint64_t obsSize = sim.state.width * sim.state.height * sizeof(uint32_t);

        GpuReadback::request<float>(*sim.state.velocity, velSize,
                                    [state, tryFinish](std::span<const float> data) {
                                        state->velocity.assign(data.begin(), data.end());
                                        tryFinish();
                                    });

        GpuReadback::request<float>(*sim.state.dye, dyeSize, [state, tryFinish](std::span<const float> data) {
            state->dye.assign(data.begin(), data.end());
            tryFinish();
        });

        GpuReadback::request<uint32_t>(*sim.state.obstacles, obsSize,
                                       [state, tryFinish](std::span<const uint32_t> data) {
                                           state->obstacles.assign(data.begin(), data.end());
                                           tryFinish();
                                       });
    }

    inline void load(const std::filesystem::path& path, FluidSim& sim, std::vector<FluidSource>& sources,
                     AppSettings& settings) {
        FsimFile file = FsimSerializer::load(path);

        settings.dt = file.dt;
        settings.velDissipation = file.velDissipation;
        settings.dyeDissipation = file.dyeDissipation;
        settings.curlStrength = file.curlStrength;

        sim.resize(file.width, file.height, file.dyeWidth, file.dyeHeight);

        sources = std::ranges::to<std::vector>(file.sources |
                                               std::views::transform(&FsimFluidSource::toFluidSource));

        WGPUContext& ctx = WGPUContext::instance();
        ctx.queue().writeBuffer(*sim.state.velocity, 0, file.velocity.data(),
                                file.velocity.size() * sizeof(float));
        ctx.queue().writeBuffer(*sim.getCurrentDye(), 0, file.dye.data(), file.dye.size() * sizeof(float));
        ctx.queue().writeBuffer(*sim.state.obstacles, 0, file.obstacles.data(),
                                file.obstacles.size() * sizeof(uint32_t));
    }
}
