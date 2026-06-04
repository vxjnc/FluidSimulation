#pragma once

#include <webgpu/webgpu-raii.hpp>

#include "src/compute/wgpu_helper.hpp"

class FluidState {
public:
    void init(wgpu::Device device, wgpu::Queue queue, uint32_t w, uint32_t h) {
        device_ = device;
        queue_ = queue;

        paramsBuffer =
            WGPUHelper::makeBuffer(device, 4 * sizeof(float) + 2 * sizeof(uint32_t),
                                   wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst, "sim_params");

        injectBuffer = WGPUHelper::makeBuffer(
            device, 48, wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst, "inject_params");

        fillCircleBuffer = WGPUHelper::makeBuffer(device, 6 * sizeof(uint32_t),
                                                  wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst,
                                                  "FillCircleParams");

        resize(w, h);
    }

    void resize(uint32_t w, uint32_t h) {
        width = w;
        height = h;

        auto buf = [&](std::string_view label, size_t sizeElement) {
            return WGPUHelper::makeBuffer(device_, width * height * sizeElement,
                                          wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst, label);
        };

        velocity = buf("velocity", 2 * sizeof(float));
        velocity_next = buf("velocity_next", 2 * sizeof(float));
        pressure = buf("pressure", 1 * sizeof(float));
        pressure_next = buf("pressure_next", 1 * sizeof(float));
        divergence = buf("divergence", 1 * sizeof(float));
        curl = buf("curl", 1 * sizeof(float));
        dye = buf("dye", 4 * sizeof(float));
        dye_next = buf("dye_next", 4 * sizeof(float));
        obstacles = buf("obstacles", 1 * sizeof(uint32_t));

        uint32_t params[2] = {width, height};
        queue_.writeBuffer(*paramsBuffer, 0, params, sizeof(params));
    }

    void clear() {
        wgpu::CommandEncoderDescriptor encoderDesc{};
        encoderDesc.label = wgpu::StringView("FluidClearEncoder");
        wgpu::CommandEncoder encoder = device_.createCommandEncoder(encoderDesc);

        encoder.clearBuffer(*velocity, 0, velocity->getSize());
        encoder.clearBuffer(*velocity_next, 0, velocity_next->getSize());
        encoder.clearBuffer(*pressure, 0, pressure->getSize());
        encoder.clearBuffer(*pressure_next, 0, pressure_next->getSize());
        encoder.clearBuffer(*divergence, 0, divergence->getSize());
        encoder.clearBuffer(*curl, 0, curl->getSize());
        encoder.clearBuffer(*dye, 0, dye->getSize());
        encoder.clearBuffer(*dye_next, 0, dye_next->getSize());
        encoder.clearBuffer(*obstacles, 0, obstacles->getSize());

        wgpu::CommandBufferDescriptor cmdBufferDesc{};
        cmdBufferDesc.label = wgpu::StringView("FluidClear");
        wgpu::CommandBuffer commands = encoder.finish(cmdBufferDesc);

        queue_.submit(1, &commands);
    }

    uint32_t width = 0;
    uint32_t height = 0;

    wgpu::raii::Buffer velocity, velocity_next;
    wgpu::raii::Buffer pressure, pressure_next;
    wgpu::raii::Buffer divergence;
    wgpu::raii::Buffer curl;
    wgpu::raii::Buffer dye, dye_next;

    wgpu::raii::Buffer obstacles;

    wgpu::raii::Buffer paramsBuffer;
    wgpu::raii::Buffer injectBuffer;
    wgpu::raii::Buffer fillCircleBuffer;

private:
    wgpu::Device device_;
    wgpu::Queue queue_;
};
