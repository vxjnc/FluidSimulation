#pragma once
#include <webgpu/webgpu-raii.hpp>

#include "generated/shaders/resample_f32.wgsl.h"
#include "generated/shaders/resample_u32.wgsl.h"
#include "generated/shaders/resample_vec2f.wgsl.h"
#include "generated/shaders/resample_vec4f.wgsl.h"
#include "src/compute/wgpu_helper.hpp"

class ResamplePipelines {
public:
    void init(wgpu::Device device) {
        f32 = WGPUHelper::makeComputePipeline(device, resample_f32_wgsl, "ResampleF32");
        vec2f = WGPUHelper::makeComputePipeline(device, resample_vec2f_wgsl, "ResampleVec2f");
        vec4f = WGPUHelper::makeComputePipeline(device, resample_vec4f_wgsl, "ResampleVec4f");
        u32 = WGPUHelper::makeComputePipeline(device, resample_u32_wgsl, "ResampleU32");
    }

    wgpu::raii::ComputePipeline u32;
    wgpu::raii::ComputePipeline f32;
    wgpu::raii::ComputePipeline vec2f;
    wgpu::raii::ComputePipeline vec4f;
};
