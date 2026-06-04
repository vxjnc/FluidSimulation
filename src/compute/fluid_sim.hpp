#pragma once
#include <ranges>
#include <vector>

#include <webgpu/webgpu-raii.hpp>

#include "src/compute/fluid_pipelines.hpp"
#include "src/compute/fluid_source.hpp"
#include "src/compute/fluid_state.hpp"
#include "src/compute/resample_pipelines.hpp"
#include "src/compute/wgpu_helper.hpp"
#include "webgpu/webgpu.hpp"

class FluidSim {
public:
    void init(wgpu::Device device, wgpu::Queue queue, uint32_t width, uint32_t height) {
        device_ = device;
        queue_ = queue;
        state.init(device, queue, width, height);
        pipelines.init(device);
        resamplePipelines.init(device);
        initBindGroups();
    }

    void resize(uint32_t w, uint32_t h) {
        if (w != state.width || h != state.height) {
            state.resize(w, h);
        }
    }

    void resizeWithResample(uint32_t w, uint32_t h) {
        struct ResampleParams {
            uint32_t src_w, src_h, dst_w, dst_h;
        };
        ResampleParams rp{state.width, state.height, w, h};
        wgpu::BufferDescriptor pbDesc{};
        pbDesc.size = sizeof(rp);
        pbDesc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
        pbDesc.label = wgpu::StringView("ResampleParams");
        wgpu::raii::Buffer paramsBuffer = device_.createBuffer(pbDesc);
        queue_.writeBuffer(*paramsBuffer, 0, &rp, sizeof(rp));

        auto makeBuf = [&](size_t elementSize, std::string_view label) {
            wgpu::BufferDescriptor desc{};
            desc.size = w * h * elementSize;
            desc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
            desc.label = wgpu::StringView(label);
            return device_.createBuffer(desc);
        };

        wgpu::raii::Buffer new_velocity = makeBuf(2 * sizeof(float), "velocity_new");
        wgpu::raii::Buffer new_pressure = makeBuf(sizeof(float), "pressure_new");
        wgpu::raii::Buffer new_dye = makeBuf(sizeof(float), "dye_new");
        wgpu::raii::Buffer new_obstacles = makeBuf(sizeof(uint32_t), "obstacles_new");

        uint32_t W = (w + 7) / 8;
        uint32_t H = (h + 7) / 8;

        wgpu::raii::CommandEncoder enc = device_.createCommandEncoder({});
        wgpu::raii::ComputePassEncoder pass = enc->beginComputePass({});

        auto dispatch = [&](wgpu::raii::ComputePipeline& pipeline, wgpu::Buffer src, wgpu::Buffer dst) {
            wgpu::raii::BindGroup bg =
                WGPUHelper::makeBindGroup(device_, pipeline, {*paramsBuffer, src, dst}, "ResampleBG");
            FluidPipelines::dispatch(pass, pipeline, bg, W, H);
        };

        dispatch(resamplePipelines.vec2f, *state.velocity, *new_velocity);
        dispatch(resamplePipelines.f32, *state.pressure, *new_pressure);
        dispatch(resamplePipelines.f32, *state.dye, *new_dye);
        dispatch(resamplePipelines.u32, *state.obstacles, *new_obstacles);

        pass->end();
        wgpu::raii::CommandBuffer cmd = enc->finish({});
        queue_.submit(1, &*cmd);

        state.width = w;
        state.height = h;
        state.velocity = std::move(new_velocity);
        state.pressure = std::move(new_pressure);
        state.dye = std::move(new_dye);
        state.obstacles = std::move(new_obstacles);

        state.velocity_next = makeBuf(2 * sizeof(float), "velocity_next");
        state.pressure_next = makeBuf(sizeof(float), "pressure_next");
        state.divergence = makeBuf(sizeof(float), "divergence");
        state.dye_next = makeBuf(sizeof(float), "dye_next");

        uint32_t params[2] = {w, h};
        queue_.writeBuffer(*state.paramsBuffer, 0, params, sizeof(params));

        initBindGroups();
    }

    void setDt(float dt) { queue_.writeBuffer(*state.dtBuffer, 0, &dt, sizeof(dt)); }

