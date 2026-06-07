#pragma once

#include <ranges>
#include <span>

#include <webgpu/webgpu-raii.hpp>

#include "src/compute/fluid_pipelines.hpp"
#include "src/compute/fluid_source.hpp"
#include "src/compute/fluid_state.hpp"
#include "src/compute/resample_pipelines.hpp"
#include "src/compute/wgpu_helper.hpp"
#include "webgpu/webgpu.hpp"

class FluidSim {
public:
    void init(wgpu::Device device, wgpu::Queue queue, uint32_t width, uint32_t height, uint32_t dye_width,
              uint32_t dye_height) {
        device_ = device;
        queue_ = queue;
        state.init(device, queue, width, height, dye_width, dye_height);
        pipelines.init(device);
        resamplePipelines.init(device);
        initBindGroups();
    }

    void resize(uint32_t w, uint32_t h) { resize(w, h, state.dye_width, state.dye_height); }

    void resize(uint32_t w, uint32_t h, uint32_t dye_w, uint32_t dye_h) {
        bool changed = false;
        if (w != state.width || h != state.height) {
            state.resize(w, h);
            changed = true;
        }
        if (dye_w != state.dye_width || dye_h != state.dye_height) {
            state.resizeDye(dye_w, dye_h);
            changed = true;
        }

        if (changed) {
            initBindGroups();
        }
    }

    void resizeWithResample(uint32_t w, uint32_t h) {
        resizeWithResample(w, h, state.dye_width, state.dye_height);
    }

    void resizeWithResample(uint32_t w, uint32_t h, uint32_t dye_w, uint32_t dye_h) {
        if (w == state.width && h == state.height && dye_w == state.dye_width && dye_h == state.dye_height) {
            return;
        }

        struct ResampleParams {
            uint32_t src_w, src_h, dst_w, dst_h;
        };

        auto makeResampleBuf = [&](const ResampleParams& rp) {
            wgpu::BufferDescriptor pbDesc{};
            pbDesc.size = sizeof(rp);
            pbDesc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
            pbDesc.label = wgpu::StringView("ResampleParams");
            wgpu::raii::Buffer buf = device_.createBuffer(pbDesc);
            queue_.writeBuffer(*buf, 0, &rp, sizeof(rp));
            return buf;
        };

        auto makeBuf = [&](size_t w_, size_t h_, size_t elementSize, std::string_view label) {
            wgpu::BufferDescriptor desc{};
            desc.size = w_ * h_ * elementSize;
            desc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc;
            desc.label = wgpu::StringView(label);
            return device_.createBuffer(desc);
        };

        wgpu::raii::CommandEncoder enc = device_.createCommandEncoder({});
        wgpu::raii::ComputePassEncoder pass = enc->beginComputePass({});

        if (w != state.width || h != state.height) {
            wgpu::raii::Buffer physParams = makeResampleBuf({state.width, state.height, w, h});
            uint32_t W = (w + 15) / 16, H = (h + 15) / 16;

            wgpu::raii::Buffer new_velocity = makeBuf(w, h, 2 * sizeof(float), "velocity");
            wgpu::raii::Buffer new_pressure = makeBuf(w, h, sizeof(float), "pressure");
            wgpu::raii::Buffer new_obstacles = makeBuf(w, h, sizeof(uint32_t), "obstacles");

            auto dispatch = [&](wgpu::raii::ComputePipeline& pipeline, wgpu::Buffer src, wgpu::Buffer dst) {
                wgpu::raii::BindGroup bg =
                    WGPUHelper::makeBindGroup(device_, pipeline, {*physParams, src, dst}, "ResampleBG");
                FluidPipelines::dispatch(pass, pipeline, bg, W, H);
            };
            dispatch(resamplePipelines.vec2f, *state.velocity, *new_velocity);
            dispatch(resamplePipelines.f32, *state.pressure, *new_pressure);
            dispatch(resamplePipelines.u32, *state.obstacles, *new_obstacles);

            pass->end();
            wgpu::raii::CommandBuffer cmd = enc->finish({});
            queue_.submit(1, &*cmd);

            state.width = w;
            state.height = h;
            state.velocity = std::move(new_velocity);
            state.pressure = std::move(new_pressure);
            state.obstacles = std::move(new_obstacles);
            state.velocity_next = makeBuf(w, h, 2 * sizeof(float), "velocity_next");
            state.pressure_next = makeBuf(w, h, sizeof(float), "pressure_next");
            state.divergence = makeBuf(w, h, sizeof(float), "divergence");
            state.curl = makeBuf(w, h, sizeof(float), "curl");

            uint32_t phys[2] = {w, h};
            queue_.writeBuffer(*state.paramsBuffer, offsetof(FluidState::Params, width), phys, sizeof(phys));

            enc = device_.createCommandEncoder({});
            pass = enc->beginComputePass({});
        }

        if (dye_w != state.dye_width || dye_h != state.dye_height) {
            wgpu::raii::Buffer dyeParams = makeResampleBuf({state.dye_width, state.dye_height, dye_w, dye_h});
            uint32_t W = (dye_w + 15) / 16, H = (dye_h + 15) / 16;

            wgpu::raii::Buffer new_dye = makeBuf(dye_w, dye_h, 4 * sizeof(float), "dye");
            wgpu::raii::BindGroup bg = WGPUHelper::makeBindGroup(
                device_, resamplePipelines.vec4f, {*dyeParams, *state.dye, *new_dye}, "ResampleBG");
            FluidPipelines::dispatch(pass, resamplePipelines.vec4f, bg, W, H);

            pass->end();
            wgpu::raii::CommandBuffer cmd = enc->finish({});
            queue_.submit(1, &*cmd);

            state.dye_width = dye_w;
            state.dye_height = dye_h;
            state.dye = std::move(new_dye);
            state.dye_next = makeBuf(dye_w, dye_h, 4 * sizeof(float), "dye_next");

            uint32_t dye[2] = {dye_w, dye_h};
            queue_.writeBuffer(*state.paramsBuffer, offsetof(FluidState::Params, dye_width), dye,
                               sizeof(dye));
        }
        else {
            pass->end();
            wgpu::raii::CommandBuffer cmd = enc->finish({});
            queue_.submit(1, &*cmd);
        }

        initBindGroups();
    }

