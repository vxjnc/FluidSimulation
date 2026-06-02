#pragma once
#include <ranges>
#include <vector>

#include <webgpu/webgpu-raii.hpp>

#include "src/compute/fluid_pipelines.hpp"
#include "src/compute/fluid_source.hpp"
#include "src/compute/fluid_state.hpp"
#include "src/compute/resample_pipelines.hpp"
#include "src/compute/wgpu_helper.hpp"

class FluidSim {
public:
    void init(wgpu::Device device, wgpu::Queue queue, uint32_t width, uint32_t height) {
        device_ = device;
        queue_ = queue;
        state.init(device, queue, width, height);
        pipelines.init(device);
        resamplePipelines.init(device);
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
    }

    void setDt(float dt) { queue_.writeBuffer(*state.dtBuffer, 0, &dt, sizeof(dt)); }

    void step() {
        injectSource();

        wgpu::raii::CommandEncoder enc = device_.createCommandEncoder({});

        uint32_t W = (state.width + 7) / 8;
        uint32_t H = (state.height + 7) / 8;

        advect(enc, W, H);
        advectDye(enc, W, H);

        std::swap(state.velocity, state.velocity_next);
        std::swap(state.dye, state.dye_next);

        computeDivergence(enc, W, H);
        solvePressure(enc, W, H);

        subtractGradient(enc, W, H);
        std::swap(state.velocity, state.velocity_next);

        applyBoundary(enc, W, H);

        wgpu::raii::CommandBuffer cmd = enc->finish({});
        queue_.submit(1, &*cmd);
    }

    void inject(float x, float y, float vx, float vy, float radius) {
        struct InjectParams {
            float x, y, vx, vy, radius;
        };
        InjectParams p{x, y, vx, vy, radius};
        queue_.writeBuffer(*state.injectBuffer, 0, &p, sizeof(p));

        wgpu::raii::CommandEncoder enc = device_.createCommandEncoder({});
        uint32_t W = (state.width + 7) / 8;
        uint32_t H = (state.height + 7) / 8;

        {
            wgpu::raii::BindGroup bg = WGPUHelper::makeBindGroup(
                device_, pipelines.inject, {*state.paramsBuffer, *state.injectBuffer, *state.velocity},
                "InjectVelocityBindGroup");
            wgpu::raii::ComputePassEncoder pass = enc->beginComputePass({});
            pipelines.dispatch(pass, pipelines.inject, bg, W, H);
            pass->end();
        }
        {
            wgpu::raii::BindGroup bg = WGPUHelper::makeBindGroup(
                device_, pipelines.injectDye, {*state.paramsBuffer, *state.injectBuffer, *state.dye},
                "InjectDyeBindGroup");
            wgpu::raii::ComputePassEncoder pass = enc->beginComputePass({});
            pipelines.dispatch(pass, pipelines.injectDye, bg, W, H);
            pass->end();
        }

        wgpu::raii::CommandBuffer cmd = enc->finish({});
        queue_.submit(1, &*cmd);
    }

    void paintObstacle(uint32_t cx, uint32_t cy, uint32_t radius, bool erase = false) {
        uint32_t val = erase ? 0u : 1u;
        int r = static_cast<int>(radius);

        for (int dy = -r; dy <= r; ++dy) {
            for (int dx = -r; dx <= r; ++dx) {
                if (dx * dx + dy * dy > r * r) {
                    continue;
                }
                int x = static_cast<int>(cx) + dx;
                int y = static_cast<int>(cy) + dy;
                if (x < 0 || y < 0 || x >= static_cast<int>(state.width) ||
                    y >= static_cast<int>(state.height)) {
                    continue;
                }
                uint32_t idx = static_cast<uint32_t>(y) * state.width + static_cast<uint32_t>(x);
                queue_.writeBuffer(*state.obstacles, idx * sizeof(uint32_t), &val, sizeof(uint32_t));
            }
        }
    }

    std::vector<FluidSource> sources;

    FluidState state;
    FluidPipelines pipelines;
    ResamplePipelines resamplePipelines;

private:
    wgpu::Device device_;
    wgpu::Queue queue_;

