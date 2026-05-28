#pragma once
#include <initializer_list>
#include <string_view>
#include <utility>
#include <vector>

#include <webgpu/webgpu-raii.hpp>

#include "generated/shaders/advect.wgsl.h"
#include "generated/shaders/advect_dye.wgsl.h"
#include "generated/shaders/boundary.wgsl.h"
#include "generated/shaders/divergence.wgsl.h"
#include "generated/shaders/inject.wgsl.h"
#include "generated/shaders/inject_dye.wgsl.h"
#include "generated/shaders/pressure.wgsl.h"
#include "generated/shaders/subtract.wgsl.h"
#include "src/wgpu_context.hpp"

struct BindingDesc {
    wgpu::BufferBindingType type;
};

class FluidPipelines {
public:
    void init() {
        inject = createComputePipeline(
            inject_wgsl, "Inject",
            {{wgpu::BufferBindingType::Uniform}, {wgpu::BufferBindingType::Uniform}, {wgpu::BufferBindingType::Storage}});

        injectDye = createComputePipeline(
            inject_dye_wgsl, "InjectDye",
            {{wgpu::BufferBindingType::Uniform}, {wgpu::BufferBindingType::Uniform}, {wgpu::BufferBindingType::Storage}});

        advect = createComputePipeline(advect_wgsl, "Advect",
                                       {{wgpu::BufferBindingType::Uniform},
                                        {wgpu::BufferBindingType::Uniform},
                                        {wgpu::BufferBindingType::ReadOnlyStorage},
                                        {wgpu::BufferBindingType::Storage}});

        advectDye = createComputePipeline(advect_dye_wgsl, "AdvectDye",
                                          {{wgpu::BufferBindingType::Uniform},
                                           {wgpu::BufferBindingType::Uniform},
                                           {wgpu::BufferBindingType::ReadOnlyStorage},
                                           {wgpu::BufferBindingType::ReadOnlyStorage},
                                           {wgpu::BufferBindingType::Storage}});

        divergence = createComputePipeline(
            divergence_wgsl, "Divergence",
            {{wgpu::BufferBindingType::Uniform}, {wgpu::BufferBindingType::ReadOnlyStorage}, {wgpu::BufferBindingType::Storage}});

        pressure = createComputePipeline(pressure_wgsl, "Pressure",
                                         {{wgpu::BufferBindingType::Uniform},
                                          {wgpu::BufferBindingType::ReadOnlyStorage},
                                          {wgpu::BufferBindingType::ReadOnlyStorage},
                                          {wgpu::BufferBindingType::Storage}});

        subtract = createComputePipeline(subtract_wgsl, "Subtract",
                                         {{wgpu::BufferBindingType::Uniform},
                                          {wgpu::BufferBindingType::ReadOnlyStorage},
                                          {wgpu::BufferBindingType::ReadOnlyStorage},
                                          {wgpu::BufferBindingType::Storage}});

        boundary =
            createComputePipeline(boundary_wgsl, "Boundary", {{wgpu::BufferBindingType::Uniform}, {wgpu::BufferBindingType::Storage}});
    }

    static wgpu::raii::ComputePipeline createComputePipeline(std::string_view wgsl, std::string_view label,
                                                             std::initializer_list<BindingDesc> bindings) {
        WGPUContext& ctx = WGPUContext::instance();

        WGPUShaderSourceWGSL wgslDesc{};
        wgslDesc.chain.sType = wgpu::SType::ShaderSourceWGSL;
        wgslDesc.code = wgpu::StringView(wgsl);
        wgpu::ShaderModuleDescriptor shaderDesc{};
        shaderDesc.label = wgpu::StringView(label);
        shaderDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&wgslDesc);
        wgpu::raii::ShaderModule shader = ctx.device().createShaderModule(shaderDesc);

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

        wgpu::PipelineLayoutDescriptor plDesc{};
        plDesc.bindGroupLayoutCount = 1;
        plDesc.bindGroupLayouts = reinterpret_cast<WGPUBindGroupLayout*>(&bgl);
        wgpu::raii::PipelineLayout pipelineLayout = ctx.device().createPipelineLayout(plDesc);

        wgpu::ComputePipelineDescriptor pipeDesc{};
        pipeDesc.label = wgpu::StringView(label);
        pipeDesc.layout = *pipelineLayout;
        pipeDesc.compute.module = *shader;
        pipeDesc.compute.entryPoint = wgpu::StringView("main");

        return ctx.device().createComputePipeline(pipeDesc);
    }

    static void dispatch(wgpu::raii::ComputePassEncoder& pass, wgpu::raii::ComputePipeline& pipeline, wgpu::raii::BindGroup& bg, uint32_t W,
                         uint32_t H) {
        pass->setPipeline(*pipeline);
        pass->setBindGroup(0, *bg, 0, nullptr);
        pass->dispatchWorkgroups(W, H, 1);
    }

    wgpu::raii::ComputePipeline inject;
    wgpu::raii::ComputePipeline injectDye;
    wgpu::raii::ComputePipeline advect;
    wgpu::raii::ComputePipeline advectDye;
    wgpu::raii::ComputePipeline divergence;
    wgpu::raii::ComputePipeline pressure;
    wgpu::raii::ComputePipeline subtract;
    wgpu::raii::ComputePipeline boundary;
};