    const wgpu::raii::Buffer& getCurrentDye() const { return frameIndex % 2 ? state.dye_next : state.dye; }

    void setDt(float dt) {
        queue_.writeBuffer(*state.paramsBuffer, offsetof(FluidState::Params, dt), &dt, sizeof(dt));
    }
    void setVelDissipation(float d) {
        queue_.writeBuffer(*state.paramsBuffer, offsetof(FluidState::Params, vel_dissipation), &d, sizeof(d));
    }
    void setDyeDissipation(float d) {
        queue_.writeBuffer(*state.paramsBuffer, offsetof(FluidState::Params, dye_dissipation), &d, sizeof(d));
    }
    void setCurlStrength(float s) {
        queue_.writeBuffer(*state.paramsBuffer, offsetof(FluidState::Params, curl_strength), &s, sizeof(s));
    }

    void step() {
        wgpu::CommandEncoderDescriptor desc{};
        desc.label = wgpu::StringView("FluidStepEncoder");
        wgpu::raii::CommandEncoder enc = device_.createCommandEncoder(desc);
        step(enc);
        wgpu::raii::CommandBuffer cmd = enc->finish({});
        queue_.submit(1, &*cmd);
    }

    void step(wgpu::raii::CommandEncoder& enc) {
        uint32_t W = (state.width + 15) / 16;
        uint32_t H = (state.height + 15) / 16;

        uint32_t DW = (state.dye_width + 15) / 16;
        uint32_t DH = (state.dye_height + 15) / 16;

        wgpu::ComputePassDescriptor passDesc{};
        passDesc.label = wgpu::StringView("FluidStepPass");
        wgpu::raii::ComputePassEncoder pass = enc->beginComputePass(passDesc);

        advect(pass, W, H);
        computeDivergence(pass, W, H);
        solvePressure(pass, W, H);
        subtractGradient(pass, W, H);
        applyBoundary(pass, W, H);
        computeCurl(pass, W, H);
        applyVorticity(pass, W, H);
        advectDye(pass, DW, DH);

        pass->end();
        ++frameIndex;
    }

    void inject(wgpu::raii::CommandEncoder& enc, std::span<const FluidSource> batch) {
        if (batch.empty()) {
            return;
        }

        auto params = std::ranges::to<std::vector<FluidState::InjectParams>>(
            batch | std::views::filter(&FluidSource::active) |
            std::views::transform([](const auto& s) { return toParams(s); }));

        injectBatch(enc, params);
    }

    void paintObstacle(wgpu::raii::CommandEncoder& enc, uint32_t cx, uint32_t cy, uint32_t radius,
                       bool erase = false) {
        FluidState::FillCircleParams p{cx, cy, radius, erase ? 0u : 1u, state.width, state.height};
        queue_.writeBuffer(*state.fillCircleBuffer, 0, &p, sizeof(p));

        uint32_t W = (state.width + 15) / 16;
        uint32_t H = (state.height + 15) / 16;

        wgpu::raii::BindGroup bg =
            WGPUHelper::makeBindGroup(device_, pipelines.fillCircle,
                                      {*state.fillCircleBuffer, *state.obstacles}, "FillCircleBindGroup");

        wgpu::raii::ComputePassEncoder pass = enc->beginComputePass({});
        pipelines.dispatch(pass, pipelines.fillCircle, bg, W, H);
        pass->end();
    }

