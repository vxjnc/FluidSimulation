#pragma once
#include <webgpu/webgpu-raii.hpp>

class ResamplePipelines {
public:
    void init(wgpu::Device device);

    wgpu::raii::ComputePipeline u32;
    wgpu::raii::ComputePipeline f32;
    wgpu::raii::ComputePipeline vec2f;
    wgpu::raii::ComputePipeline vec4f;
};