    void step() {
        injectSource();

        wgpu::raii::CommandEncoder enc = device_.createCommandEncoder({});

        uint32_t W = (state.width + 7) / 8;
        uint32_t H = (state.height + 7) / 8;

        advect(enc, W, H);
        std::swap(state.velocity, state.velocity_next);

        computeDivergence(enc, W, H);
        solvePressure(enc, W, H);

        subtractGradient(enc, W, H);
        std::swap(state.velocity, state.velocity_next);

        applyBoundary(enc, W, H);

        advectDye(enc, W, H);
        std::swap(state.dye, state.dye_next);

        wgpu::raii::CommandBuffer cmd = enc->finish({});
        queue_.submit(1, &*cmd);
        ++frameIndex;
    }

    void inject(const FluidSource& source) {
        struct InjectParams {
            float x, y;
            float vx, vy;
            float radius;
            uint32_t mode_mask;
            uint32_t form;
        };

        InjectParams p{source.x,
                       source.y,
                       source.vx,
                       source.vy,
                       source.radius,
                       static_cast<uint32_t>(source.mode_mask),
                       static_cast<uint32_t>(source.form)};
        queue_.writeBuffer(*state.injectBuffer, 0, &p, sizeof(p));

        wgpu::raii::CommandEncoder enc = device_.createCommandEncoder({});
        uint32_t W = (state.width + 7) / 8;
        uint32_t H = (state.height + 7) / 8;

        wgpu::raii::BindGroup bg = WGPUHelper::makeBindGroup(
            device_, pipelines.inject,
            {*state.paramsBuffer, *state.injectBuffer, *state.velocity, *state.dye}, "InjectBindGroup");

        wgpu::raii::ComputePassEncoder pass = enc->beginComputePass({});
        pipelines.dispatch(pass, pipelines.inject, bg, W, H);
        pass->end();

        wgpu::raii::CommandBuffer cmd = enc->finish({});
        queue_.submit(1, &*cmd);
    }

    void paintObstacle(uint32_t cx, uint32_t cy, uint32_t radius, bool erase = false) {
        struct FillCircleParams {
            uint32_t cx, cy;
            uint32_t radius;
            uint32_t val;
            uint32_t width, height;
        };

        FillCircleParams p{cx, cy, radius, erase ? 0u : 1u, state.width, state.height};
        queue_.writeBuffer(*state.fillCircleBuffer, 0, &p, sizeof(p));

        wgpu::raii::CommandEncoder enc = device_.createCommandEncoder({});
        uint32_t W = (state.width + 7) / 8;
        uint32_t H = (state.height + 7) / 8;

        wgpu::raii::BindGroup bg =
            WGPUHelper::makeBindGroup(device_, pipelines.fillCircle,
                                      {*state.fillCircleBuffer, *state.obstacles}, "FillCircleBindGroup");

        wgpu::raii::ComputePassEncoder pass = enc->beginComputePass({});
        pipelines.dispatch(pass, pipelines.fillCircle, bg, W, H);
        pass->end();

        wgpu::raii::CommandBuffer cmd = enc->finish({});
        queue_.submit(1, &*cmd);
    }

    std::vector<FluidSource> sources;

    FluidState state;
    FluidPipelines pipelines;
    ResamplePipelines resamplePipelines;

private:
    size_t frameIndex = 0;

    wgpu::Device device_;
    wgpu::Queue queue_;

    wgpu::raii::BindGroup bg_advect;
    wgpu::raii::BindGroup bg_divergence;
    wgpu::raii::BindGroup bg_pressure0;
    wgpu::raii::BindGroup bg_pressure1;
    wgpu::raii::BindGroup bg_subtract;
    wgpu::raii::BindGroup bg_boundary;
    wgpu::raii::BindGroup bg_advect_dye0; // dye → dye_next
    wgpu::raii::BindGroup bg_advect_dye1; // dye_next → dye

