#pragma once
#include <cstdint>

#include <webgpu/webgpu-raii.hpp>

class FluidPipelines {
public:
    void init(wgpu::Device device);

    static void dispatch(wgpu::raii::ComputePassEncoder& pass, wgpu::raii::ComputePipeline& pipeline,
                         wgpu::raii::BindGroup& bg, uint32_t workgroupX, uint32_t workgroupY,
                         uint32_t workgroupZ = 1);

    wgpu::raii::ComputePipeline inject;
    wgpu::raii::ComputePipeline fillCircle;

    wgpu::raii::ComputePipeline advect;
    wgpu::raii::ComputePipeline advectDye;
    wgpu::raii::ComputePipeline divergence;
    wgpu::raii::ComputePipeline pressure;
    wgpu::raii::ComputePipeline subtract;
    wgpu::raii::ComputePipeline boundary;
    wgpu::raii::ComputePipeline curl;
    wgpu::raii::ComputePipeline vorticity;
};
