#pragma once

#include <webgpu/webgpu-raii.hpp>

#include "src/wgpu_context.hpp"

class FluidState {
public:
    void init(uint32_t w, uint32_t h) {
        WGPUContext& ctx = WGPUContext::instance();

        paramsBuffer = ctx.createBuffer(2 * sizeof(uint32_t), wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst, "sim_params");
        dtBuffer = ctx.createBuffer(sizeof(float), wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst, "dt");
        injectBuffer = ctx.createBuffer(5 * sizeof(float), wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst, "inject_params");

        resize(w, h);
    }

    void resize(uint32_t w, uint32_t h) {
        width = w;
        height = h;
        WGPUContext& ctx = WGPUContext::instance();

        auto buf = [&](std::string_view label, size_t count) {
            return ctx.createBuffer(width * height * sizeof(float) * count, wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst, label);
        };

        velocity = buf("velocity", 2);
        velocity_next = buf("velocity_next", 2);
        pressure = buf("pressure", 1);
        pressure_next = buf("pressure_next", 1);
        divergence = buf("divergence", 1);
        dye = buf("dye", 1);
        dye_next = buf("dye_next", 1);

        uint32_t params[2] = {width, height};
        ctx.queue().writeBuffer(*paramsBuffer, 0, params, sizeof(params));
    }

    uint32_t width = 0;
    uint32_t height = 0;

    wgpu::raii::Buffer velocity, velocity_next;
    wgpu::raii::Buffer pressure, pressure_next;
    wgpu::raii::Buffer divergence;
    wgpu::raii::Buffer dye, dye_next;

    wgpu::raii::Buffer paramsBuffer;
    wgpu::raii::Buffer dtBuffer;
    wgpu::raii::Buffer injectBuffer;
};
