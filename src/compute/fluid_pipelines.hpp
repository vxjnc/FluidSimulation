#pragma once
#include <array>
#include <initializer_list>
#include <string_view>
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
#include "src/compute/wgpu_helper.hpp"

class FluidPipelines {
public:
    void init(wgpu::Device device) {
        inject = createComputePipeline(device, inject_wgsl, "Inject", wgpu::BufferBindingType::Uniform,
                                       wgpu::BufferBindingType::Uniform, wgpu::BufferBindingType::Storage);

        injectDye =
            createComputePipeline(device, inject_dye_wgsl, "InjectDye", wgpu::BufferBindingType::Uniform,
                                  wgpu::BufferBindingType::Uniform, wgpu::BufferBindingType::Storage);

        advect =
            createComputePipeline(device, advect_wgsl, "Advect", wgpu::BufferBindingType::Uniform,
                                  wgpu::BufferBindingType::Uniform, wgpu::BufferBindingType::ReadOnlyStorage,
                                  wgpu::BufferBindingType::Storage, wgpu::BufferBindingType::ReadOnlyStorage);

        advectDye =
            createComputePipeline(device, advect_dye_wgsl, "AdvectDye", wgpu::BufferBindingType::Uniform,
                                  wgpu::BufferBindingType::Uniform, wgpu::BufferBindingType::ReadOnlyStorage,
                                  wgpu::BufferBindingType::ReadOnlyStorage, wgpu::BufferBindingType::Storage,
                                  wgpu::BufferBindingType::ReadOnlyStorage);

        divergence =
            createComputePipeline(device, divergence_wgsl, "Divergence", wgpu::BufferBindingType::Uniform,
                                  wgpu::BufferBindingType::ReadOnlyStorage, wgpu::BufferBindingType::Storage,
                                  wgpu::BufferBindingType::ReadOnlyStorage);

        pressure = createComputePipeline(
            device, pressure_wgsl, "Pressure", wgpu::BufferBindingType::Uniform,
            wgpu::BufferBindingType::ReadOnlyStorage, wgpu::BufferBindingType::ReadOnlyStorage,
            wgpu::BufferBindingType::Storage, wgpu::BufferBindingType::ReadOnlyStorage);

        subtract = createComputePipeline(
            device, subtract_wgsl, "Subtract", wgpu::BufferBindingType::Uniform,
            wgpu::BufferBindingType::ReadOnlyStorage, wgpu::BufferBindingType::ReadOnlyStorage,
            wgpu::BufferBindingType::Storage, wgpu::BufferBindingType::ReadOnlyStorage);

        boundary =
            createComputePipeline(device, boundary_wgsl, "Boundary", wgpu::BufferBindingType::Uniform,
                                  wgpu::BufferBindingType::Storage, wgpu::BufferBindingType::ReadOnlyStorage);
    }

    template <typename... Args>
    static wgpu::raii::ComputePipeline createComputePipeline(wgpu::Device device, std::string_view wgsl,
                                                             std::string_view label, Args... bindings) {
        wgpu::raii::ShaderModule shader =
            WGPUHelper::makeShaderModule(device, wgsl, std::format("Shader{}", label));

        constexpr size_t N = sizeof...(Args);
        std::array<wgpu::BindGroupLayoutEntry, N> entries;
        uint32_t binding = 0;
        ((entries[binding].binding = binding, entries[binding].visibility = wgpu::ShaderStage::Compute,
          entries[binding].buffer.type = bindings, binding++),
         ...);

        wgpu::BindGroupLayoutDescriptor bglDesc{};
        bglDesc.entryCount = entries.size();
        bglDesc.entries = entries.data();
        wgpu::raii::BindGroupLayout bgl = device.createBindGroupLayout(bglDesc);

        wgpu::PipelineLayoutDescriptor plDesc{};
        plDesc.bindGroupLayoutCount = 1;
        plDesc.bindGroupLayouts = reinterpret_cast<WGPUBindGroupLayout*>(&bgl);
        wgpu::raii::PipelineLayout pipelineLayout = device.createPipelineLayout(plDesc);

        wgpu::ComputePipelineDescriptor pipeDesc{};
        pipeDesc.label = wgpu::StringView(label);
        pipeDesc.layout = *pipelineLayout;
        pipeDesc.compute.module = *shader;
        pipeDesc.compute.entryPoint = wgpu::StringView("main");

        return device.createComputePipeline(pipeDesc);
    }

    static void dispatch(wgpu::raii::ComputePassEncoder& pass, wgpu::raii::ComputePipeline& pipeline,
                         wgpu::raii::BindGroup& bg, uint32_t W, uint32_t H) {
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
