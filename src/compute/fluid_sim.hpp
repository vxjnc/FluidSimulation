#pragma once
#include <vector>

#include <webgpu/webgpu-raii.hpp>

#include "src/compute/fluid_pipelines.hpp"
#include "src/compute/fluid_state.hpp"

namespace {
    inline wgpu::BindGroup makeBindGroup(wgpu::Device device, wgpu::raii::ComputePipeline& pipeline,
                                         std::initializer_list<std::pair<wgpu::Buffer, uint64_t>> buffers, std::string_view label) {
        wgpu::raii::BindGroupLayout layout = pipeline->getBindGroupLayout(0);

        std::vector<wgpu::BindGroupEntry> entries;
        uint32_t binding = 0;
        for (auto& [buf, size] : buffers) {
            wgpu::BindGroupEntry e{};
            e.binding = binding++;
            e.buffer = buf;
            e.size = size;
            entries.emplace_back(e);
        }

        wgpu::BindGroupDescriptor desc{};
        desc.layout = *layout;
        desc.entryCount = entries.size();
        desc.entries = entries.data();
        desc.label = wgpu::StringView(label);
        return device.createBindGroup(desc);
    }
}

class FluidSim {
public:
    void init(wgpu::Device device, wgpu::Queue queue, uint32_t width, uint32_t height) {
        device_ = device;
        queue_ = queue;
        state.init(device, queue, width, height);
        pipelines.init(device);
    }

    void resize(uint32_t width, uint32_t height) { state.resize(width, height); }

    void step(float dt) {
        queue_.writeBuffer(*state.dtBuffer, 0, &dt, sizeof(dt));

        wgpu::raii::CommandEncoder enc = device_.createCommandEncoder({});

        uint32_t W = (state.width + 7) / 8;
        uint32_t H = (state.height + 7) / 8;

        advect(enc, W, H);
        std::swap(state.velocity, state.velocity_next);

        advectDye(enc, W, H);
        std::swap(state.dye, state.dye_next);

        computeDivergence(enc, W, H);
        solvePressure(enc, W, H);

        subtractGradient(enc, W, H);
        std::swap(state.velocity, state.velocity_next);

        applyBoundary(enc, W, H);

        wgpu::raii::CommandBuffer cmd = enc->finish({});
        queue_.submit(1, &*cmd);

        injectSource();
    }

    void inject(float x, float y, float vx, float vy, float radius = 10.0f) {
        struct InjectParams {
            float x, y, vx, vy, radius;
        };
        InjectParams p{x, y, vx, vy, radius};
        queue_.writeBuffer(*state.injectBuffer, 0, &p, sizeof(p));

        wgpu::raii::CommandEncoder enc = device_.createCommandEncoder({});
        uint32_t W = (state.width + 7) / 8;
        uint32_t H = (state.height + 7) / 8;

        {
            wgpu::raii::BindGroup bg = makeBindGroup(
                device_, pipelines.inject,
                {{*state.paramsBuffer, 8}, {*state.injectBuffer, sizeof(InjectParams)}, {*state.velocity, state.velocity->getSize()}},
                "InjectVelocityBindGroup");
            wgpu::raii::ComputePassEncoder pass = enc->beginComputePass({});
            pipelines.dispatch(pass, pipelines.inject, bg, W, H);
            pass->end();
        }
        {
            wgpu::raii::BindGroup bg =
                makeBindGroup(device_, pipelines.injectDye,
                              {{*state.paramsBuffer, 8}, {*state.injectBuffer, sizeof(InjectParams)}, {*state.dye, state.dye->getSize()}},
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
                if (x < 0 || y < 0 || x >= static_cast<int>(state.width) || y >= static_cast<int>(state.height)) {
                    continue;
                }
                uint32_t idx = static_cast<uint32_t>(y) * state.width + static_cast<uint32_t>(x);
                queue_.writeBuffer(*state.obstacles, idx * sizeof(uint32_t), &val, sizeof(uint32_t));
            }
        }
    }

    FluidState state;
    FluidPipelines pipelines;

private:
    wgpu::Device device_;
    wgpu::Queue queue_;

    void injectSource() {
        constexpr uint32_t r = 4;
        uint32_t cx = state.width / 2;
        uint32_t cy = state.height / 2;

        static std::vector<float> velocity_patch(r * 2 * 2);
        for (uint32_t i = 0; i < r * 2; ++i) {
            velocity_patch[i * 2 + 0] = 0.0f;
            velocity_patch[i * 2 + 1] = 100.0f;
        }
        static std::vector<float> dye_patch(r * 2, 1.0f);

        for (uint32_t y = cy - r; y < cy + r; ++y) {
            queue_.writeBuffer(*state.velocity, (y * state.width + cx - r) * sizeof(float) * 2, velocity_patch.data(),
                               velocity_patch.size() * sizeof(float));
            queue_.writeBuffer(*state.dye, (y * state.width + cx - r) * sizeof(float), dye_patch.data(), dye_patch.size() * sizeof(float));
        }
    }

