#pragma once

#include "src/compute/gpu_profiler.hpp"

class FluidSim;
class RenderSettings;

class Render {
    struct RenderParams {
        uint32_t width, height;
        uint32_t dye_width, dye_height;
        uint32_t mode, show_obstacles;
    };

public:
    void init(wgpu::Device device);

    void draw(const wgpu::raii::TextureView& targetView, FluidSim& fluid_sim, const RenderSettings& settings);
    void draw(wgpu::raii::CommandEncoder& enc, const wgpu::raii::TextureView& targetView, FluidSim& fluid_sim,
              const RenderSettings& settings);

    GpuProfiler<> profiler;

private:
    void initPipeline();

    wgpu::raii::BindGroupLayout bindGroupLayout;
    wgpu::raii::RenderPipeline pipeline;
    wgpu::raii::Buffer paramsBuffer;
};
