#pragma once
#include <string_view>

#include <webgpu/webgpu-raii.hpp>

#include "generated/shaders/advect.wgsl.h"
#include "generated/shaders/advect_dye.wgsl.h"
#include "generated/shaders/boundary.wgsl.h"
#include "generated/shaders/divergence.wgsl.h"
#include "generated/shaders/fill_circle.wgsl.h"
#include "generated/shaders/inject.wgsl.h"
#include "generated/shaders/pressure.wgsl.h"
#include "generated/shaders/subtract.wgsl.h"
#include "src/compute/wgpu_helper.hpp"

class FluidPipelines {
public:
    void init(wgpu::Device device) {
        inject = WGPUHelper::makeComputePipeline(device, inject_wgsl, "Inject");
        fillCircle = WGPUHelper::makeComputePipeline(device, fill_circle_wgsl, "FillCircle");

        advect = WGPUHelper::makeComputePipeline(device, advect_wgsl, "Advect");
        advectDye = WGPUHelper::makeComputePipeline(device, advect_dye_wgsl, "AdvectDye");
        divergence = WGPUHelper::makeComputePipeline(device, divergence_wgsl, "Divergence");
        pressure = WGPUHelper::makeComputePipeline(device, pressure_wgsl, "Pressure");
        subtract = WGPUHelper::makeComputePipeline(device, subtract_wgsl, "Subtract");
        boundary = WGPUHelper::makeComputePipeline(device, boundary_wgsl, "Boundary");
    }

    static void dispatch(wgpu::raii::ComputePassEncoder& pass, wgpu::raii::ComputePipeline& pipeline,
                         wgpu::raii::BindGroup& bg, uint32_t W, uint32_t H) {
        pass->setPipeline(*pipeline);
        pass->setBindGroup(0, *bg, 0, nullptr);
        pass->dispatchWorkgroups(W, H, 1);
    }

    wgpu::raii::ComputePipeline inject;
    wgpu::raii::ComputePipeline fillCircle;

    wgpu::raii::ComputePipeline advect;
    wgpu::raii::ComputePipeline advectDye;
    wgpu::raii::ComputePipeline divergence;
    wgpu::raii::ComputePipeline pressure;
    wgpu::raii::ComputePipeline subtract;
    wgpu::raii::ComputePipeline boundary;
};
