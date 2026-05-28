#pragma once
#include <string_view>
#include <vector>

#include <webgpu/webgpu-raii.hpp>

#include "src/compute/fluid_pipelines.hpp"
#include "src/compute/fluid_state.hpp"
#include "src/wgpu_context.hpp"

class FluidSim {
public:
    void init(uint32_t width, uint32_t height) {
        state.init(width, height);
        pipelines.init();
    }

    void resize(uint32_t width, uint32_t height) { state.resize(width, height); }

    void step(float dt) {
        WGPUContext& ctx = WGPUContext::instance();

        ctx.queue().writeBuffer(*state.dtBuffer, 0, &dt, sizeof(dt));

        wgpu::raii::CommandEncoder enc = ctx.device().createCommandEncoder({});

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
        ctx.queue().submit(1, &*cmd);

        injectSource();
    }

    void inject(float x, float y, float vx, float vy, float radius = 10.0f) {
        WGPUContext& ctx = WGPUContext::instance();
        wgpu::raii::CommandEncoder enc = ctx.device().createCommandEncoder({});

        struct InjectParams {
            float x, y, vx, vy, radius;
        };
        InjectParams p{x, y, vx, vy, radius};
        ctx.queue().writeBuffer(*state.injectBuffer, 0, &p, sizeof(p));

        uint32_t W = (state.width + 7) / 8;
        uint32_t H = (state.height + 7) / 8;

        // Inject velocity
        {
            wgpu::raii::BindGroup bg = ctx.makeBindGroup(pipelines.inject,
                                                         {
                                                             {*state.paramsBuffer, 8},
                                                             {*state.injectBuffer, sizeof(InjectParams)},
                                                             {*state.velocity, state.velocity->getSize()},
                                                         },
                                                         "InjectVelocityBindGroup");

            wgpu::raii::ComputePassEncoder pass = enc->beginComputePass({});
            pipelines.dispatch(pass, pipelines.inject, bg, W, H);
            pass->end();
        }

        // Inject dye
        {
            wgpu::raii::BindGroup bg = ctx.makeBindGroup(pipelines.injectDye,
                                                         {
                                                             {*state.paramsBuffer, 8},
                                                             {*state.injectBuffer, sizeof(InjectParams)},
                                                             {*state.dye, state.dye->getSize()},
                                                         },
                                                         "InjectDyeBindGroup");

            wgpu::raii::ComputePassEncoder pass = enc->beginComputePass({});
            pipelines.dispatch(pass, pipelines.injectDye, bg, W, H);
            pass->end();
        }

        wgpu::raii::CommandBuffer cmd = enc->finish({});
        ctx.queue().submit(1, &*cmd);
    }

    FluidState state;
    FluidPipelines pipelines;

private:
    void injectSource() {
        WGPUContext& ctx = WGPUContext::instance();
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
            uint64_t vel_offset = (y * state.width + cx - r) * sizeof(float) * 2;
            ctx.queue().writeBuffer(*state.velocity, vel_offset, velocity_patch.data(), velocity_patch.size() * sizeof(float));

            uint64_t dye_offset = (y * state.width + cx - r) * sizeof(float);
            ctx.queue().writeBuffer(*state.dye, dye_offset, dye_patch.data(), dye_patch.size() * sizeof(float));
        }
    }

    void advect(wgpu::raii::CommandEncoder& enc, uint32_t W, uint32_t H) {
        wgpu::raii::BindGroup bg = WGPUContext::instance().makeBindGroup(pipelines.advect,
                                                                         {
                                                                             {*state.paramsBuffer, 8},
                                                                             {*state.dtBuffer, 4},
                                                                             {*state.velocity, state.velocity->getSize()},
                                                                             {*state.velocity_next, state.velocity_next->getSize()},
                                                                         },
                                                                         "AdvectBindGroup");
        wgpu::raii::ComputePassEncoder pass = enc->beginComputePass();
        pipelines.dispatch(pass, pipelines.advect, bg, W, H);
        pass->end();
    }

    void advectDye(wgpu::raii::CommandEncoder& enc, uint32_t W, uint32_t H) {
        wgpu::raii::BindGroup bg = WGPUContext::instance().makeBindGroup(pipelines.advectDye,
                                                                         {
                                                                             {*state.paramsBuffer, 8},
                                                                             {*state.dtBuffer, 4},
                                                                             {*state.velocity, state.velocity->getSize()},
                                                                             {*state.dye, state.dye->getSize()},
                                                                             {*state.dye_next, state.dye_next->getSize()},
                                                                         },
                                                                         "AdvectDyeBindGroup");
        wgpu::raii::ComputePassEncoder pass = enc->beginComputePass({});
        pipelines.dispatch(pass, pipelines.advectDye, bg, W, H);
        pass->end();
    }

    void computeDivergence(wgpu::raii::CommandEncoder& enc, uint32_t W, uint32_t H) {
        wgpu::raii::BindGroup bg = WGPUContext::instance().makeBindGroup(pipelines.divergence,
                                                                         {
                                                                             {*state.paramsBuffer, 8},
                                                                             {*state.velocity, state.velocity->getSize()},
                                                                             {*state.divergence, state.divergence->getSize()},
                                                                         },
                                                                         "DivergenceBindGroup");
        wgpu::raii::ComputePassEncoder pass = enc->beginComputePass({});
        pipelines.dispatch(pass, pipelines.divergence, bg, W, H);
        pass->end();
    }

    void solvePressure(wgpu::raii::CommandEncoder& enc, uint32_t W, uint32_t H) {
        wgpu::raii::BindGroup bg0 = WGPUContext::instance().makeBindGroup(pipelines.pressure,
                                                                          {
                                                                              {*state.paramsBuffer, 8},
                                                                              {*state.pressure, state.pressure->getSize()},
                                                                              {*state.divergence, state.divergence->getSize()},
                                                                              {*state.pressure_next, state.pressure_next->getSize()},
                                                                          },
                                                                          "SolvePressureBindGroup0");
        wgpu::raii::BindGroup bg1 = WGPUContext::instance().makeBindGroup(pipelines.pressure,
                                                                          {
                                                                              {*state.paramsBuffer, 8},
                                                                              {*state.pressure_next, state.pressure_next->getSize()},
                                                                              {*state.divergence, state.divergence->getSize()},
                                                                              {*state.pressure, state.pressure->getSize()},
                                                                          },
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
        wgpu::raii::BindGroup bg = WGPUContext::instance().makeBindGroup(pipelines.subtract,
                                                                         {
                                                                             {*state.paramsBuffer, 8},
                                                                             {*state.pressure, state.pressure->getSize()},
                                                                             {*state.velocity, state.velocity->getSize()},
                                                                             {*state.velocity_next, state.velocity_next->getSize()},
                                                                         },
                                                                         "SubtractGradientBindGroup");
        wgpu::raii::ComputePassEncoder pass = enc->beginComputePass({});
        pipelines.dispatch(pass, pipelines.subtract, bg, W, H);
        pass->end();
    }

    void applyBoundary(wgpu::raii::CommandEncoder& enc, uint32_t W, uint32_t H) {
        wgpu::raii::BindGroup bg = WGPUContext::instance().makeBindGroup(pipelines.boundary,
                                                                         {
                                                                             {*state.paramsBuffer, 8},
                                                                             {*state.velocity, state.velocity->getSize()},
                                                                         },
                                                                         "ApplyBoundaryBindGroup");
        wgpu::raii::ComputePassEncoder pass = enc->beginComputePass({});
        pipelines.dispatch(pass, pipelines.boundary, bg, W, H);
        pass->end();
    }
};