    FluidState state;
    FluidPipelines pipelines;
    ResamplePipelines resamplePipelines;

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

    static FluidState::InjectParams toParams(const FluidSource& s) {
        return {{s.color[0], s.color[1], s.color[2], 1.0f},
                s.x,
                s.y,
                s.vx,
                s.vy,
                s.radius,
                static_cast<uint32_t>(s.mode_mask),
                static_cast<uint32_t>(s.form),
                0u};
    }

    void initBindGroups() {
        bg_advect = WGPUHelper::makeBindGroup(
            device_, pipelines.advect,
            {*state.paramsBuffer, *state.velocity, *state.velocity_next, *state.obstacles},
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
        bg_advect_dye0 = WGPUHelper::makeBindGroup(
            device_, pipelines.advectDye,
            {*state.paramsBuffer, *state.velocity, *state.dye, *state.dye_next, *state.obstacles},
            "AdvectDyeBindGroup0");
        bg_advect_dye1 = WGPUHelper::makeBindGroup(
            device_, pipelines.advectDye,
            {*state.paramsBuffer, *state.velocity, *state.dye_next, *state.dye, *state.obstacles},
            "AdvectDyeBindGroup1");

        bg_curl = WGPUHelper::makeBindGroup(
            device_, pipelines.curl, {*state.paramsBuffer, *state.velocity, *state.curl}, "CurlBindGroup");
        bg_vorticity = WGPUHelper::makeBindGroup(device_, pipelines.vorticity,
                                                 {*state.paramsBuffer, *state.curl, *state.velocity},
                                                 "VorticityBindGroup");
    }

    void injectBatch(wgpu::raii::CommandEncoder& enc, std::span<const FluidState::InjectParams> params) {
        if (!state.uploadInjects(params)) {
            return;
        }

        uint32_t W = (std::max(state.width, state.dye_width) + 15) / 16;
        uint32_t H = (std::max(state.height, state.dye_height) + 15) / 16;

        wgpu::raii::BindGroup bg =
            WGPUHelper::makeBindGroup(device_, pipelines.inject,
                                      {*state.paramsBuffer, *state.injectCountBuffer, *state.injectsBuffer,
                                       *state.velocity, *getCurrentDye()},
                                      "InjectBindGroup");

        wgpu::raii::ComputePassEncoder pass = enc->beginComputePass({});
        pipelines.dispatch(pass, pipelines.inject, bg, W, H);
        pass->end();
    }

    void advect(wgpu::raii::ComputePassEncoder& pass, uint32_t W, uint32_t H) {
        pipelines.dispatch(pass, pipelines.advect, bg_advect, W, H);
    }

    void advectDye(wgpu::raii::ComputePassEncoder& pass, uint32_t W, uint32_t H) {
        pipelines.dispatch(pass, pipelines.advectDye, frameIndex % 2 ? bg_advect_dye1 : bg_advect_dye0, W, H);
    }

    void computeDivergence(wgpu::raii::ComputePassEncoder& pass, uint32_t W, uint32_t H) {
        pipelines.dispatch(pass, pipelines.divergence, bg_divergence, W, H);
    }

    void solvePressure(wgpu::raii::ComputePassEncoder& pass, uint32_t W, uint32_t H) {
        constexpr size_t ITERS = 30;
        static_assert(ITERS % 2 == 0, "pressure iterations must be even");
        for (size_t i = 0; i < ITERS; ++i) {
            pass->setPipeline(*pipelines.pressure);
            pass->setBindGroup(0, i % 2 ? *bg_pressure1 : *bg_pressure0, 0, nullptr);
            pass->dispatchWorkgroups(W, H, 1);
        }
    }

    void subtractGradient(wgpu::raii::ComputePassEncoder& pass, uint32_t W, uint32_t H) {
        pipelines.dispatch(pass, pipelines.subtract, bg_subtract, W, H);
    }

    void applyBoundary(wgpu::raii::ComputePassEncoder& pass, uint32_t W, uint32_t H) {
        pipelines.dispatch(pass, pipelines.boundary, bg_boundary, W, H);
    }

    void computeCurl(wgpu::raii::ComputePassEncoder& pass, uint32_t W, uint32_t H) {
        pipelines.dispatch(pass, pipelines.curl, bg_curl, W, H);
    }

    void applyVorticity(wgpu::raii::ComputePassEncoder& pass, uint32_t W, uint32_t H) {
        pipelines.dispatch(pass, pipelines.vorticity, bg_vorticity, W, H);
    }
};
