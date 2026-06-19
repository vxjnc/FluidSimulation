#include "fluid_pipelines.hpp"

#include "generated/shaders/advect.wgsl.h"
#include "generated/shaders/advect_dye.wgsl.h"
#include "generated/shaders/boundary.wgsl.h"
#include "generated/shaders/curl.wgsl.h"
#include "generated/shaders/divergence.wgsl.h"
#include "generated/shaders/fill_circle.wgsl.h"
#include "generated/shaders/inject.wgsl.h"
#include "generated/shaders/pressure.wgsl.h"
#include "generated/shaders/subtract.wgsl.h"
#include "generated/shaders/vorticity.wgsl.h"
#include "src/compute/wgpu_helper.hpp"

void FluidPipelines::init(wgpu::Device device) {
    inject = WGPUHelper::makeComputePipeline(device, inject_wgsl, "Inject");
    fillCircle = WGPUHelper::makeComputePipeline(device, fill_circle_wgsl, "FillCircle");

    advect = WGPUHelper::makeComputePipeline(device, advect_wgsl, "Advect");
    advectDye = WGPUHelper::makeComputePipeline(device, advect_dye_wgsl, "AdvectDye");
    divergence = WGPUHelper::makeComputePipeline(device, divergence_wgsl, "Divergence");
    pressure = WGPUHelper::makeComputePipeline(device, pressure_wgsl, "Pressure");
    subtract = WGPUHelper::makeComputePipeline(device, subtract_wgsl, "Subtract");
    boundary = WGPUHelper::makeComputePipeline(device, boundary_wgsl, "Boundary");
    curl = WGPUHelper::makeComputePipeline(device, curl_wgsl, "Curl");
    vorticity = WGPUHelper::makeComputePipeline(device, vorticity_wgsl, "Vorticity");
}

void FluidPipelines::dispatch(wgpu::raii::ComputePassEncoder& pass, wgpu::raii::ComputePipeline& pipeline,
                              wgpu::raii::BindGroup& bg, uint32_t workgroupX, uint32_t workgroupY,
                              uint32_t workgroupZ) {
    pass->setPipeline(*pipeline);
    pass->setBindGroup(0, *bg, 0, nullptr);
    pass->dispatchWorkgroups(workgroupX, workgroupY, workgroupZ);
}
