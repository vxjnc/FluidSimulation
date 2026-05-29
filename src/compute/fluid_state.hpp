#pragma once

#include <webgpu/webgpu-raii.hpp>

class FluidState {
public:
    void init(wgpu::Device device, wgpu::Queue queue, uint32_t w, uint32_t h) {
        device_ = device;
        queue_ = queue;

        paramsBuffer = makeBuffer(2 * sizeof(uint32_t), wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst, "sim_params");
        dtBuffer = makeBuffer(sizeof(float), wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst, "dt");
        injectBuffer = makeBuffer(5 * sizeof(float), wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst, "inject_params");

        resize(w, h);
    }

    void resize(uint32_t w, uint32_t h) {
        width = w;
        height = h;

        auto buf = [&](std::string_view label, size_t sizeElement) {
            return makeBuffer(width * height * sizeElement, wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst, label);
        };

        velocity = buf("velocity", 2 * sizeof(float));
        velocity_next = buf("velocity_next", 2 * sizeof(float));
        pressure = buf("pressure", 1 * sizeof(float));
        pressure_next = buf("pressure_next", 1 * sizeof(float));
        divergence = buf("divergence", 1 * sizeof(float));
        dye = buf("dye", 1 * sizeof(float));
        dye_next = buf("dye_next", 1 * sizeof(float));
        obstacles = buf("obstacles", 1 * sizeof(uint32_t));

        uint32_t params[2] = {width, height};
        queue_.writeBuffer(*paramsBuffer, 0, params, sizeof(params));
    }

    uint32_t width = 0;
    uint32_t height = 0;

    wgpu::raii::Buffer velocity, velocity_next;
    wgpu::raii::Buffer pressure, pressure_next;
    wgpu::raii::Buffer divergence;
    wgpu::raii::Buffer dye, dye_next;

    wgpu::raii::Buffer obstacles;

    wgpu::raii::Buffer paramsBuffer;
    wgpu::raii::Buffer dtBuffer;
    wgpu::raii::Buffer injectBuffer;

private:
    wgpu::Buffer makeBuffer(size_t bytes, wgpu::BufferUsage usage, std::string_view label) {
        wgpu::BufferDescriptor desc{};
        desc.label = wgpu::StringView(label);
        desc.size = bytes;
        desc.usage = usage;
        return device_.createBuffer(desc);
    }

    wgpu::Device device_;
    wgpu::Queue queue_;
};
