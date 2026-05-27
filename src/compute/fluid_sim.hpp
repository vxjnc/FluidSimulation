#pragma once
#include <vector>

#include <webgpu/webgpu-raii.hpp>

#include "generated/shaders/advect.wgsl.h"
#include "generated/shaders/divergence.wgsl.h"
#include "generated/shaders/inject.wgsl.h"
#include "generated/shaders/pressure.wgsl.h"
#include "generated/shaders/subtract.wgsl.h"
#include "src/wgpu_context.hpp"

class FluidSim {
public:
    void init(uint32_t width, uint32_t height) {
        WGPUContext& ctx = WGPUContext::instance();
        paramsBuffer_ = ctx.createBuffer(2 * sizeof(uint32_t), wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst, "sim_params");
        resize(width, height);
        initPipelines();
    }

    void resize(uint32_t width, uint32_t height) {
        width_ = width;
        height_ = height;

        WGPUContext& ctx = WGPUContext::instance();

        auto vel_buf = [&](std::string_view label) {
            return ctx.createBuffer(width * height * sizeof(float) * 2, wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst, label);
        };
        auto scalar_buf = [&](std::string_view label) {
            return ctx.createBuffer(width * height * sizeof(float), wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst, label);
        };

        velocity = vel_buf("velocity");
        velocity_next = vel_buf("velocity_next");
        pressure = scalar_buf("pressure");
        pressure_next = scalar_buf("pressure_next");
        divergence = scalar_buf("divergence");

        uint32_t params[2] = {width, height};
        ctx.queue().writeBuffer(*paramsBuffer_, 0, params, sizeof(params));
    }

    void step(float dt) {
        WGPUContext& ctx = WGPUContext::instance();

        ctx.queue().writeBuffer(*dtBuffer_, 0, &dt, sizeof(dt));

        wgpu::raii::CommandEncoder enc = ctx.device().createCommandEncoder({});

        uint32_t W = (width_ + 7) / 8;
        uint32_t H = (height_ + 7) / 8;

        injectSource();

        advect(enc, W, H);
        std::swap(velocity, velocity_next);

        computeDivergence(enc, W, H);

        for (int i = 0; i < 30; ++i) {
            solvePressure(enc, W, H);
            std::swap(pressure, pressure_next);
        }

        subtractGradient(enc, W, H);
        std::swap(velocity, velocity_next);

        wgpu::raii::CommandBuffer cmd = enc->finish({});
        ctx.queue().submit(1, &*cmd);
    }

    void inject(wgpu::raii::CommandEncoder& enc, float x, float y, float vx, float vy, float radius = 10.0f) {
        WGPUContext& ctx = WGPUContext::instance();

        struct InjectParams {
            float x, y, vx, vy, radius;
        };
        InjectParams p{x, y, vx, vy, radius};
        ctx.queue().writeBuffer(*injectBuffer_, 0, &p, sizeof(p));

        wgpu::raii::BindGroup bg = makeBindGroup(injectPipeline_, {
                                                                      {*paramsBuffer_, 8},
                                                                      {*injectBuffer_, sizeof(InjectParams)},
                                                                      {*velocity, velocity->getSize()},
                                                                  });

        uint32_t W = (width_ + 7) / 8;
        uint32_t H = (height_ + 7) / 8;

        wgpu::raii::ComputePassEncoder pass = enc->beginComputePass({});
        dispatch(pass, injectPipeline_, bg, W, H);
        pass->end();
    }

    wgpu::raii::Buffer velocity, velocity_next;
    wgpu::raii::Buffer pressure, pressure_next;
    wgpu::raii::Buffer divergence;

private:
    void injectSource() {
        WGPUContext& ctx = WGPUContext::instance();
        constexpr uint32_t r = 1;
        uint32_t cx = width_ / 2;
        uint32_t cy = height_ / 2;

        std::vector<float> patch(r * 2 * 2);
        for (uint32_t i = 0; i < r * 2; ++i) {
            patch[i * 2 + 0] = 0.0f;
            patch[i * 2 + 1] = 100.0f;
        }

        for (uint32_t y = cy - r; y < cy + r; ++y) {
            uint64_t offset = (y * width_ + cx - r) * sizeof(float) * 2;
            ctx.queue().writeBuffer(*velocity, offset, patch.data(), patch.size() * sizeof(float));
        }
    }

