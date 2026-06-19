#include "fluid_sim.hpp"

#include <ranges>

#include "src/compute/fluid_source.hpp"
#include "src/compute/wgpu_helper.hpp"

void FluidSim::init(wgpu::Device device, wgpu::Queue queue, uint32_t width, uint32_t height,
                    uint32_t dye_width, uint32_t dye_height) {
    device_ = device;
    queue_ = queue;
    state.init(device, queue, width, height, dye_width, dye_height);
    pipelines.init(device);
    resamplePipelines.init(device);
    initBindGroups();

    profiler.init(device);
}

void FluidSim::resize(uint32_t w, uint32_t h, uint32_t dye_w, uint32_t dye_h) {
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

void FluidSim::resizeWithResample(uint32_t w, uint32_t h, uint32_t dye_w, uint32_t dye_h) {
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
        queue_.writeBuffer(*state.paramsBuffer, offsetof(FluidState::Params, dye_width), dye, sizeof(dye));
    }
    else {
        pass->end();
        wgpu::raii::CommandBuffer cmd = enc->finish({});
        queue_.submit(1, &*cmd);
    }

    initBindGroups();
}

void FluidSim::setDt(float dt) {
    queue_.writeBuffer(*state.paramsBuffer, offsetof(FluidState::Params, dt), &dt, sizeof(dt));
}
void FluidSim::setVelDissipation(float d) {
    queue_.writeBuffer(*state.paramsBuffer, offsetof(FluidState::Params, vel_dissipation), &d, sizeof(d));
}
void FluidSim::setDyeDissipation(float d) {
    queue_.writeBuffer(*state.paramsBuffer, offsetof(FluidState::Params, dye_dissipation), &d, sizeof(d));
}
void FluidSim::setCurlStrength(float s) {
    queue_.writeBuffer(*state.paramsBuffer, offsetof(FluidState::Params, curl_strength), &s, sizeof(s));
}

void FluidSim::step() {
    wgpu::CommandEncoderDescriptor desc{};
    desc.label = wgpu::StringView("FluidStepEncoder");
    wgpu::raii::CommandEncoder enc = device_.createCommandEncoder(desc);
    step(enc);
    wgpu::raii::CommandBuffer cmd = enc->finish({});
    queue_.submit(1, &*cmd);
}

void FluidSim::step(wgpu::raii::CommandEncoder& enc) {
    uint32_t W = (state.width + 15) / 16;
    uint32_t H = (state.height + 15) / 16;

    uint32_t DW = (state.dye_width + 15) / 16;
    uint32_t DH = (state.dye_height + 15) / 16;

    wgpu::ComputePassDescriptor passDesc{};
    passDesc.label = wgpu::StringView("FluidStepPass");
    wgpu::raii::ComputePassEncoder pass = enc->beginComputePass(passDesc);
    profiler.writeBegin(pass);

    advect(pass, W, H);
    computeDivergence(pass, W, H);
    solvePressure(pass, W, H);
    subtractGradient(pass, W, H);
    applyBoundary(pass, W, H);
    computeCurl(pass, W, H);
    applyVorticity(pass, W, H);
    advectDye(pass, DW, DH);

    profiler.writeEnd(pass);
    pass->end();

    profiler.resolve(enc);
    ++frameIndex;
}

void FluidSim::inject(wgpu::raii::CommandEncoder& enc, std::span<const FluidSource> batch) {
    if (batch.empty()) {
        return;
    }

    auto params = std::ranges::to<std::vector>(batch | std::views::filter(&FluidSource::active) |
                                               std::views::transform(toParams));

    injectBatch(enc, params);
}

void FluidSim::paintObstacle(wgpu::raii::CommandEncoder& enc, float cx, float cy, float radius, bool erase) {
    FluidState::FillCircleParams p{cx, cy, radius, erase ? 0u : 1u, state.width, state.height};
    queue_.writeBuffer(*state.fillCircleBuffer, 0, &p, sizeof(p));

    uint32_t W = (state.width + 15) / 16;
    uint32_t H = (state.height + 15) / 16;

    wgpu::raii::BindGroup bg = WGPUHelper::makeBindGroup(
        device_, pipelines.fillCircle, {*state.fillCircleBuffer, *state.obstacles}, "FillCircleBindGroup");

    wgpu::raii::ComputePassEncoder pass = enc->beginComputePass({});
    pipelines.dispatch(pass, pipelines.fillCircle, bg, W, H);
    pass->end();
}

FluidState::InjectParams FluidSim::toParams(const FluidSource& s) {
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

void FluidSim::initBindGroups() {
    bg_advect = WGPUHelper::makeBindGroup(
        device_, pipelines.advect,
        {*state.paramsBuffer, *state.velocity, *state.velocity_next, *state.obstacles}, "AdvectBindGroup");
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

    bg_curl = WGPUHelper::makeBindGroup(device_, pipelines.curl,
                                        {*state.paramsBuffer, *state.velocity, *state.curl}, "CurlBindGroup");
    bg_vorticity =
        WGPUHelper::makeBindGroup(device_, pipelines.vorticity,
                                  {*state.paramsBuffer, *state.curl, *state.velocity}, "VorticityBindGroup");
}

void FluidSim::injectBatch(wgpu::raii::CommandEncoder& enc,
                           std::span<const FluidState::InjectParams> params) {
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

void FluidSim::advect(wgpu::raii::ComputePassEncoder& pass, uint32_t W, uint32_t H) {
    pipelines.dispatch(pass, pipelines.advect, bg_advect, W, H);
}

void FluidSim::advectDye(wgpu::raii::ComputePassEncoder& pass, uint32_t W, uint32_t H) {
    pipelines.dispatch(pass, pipelines.advectDye, frameIndex % 2 ? bg_advect_dye1 : bg_advect_dye0, W, H);
}

void FluidSim::computeDivergence(wgpu::raii::ComputePassEncoder& pass, uint32_t W, uint32_t H) {
    pipelines.dispatch(pass, pipelines.divergence, bg_divergence, W, H);
}

void FluidSim::solvePressure(wgpu::raii::ComputePassEncoder& pass, uint32_t W, uint32_t H) {
    constexpr size_t ITERS = 30;
    static_assert(ITERS % 2 == 0, "pressure iterations must be even");
    for (size_t i = 0; i < ITERS; ++i) {
        pipelines.dispatch(pass, pipelines.pressure, i % 2 ? bg_pressure1 : bg_pressure0, W, H);
    }
}

void FluidSim::subtractGradient(wgpu::raii::ComputePassEncoder& pass, uint32_t W, uint32_t H) {
    pipelines.dispatch(pass, pipelines.subtract, bg_subtract, W, H);
}

void FluidSim::applyBoundary(wgpu::raii::ComputePassEncoder& pass, uint32_t W, uint32_t H) {
    pipelines.dispatch(pass, pipelines.boundary, bg_boundary, W, H);
}

void FluidSim::computeCurl(wgpu::raii::ComputePassEncoder& pass, uint32_t W, uint32_t H) {
    pipelines.dispatch(pass, pipelines.curl, bg_curl, W, H);
}

void FluidSim::applyVorticity(wgpu::raii::ComputePassEncoder& pass, uint32_t W, uint32_t H) {
    pipelines.dispatch(pass, pipelines.vorticity, bg_vorticity, W, H);
}
