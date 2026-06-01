#pragma once
#include <webgpu/webgpu-raii.hpp>

#include "generated/shaders/resample_f32.wgsl.h"
#include "generated/shaders/resample_u32.wgsl.h"
#include "generated/shaders/resample_vec2f.wgsl.h"
#include "src/compute/fluid_pipelines.hpp"

class ResamplePipelines {
public:
    void init(wgpu::Device device) {
        auto ros = wgpu::BufferBindingType::ReadOnlyStorage;
        auto s = wgpu::BufferBindingType::Storage;
        auto u = wgpu::BufferBindingType::Uniform;

        f32 = FluidPipelines::createComputePipeline(device, resample_f32_wgsl, "ResampleF32", u, ros, s);
        vec2f =
            FluidPipelines::createComputePipeline(device, resample_vec2f_wgsl, "ResampleVec2f", u, ros, s);
        u32 = FluidPipelines::createComputePipeline(device, resample_u32_wgsl, "ResampleU32", u, ros, s);
    }

    wgpu::raii::ComputePipeline f32;
    wgpu::raii::ComputePipeline vec2f;
    wgpu::raii::ComputePipeline u32;
};