    void initPipelines() {
        WGPUContext& ctx = WGPUContext::instance();

        dtBuffer_ = ctx.createBuffer(sizeof(float), wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst, "dt");

        injectBuffer_ = ctx.createBuffer(5 * sizeof(float), wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst, "inject_params");

        injectPipeline_ = createComputePipeline(inject_wgsl, "Inject",
                                                {
                                                    {wgpu::BufferBindingType::Uniform}, // params
                                                    {wgpu::BufferBindingType::Uniform}, // inject
                                                    {wgpu::BufferBindingType::Storage}, // velocity
                                                });

        advectPipeline_ = createComputePipeline(advect_wgsl, "Advect",
                                                {
                                                    {wgpu::BufferBindingType::Uniform},         // params
                                                    {wgpu::BufferBindingType::Uniform},         // dt
                                                    {wgpu::BufferBindingType::ReadOnlyStorage}, // velocity
                                                    {wgpu::BufferBindingType::Storage},         // velocity_next
                                                });

        divergencePipeline_ = createComputePipeline(divergence_wgsl, "Divergence",
                                                    {
                                                        {wgpu::BufferBindingType::Uniform},         // params
                                                        {wgpu::BufferBindingType::ReadOnlyStorage}, // velocity
                                                        {wgpu::BufferBindingType::Storage},         // divergence
                                                    });

        pressurePipeline_ = createComputePipeline(pressure_wgsl, "Pressure",
                                                  {
                                                      {wgpu::BufferBindingType::Uniform},         // params
                                                      {wgpu::BufferBindingType::ReadOnlyStorage}, // pressure
                                                      {wgpu::BufferBindingType::ReadOnlyStorage}, // divergence
                                                      {wgpu::BufferBindingType::Storage},         // pressure_next
                                                  });

        subtractPipeline_ = createComputePipeline(subtract_wgsl, "Subtract",
                                                  {
                                                      {wgpu::BufferBindingType::Uniform},         // params
                                                      {wgpu::BufferBindingType::ReadOnlyStorage}, // pressure
                                                      {wgpu::BufferBindingType::ReadOnlyStorage}, // velocity
                                                      {wgpu::BufferBindingType::Storage},         // velocity_next
                                                  });
    }

    struct BindingDesc {
        wgpu::BufferBindingType type;
    };

    wgpu::raii::ComputePipeline createComputePipeline(std::string_view wgsl, std::string_view label,
                                                      std::initializer_list<BindingDesc> bindings) {
        WGPUContext& ctx = WGPUContext::instance();

        // Shader
        WGPUShaderSourceWGSL wgslDesc{};
        wgslDesc.chain.sType = wgpu::SType::ShaderSourceWGSL;
        wgslDesc.code = wgpu::StringView(wgsl);
        wgpu::ShaderModuleDescriptor shaderDesc{};
        shaderDesc.label = wgpu::StringView(label);
        shaderDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&wgslDesc);
        wgpu::raii::ShaderModule shader = ctx.device().createShaderModule(shaderDesc);

        // Bind group layout
        std::vector<wgpu::BindGroupLayoutEntry> entries;
        uint32_t binding = 0;
        for (auto& b : bindings) {
            wgpu::BindGroupLayoutEntry e{};
            e.binding = binding++;
            e.visibility = wgpu::ShaderStage::Compute;
            e.buffer.type = b.type;
            entries.push_back(e);
        }
        wgpu::BindGroupLayoutDescriptor bglDesc{};
        bglDesc.entryCount = entries.size();
        bglDesc.entries = entries.data();
        wgpu::raii::BindGroupLayout bgl = ctx.device().createBindGroupLayout(bglDesc);

        // Pipeline layout
        wgpu::PipelineLayoutDescriptor plDesc{};
        plDesc.bindGroupLayoutCount = 1;
        plDesc.bindGroupLayouts = (WGPUBindGroupLayout*)&bgl;
        wgpu::raii::PipelineLayout pipelineLayout = ctx.device().createPipelineLayout(plDesc);