    void initBindGroups() {
        bg_advect = WGPUHelper::makeBindGroup(
            device_, pipelines.advect,
            {*state.paramsBuffer, *state.dtBuffer, *state.velocity, *state.velocity_next, *state.obstacles},
            "AdvectBindGroup");
        bg_divergence = WGPUHelper::makeBindGroup(
            device_, pipelines.divergence,
            {*state.paramsBuffer, *state.velocity_next, *state.divergence, *state.obstacles},
            "DivergenceBindGroup");
        bg_pressure0 = WGPUHelper::makeBindGroup(
            device_, pipelines.pressure,
            {*state.paramsBuffer, *state.pressure, *state.divergence, *state.pressure_next, *state.obstacles},
            "SolvePressureBindGroup0");
        bg_pressure1 = WGPUHelper::makeBindGroup(
            device_, pipelines.pressure,
            {*state.paramsBuffer, *state.pressure_next, *state.divergence, *state.pressure, *state.obstacles},
            "SolvePressureBindGroup1");
        bg_subtract = WGPUHelper::makeBindGroup(
            device_, pipelines.subtract,
            {*state.paramsBuffer, *state.pressure, *state.velocity_next, *state.velocity, *state.obstacles},
            "SubtractGradientBindGroup");
        bg_boundary = WGPUHelper::makeBindGroup(device_, pipelines.boundary,
                                                {*state.paramsBuffer, *state.velocity, *state.obstacles},
                                                "ApplyBoundaryBindGroup");
        bg_advect_dye0 = WGPUHelper::makeBindGroup(device_, pipelines.advectDye,
                                                   {*state.paramsBuffer, *state.dtBuffer, *state.velocity,
                                                    *state.dye, *state.dye_next, *state.obstacles},
                                                   "AdvectDyeBindGroup0");
        bg_advect_dye1 = WGPUHelper::makeBindGroup(device_, pipelines.advectDye,
                                                   {*state.paramsBuffer, *state.dtBuffer, *state.velocity,
                                                    *state.dye_next, *state.dye, *state.obstacles},
                                                   "AdvectDyeBindGroup1");
    }

    void injectSource() {
        for (const FluidSource& s : sources | std::views::filter(&FluidSource::active)) {
            inject(s);
        }
    }

    void advect(wgpu::raii::CommandEncoder& enc, uint32_t W, uint32_t H) {
        wgpu::ComputePassDescriptor passDesc{};
        passDesc.label = wgpu::StringView("AdvectPass");
        wgpu::raii::ComputePassEncoder pass = enc->beginComputePass(passDesc);
        pipelines.dispatch(pass, pipelines.advect, bg_advect, W, H);
        pass->end();
    }

    void advectDye(wgpu::raii::CommandEncoder& enc, uint32_t W, uint32_t H) {
        wgpu::ComputePassDescriptor passDesc{};
        passDesc.label = wgpu::StringView("AdvectDyePass");
        wgpu::raii::ComputePassEncoder pass = enc->beginComputePass(passDesc);
        pipelines.dispatch(pass, pipelines.advectDye, frameIndex % 2 ? bg_advect_dye1 : bg_advect_dye0, W, H);
        pass->end();
    }

    void computeDivergence(wgpu::raii::CommandEncoder& enc, uint32_t W, uint32_t H) {
        wgpu::ComputePassDescriptor passDesc{};
        passDesc.label = wgpu::StringView("DivergencePass");
        wgpu::raii::ComputePassEncoder pass = enc->beginComputePass(passDesc);
        pipelines.dispatch(pass, pipelines.divergence, bg_divergence, W, H);
        pass->end();
    }

    void solvePressure(wgpu::raii::CommandEncoder& enc, uint32_t W, uint32_t H) {
        constexpr size_t ITERS = 30;
        static_assert(ITERS % 2 == 0, "pressure iterations must be even");
        for (size_t i = 0; i < ITERS; ++i) {
            wgpu::ComputePassDescriptor passDesc{};
            passDesc.label = wgpu::StringView("SolvePressurePass");
            wgpu::raii::ComputePassEncoder pass = enc->beginComputePass(passDesc);
            pass->setPipeline(*pipelines.pressure);
            pass->setBindGroup(0, i % 2 ? *bg_pressure1 : *bg_pressure0, 0, nullptr);
            pass->dispatchWorkgroups(W, H, 1);
            pass->end();
        }
    }

    void subtractGradient(wgpu::raii::CommandEncoder& enc, uint32_t W, uint32_t H) {
        wgpu::ComputePassDescriptor passDesc{};
        passDesc.label = wgpu::StringView("SubtractGradientPass");
        wgpu::raii::ComputePassEncoder pass = enc->beginComputePass(passDesc);
        pipelines.dispatch(pass, pipelines.subtract, bg_subtract, W, H);
        pass->end();
    }

    void applyBoundary(wgpu::raii::CommandEncoder& enc, uint32_t W, uint32_t H) {
        wgpu::ComputePassDescriptor passDesc{};
        passDesc.label = wgpu::StringView("ApplyBoundaryPass");
        wgpu::raii::ComputePassEncoder pass = enc->beginComputePass(passDesc);
        pipelines.dispatch(pass, pipelines.boundary, bg_boundary, W, H);
        pass->end();
    }
};
