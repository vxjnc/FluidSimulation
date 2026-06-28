#pragma once

#include <span>

#include <webgpu/webgpu-raii.hpp>

#include "src/compute/fluid_pipelines.hpp"
#include "src/compute/fluid_state.hpp"
#include "src/compute/gpu_profiler.hpp"
#include "src/compute/resample_pipelines.hpp"

struct FluidSource;

class FluidSim {
public:
    void init(wgpu::Device device, wgpu::Queue queue, uint32_t width, uint32_t height, uint32_t dye_width,
              uint32_t dye_height);

    void resize(uint32_t w, uint32_t h) { resize(w, h, state.dye_width, state.dye_height); }

    void resize(uint32_t w, uint32_t h, uint32_t dye_w, uint32_t dye_h);

    void resizeWithResample(uint32_t w, uint32_t h) {
        resizeWithResample(w, h, state.dye_width, state.dye_height);
    }
    void resizeWithResample(uint32_t w, uint32_t h, uint32_t dye_w, uint32_t dye_h);

    const wgpu::raii::Buffer& getCurrentDye() const { return frameIndex % 2 ? state.dye_next : state.dye; }

    void setDt(float dt);
    void setVelDissipation(float d);
    void setDyeDissipation(float d);
    void setCurlStrength(float s);

    void step();
    void step(wgpu::raii::CommandEncoder& enc);

    void inject(wgpu::raii::CommandEncoder& enc, std::span<const FluidSource> batch);

    void paintObstacle(wgpu::raii::CommandEncoder& enc, float cx, float cy, float radius, bool erase = false);

    FluidState state;
    FluidPipelines pipelines;
    ResamplePipelines resamplePipelines;
    GpuProfiler<> profiler;

private:
    size_t frameIndex = 0;

    wgpu::Device device_;
    wgpu::Queue queue_;

    wgpu::raii::BindGroup bg_advect;
    wgpu::raii::BindGroup bg_divergence;
    wgpu::raii::BindGroup bg_pressure0, bg_pressure1;
    wgpu::raii::BindGroup bg_subtract;
    wgpu::raii::BindGroup bg_boundary;
    wgpu::raii::BindGroup bg_advect_dye0, bg_advect_dye1;
    wgpu::raii::BindGroup bg_curl;
    wgpu::raii::BindGroup bg_vorticity;

    static FluidState::InjectParams toParams(const FluidSource& s);

    void initBindGroups();

    void injectBatch(wgpu::raii::CommandEncoder& enc, std::span<const FluidState::InjectParams> params);

    void advect(wgpu::raii::ComputePassEncoder& pass, uint32_t W, uint32_t H);
    void advectDye(wgpu::raii::ComputePassEncoder& pass, uint32_t W, uint32_t H);
    void computeDivergence(wgpu::raii::ComputePassEncoder& pass, uint32_t W, uint32_t H);
    void solvePressure(wgpu::raii::ComputePassEncoder& pass, uint32_t W, uint32_t H);
    void subtractGradient(wgpu::raii::ComputePassEncoder& pass, uint32_t W, uint32_t H);
    void applyBoundary(wgpu::raii::ComputePassEncoder& pass, uint32_t W, uint32_t H);
    void computeCurl(wgpu::raii::ComputePassEncoder& pass, uint32_t W, uint32_t H);
    void applyVorticity(wgpu::raii::ComputePassEncoder& pass, uint32_t W, uint32_t H);
};