        // Pipeline
        wgpu::ComputePipelineDescriptor pipeDesc{};
        pipeDesc.label = wgpu::StringView(label);
        pipeDesc.layout = *pipelineLayout;
        pipeDesc.compute.module = *shader;
        pipeDesc.compute.entryPoint = wgpu::StringView("main");

        return ctx.device().createComputePipeline(pipeDesc);
    }

    void dispatch(wgpu::raii::ComputePassEncoder& pass, wgpu::raii::ComputePipeline& pipeline, wgpu::raii::BindGroup& bg, uint32_t W,
                  uint32_t H) {
        pass->setPipeline(*pipeline);
        pass->setBindGroup(0, *bg, 0, nullptr);
        pass->dispatchWorkgroups(W, H, 1);
    }

    wgpu::raii::BindGroup makeBindGroup(wgpu::raii::ComputePipeline& pipeline,
                                        std::initializer_list<std::pair<wgpu::Buffer, uint64_t>> buffers) {
        WGPUContext& ctx = WGPUContext::instance();
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
        return ctx.device().createBindGroup(desc);
    }

    void advect(wgpu::raii::CommandEncoder& enc, uint32_t W, uint32_t H) {
        wgpu::raii::BindGroup bg = makeBindGroup(advectPipeline_, {
                                                                      {*paramsBuffer_, 8},
                                                                      {*dtBuffer_, 4},
                                                                      {*velocity, velocity->getSize()},
                                                                      {*velocity_next, velocity_next->getSize()},
                                                                  });
        wgpu::raii::ComputePassEncoder pass = enc->beginComputePass({});
        dispatch(pass, advectPipeline_, bg, W, H);
        pass->end();
    }

    void computeDivergence(wgpu::raii::CommandEncoder& enc, uint32_t W, uint32_t H) {
        wgpu::raii::BindGroup bg = makeBindGroup(divergencePipeline_, {
                                                                          {*paramsBuffer_, 8},
                                                                          {*velocity, velocity->getSize()},
                                                                          {*divergence, divergence->getSize()},
                                                                      });
        wgpu::raii::ComputePassEncoder pass = enc->beginComputePass({});
        dispatch(pass, divergencePipeline_, bg, W, H);
        pass->end();
    }

    void solvePressure(wgpu::raii::CommandEncoder& enc, uint32_t W, uint32_t H) {
        wgpu::raii::BindGroup bg = makeBindGroup(pressurePipeline_, {
                                                                        {*paramsBuffer_, 8},
                                                                        {*pressure, pressure->getSize()},
                                                                        {*divergence, divergence->getSize()},
                                                                        {*pressure_next, pressure_next->getSize()},
                                                                    });
        wgpu::raii::ComputePassEncoder pass = enc->beginComputePass({});
        dispatch(pass, pressurePipeline_, bg, W, H);
        pass->end();
    }

    void subtractGradient(wgpu::raii::CommandEncoder& enc, uint32_t W, uint32_t H) {
        wgpu::raii::BindGroup bg = makeBindGroup(subtractPipeline_, {
                                                                        {*paramsBuffer_, 8},
                                                                        {*pressure, pressure->getSize()},
                                                                        {*velocity, velocity->getSize()},
                                                                        {*velocity_next, velocity_next->getSize()},
                                                                    });
        wgpu::raii::ComputePassEncoder pass = enc->beginComputePass({});
        dispatch(pass, subtractPipeline_, bg, W, H);
        pass->end();
    }

    uint32_t width_ = 0;
    uint32_t height_ = 0;

    wgpu::raii::Buffer paramsBuffer_;
    wgpu::raii::Buffer dtBuffer_;

    wgpu::raii::Buffer injectBuffer_;
    wgpu::raii::ComputePipeline injectPipeline_;

    wgpu::raii::ComputePipeline advectPipeline_;
    wgpu::raii::ComputePipeline divergencePipeline_;
    wgpu::raii::ComputePipeline pressurePipeline_;
    wgpu::raii::ComputePipeline subtractPipeline_;
};