    void injectSource() {
        for (const FluidSource& s :
             sources | std::views::filter([](const FluidSource& s) { return s.active; })) {
            inject(s.x, s.y, s.vx, s.vy, s.radius);
        }
    }

    void advect(wgpu::raii::CommandEncoder& enc, uint32_t W, uint32_t H) {
        wgpu::raii::BindGroup bg = WGPUHelper::makeBindGroup(
            device_, pipelines.advect,
            {*state.paramsBuffer, *state.dtBuffer, *state.velocity, *state.velocity_next, *state.obstacles},
            "AdvectBindGroup");
        wgpu::raii::ComputePassEncoder pass = enc->beginComputePass();
        pipelines.dispatch(pass, pipelines.advect, bg, W, H);
        pass->end();
    }

    void advectDye(wgpu::raii::CommandEncoder& enc, uint32_t W, uint32_t H) {
        wgpu::raii::BindGroup bg =
            WGPUHelper::makeBindGroup(device_, pipelines.advectDye,
                                      {*state.paramsBuffer, *state.dtBuffer, *state.velocity, *state.dye,
                                       *state.dye_next, *state.obstacles},
                                      "AdvectDyeBindGroup");
        wgpu::raii::ComputePassEncoder pass = enc->beginComputePass({});
        pipelines.dispatch(pass, pipelines.advectDye, bg, W, H);
        pass->end();
    }

    void computeDivergence(wgpu::raii::CommandEncoder& enc, uint32_t W, uint32_t H) {
        wgpu::raii::BindGroup bg = WGPUHelper::makeBindGroup(
            device_, pipelines.divergence,
            {*state.paramsBuffer, *state.velocity, *state.divergence, *state.obstacles},
            "DivergenceBindGroup");
        wgpu::raii::ComputePassEncoder pass = enc->beginComputePass({});
        pipelines.dispatch(pass, pipelines.divergence, bg, W, H);
        pass->end();
    }

    void solvePressure(wgpu::raii::CommandEncoder& enc, uint32_t W, uint32_t H) {
        wgpu::raii::BindGroup bg0 = WGPUHelper::makeBindGroup(
            device_, pipelines.pressure,
            {*state.paramsBuffer, *state.pressure, *state.divergence, *state.pressure_next, *state.obstacles},
            "SolvePressureBindGroup0");
        wgpu::raii::BindGroup bg1 = WGPUHelper::makeBindGroup(
            device_, pipelines.pressure,
            {*state.paramsBuffer, *state.pressure_next, *state.divergence, *state.pressure, *state.obstacles},
            "SolvePressureBindGroup1");

        constexpr size_t ITERS = 40;
        static_assert(ITERS % 2 == 0, "pressure iterations must be even");
        for (size_t i = 0; i < ITERS; ++i) {
            wgpu::raii::ComputePassEncoder pass = enc->beginComputePass({});
            pass->setPipeline(*pipelines.pressure);
            pass->setBindGroup(0, i % 2 ? *bg1 : *bg0, 0, nullptr);
            pass->dispatchWorkgroups(W, H, 1);
            pass->end();
        }
    }

    void subtractGradient(wgpu::raii::CommandEncoder& enc, uint32_t W, uint32_t H) {
        wgpu::raii::BindGroup bg = WGPUHelper::makeBindGroup(
            device_, pipelines.subtract,
            {*state.paramsBuffer, *state.pressure, *state.velocity, *state.velocity_next, *state.obstacles},
            "SubtractGradientBindGroup");
        wgpu::raii::ComputePassEncoder pass = enc->beginComputePass({});
        pipelines.dispatch(pass, pipelines.subtract, bg, W, H);
        pass->end();
    }

    void applyBoundary(wgpu::raii::CommandEncoder& enc, uint32_t W, uint32_t H) {
        wgpu::raii::BindGroup bg = WGPUHelper::makeBindGroup(
            device_, pipelines.boundary, {*state.paramsBuffer, *state.velocity, *state.obstacles},
            "ApplyBoundaryBindGroup");
        wgpu::raii::ComputePassEncoder pass = enc->beginComputePass({});
        pipelines.dispatch(pass, pipelines.boundary, bg, W, H);
        pass->end();
    }
};
