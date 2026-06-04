#pragma once

#include <webgpu/webgpu-raii.hpp>

#include "src/compute/wgpu_helper.hpp"

class FluidState {
public:
    struct Params {
        uint32_t width, height;
        uint32_t dye_width, dye_height;
        float dt;
        float vel_dissipation;
        float dye_dissipation;
        float curl_strength;
    };

    struct FillCircleParams {
        uint32_t cx, cy;
        uint32_t radius;
        uint32_t val;
        uint32_t width, height;
    };

    struct alignas(16) InjectParams {
        float color[4];
        float x, y;
        float vx, vy;
        float radius;
        uint32_t mode_mask;
        uint32_t form;
    };

    void init(wgpu::Device device, wgpu::Queue queue, uint32_t w, uint32_t h, uint32_t dye_w,
              uint32_t dye_h) {
        device_ = device;
        queue_ = queue;

        paramsBuffer = WGPUHelper::makeBuffer(
            device, sizeof(Params), wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst, "Params");

        injectBuffer =
            WGPUHelper::makeBuffer(device, sizeof(InjectParams),
                                   wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst, "InjectParams");

        fillCircleBuffer = WGPUHelper::makeBuffer(device, sizeof(FillCircleParams),
                                                  wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst,
                                                  "FillCircleParams");

        resize(w, h);
        resizeDye(dye_w, dye_h);
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
        obstacles = buf("obstacles", 1 * sizeof(uint32_t));

        uint32_t params[2] = {width, height};
        queue_.writeBuffer(*paramsBuffer, 0, params, sizeof(params));
    }

    void resizeDye(uint32_t w, uint32_t h) {
        dye_width = w;
        dye_height = h;

        auto buf = [&](std::string_view label, size_t sizeElement) {
            return WGPUHelper::makeBuffer(device_, dye_width * dye_height * sizeElement,
                                          wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst, label);
        };

        dye = buf("dye", 4 * sizeof(float));
        dye_next = buf("dye_next", 4 * sizeof(float));

        uint32_t params[2] = {dye_width, dye_height};
        queue_.writeBuffer(*paramsBuffer, offsetof(Params, dye_width), params, sizeof(params));
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

    uint32_t dye_width = 0;
    uint32_t dye_height = 0;

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