    void advect(wgpu::raii::CommandEncoder& enc, uint32_t W, uint32_t H) {
        wgpu::raii::BindGroup bg = makeBindGroup(device_, pipelines.advect,
                                                 {{*state.paramsBuffer, 8},
                                                  {*state.dtBuffer, 4},
                                                  {*state.velocity, state.velocity->getSize()},
                                                  {*state.velocity_next, state.velocity_next->getSize()},
                                                  {*state.obstacles, state.obstacles->getSize()}},
                                                 "AdvectBindGroup");
        wgpu::raii::ComputePassEncoder pass = enc->beginComputePass();
        pipelines.dispatch(pass, pipelines.advect, bg, W, H);
        pass->end();
    }

    void advectDye(wgpu::raii::CommandEncoder& enc, uint32_t W, uint32_t H) {
        wgpu::raii::BindGroup bg = makeBindGroup(device_, pipelines.advectDye,
                                                 {{*state.paramsBuffer, 8},
                                                  {*state.dtBuffer, 4},
                                                  {*state.velocity, state.velocity->getSize()},
                                                  {*state.dye, state.dye->getSize()},
                                                  {*state.dye_next, state.dye_next->getSize()},
                                                  {*state.obstacles, state.obstacles->getSize()}},
                                                 "AdvectDyeBindGroup");
        wgpu::raii::ComputePassEncoder pass = enc->beginComputePass({});
        pipelines.dispatch(pass, pipelines.advectDye, bg, W, H);
        pass->end();
    }

    void computeDivergence(wgpu::raii::CommandEncoder& enc, uint32_t W, uint32_t H) {
        wgpu::raii::BindGroup bg = makeBindGroup(
            device_, pipelines.divergence,
            {{*state.paramsBuffer, 8}, {*state.velocity, state.velocity->getSize()}, {*state.divergence, state.divergence->getSize()}},
            "DivergenceBindGroup");
        wgpu::raii::ComputePassEncoder pass = enc->beginComputePass({});
        pipelines.dispatch(pass, pipelines.divergence, bg, W, H);
        pass->end();
    }

    void solvePressure(wgpu::raii::CommandEncoder& enc, uint32_t W, uint32_t H) {
        wgpu::raii::BindGroup bg0 = makeBindGroup(device_, pipelines.pressure,
                                                  {{*state.paramsBuffer, 8},
                                                   {*state.pressure, state.pressure->getSize()},
                                                   {*state.divergence, state.divergence->getSize()},
                                                   {*state.pressure_next, state.pressure_next->getSize()}},
                                                  "SolvePressureBindGroup0");
        wgpu::raii::BindGroup bg1 = makeBindGroup(device_, pipelines.pressure,
                                                  {{*state.paramsBuffer, 8},
                                                   {*state.pressure_next, state.pressure_next->getSize()},
                                                   {*state.divergence, state.divergence->getSize()},
                                                   {*state.pressure, state.pressure->getSize()}},
                                                  "SolvePressureBindGroup1");

        wgpu::raii::ComputePassEncoder pass = enc->beginComputePass({});
        pass->setPipeline(*pipelines.pressure);
        for (int i = 0; i < 30; ++i) {
            pass->setBindGroup(0, i % 2 == 0 ? *bg0 : *bg1, 0, nullptr);
            pass->dispatchWorkgroups(W, H, 1);
        }
        pass->end();
    }

    void subtractGradient(wgpu::raii::CommandEncoder& enc, uint32_t W, uint32_t H) {
        wgpu::raii::BindGroup bg = makeBindGroup(device_, pipelines.subtract,
                                                 {{*state.paramsBuffer, 8},
                                                  {*state.pressure, state.pressure->getSize()},
                                                  {*state.velocity, state.velocity->getSize()},
                                                  {*state.velocity_next, state.velocity_next->getSize()},
                                                  {*state.obstacles, state.obstacles->getSize()}},
                                                 "SubtractGradientBindGroup");
        wgpu::raii::ComputePassEncoder pass = enc->beginComputePass({});
        pipelines.dispatch(pass, pipelines.subtract, bg, W, H);
        pass->end();
    }

    void applyBoundary(wgpu::raii::CommandEncoder& enc, uint32_t W, uint32_t H) {
        wgpu::raii::BindGroup bg = makeBindGroup(
            device_, pipelines.boundary,
            {{*state.paramsBuffer, 8}, {*state.velocity, state.velocity->getSize()}, {*state.obstacles, state.obstacles->getSize()}},
            "ApplyBoundaryBindGroup");
        wgpu::raii::ComputePassEncoder pass = enc->beginComputePass({});
        pipelines.dispatch(pass, pipelines.boundary, bg, W, H);
        pass->end();
    }
};
