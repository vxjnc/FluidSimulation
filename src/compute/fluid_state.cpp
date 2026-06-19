#include "fluid_state.hpp"

#include "src/compute/wgpu_helper.hpp"

void FluidState::init(wgpu::Device device, wgpu::Queue queue, uint32_t w, uint32_t h, uint32_t dye_w,
                      uint32_t dye_h) {
    device_ = device;
    queue_ = queue;

    paramsBuffer = WGPUHelper::makeBuffer(device, sizeof(Params),
                                          wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst, "Params");

    injectCountBuffer = WGPUHelper::makeBuffer(
        device, sizeof(uint32_t), wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst, "InjectCount");

    injectsBuffer = WGPUHelper::makeBuffer(
        device, sizeof(InjectParams), wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst, "Injects");
    injectsCapacity_ = 1;

    fillCircleBuffer =
        WGPUHelper::makeBuffer(device, sizeof(FillCircleParams),
                               wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst, "FillCircleParams");

    resize(w, h);
    resizeDye(dye_w, dye_h);
}

bool FluidState::uploadInjects(std::span<const InjectParams> sources) {
    uint32_t count = static_cast<uint32_t>(sources.size());
    queue_.writeBuffer(*injectCountBuffer, 0, &count, sizeof(count));

    if (count == 0) {
        return false;
    }

    if (count > injectsCapacity_) {
        injectsCapacity_ = count;
        injectsBuffer =
            WGPUHelper::makeBuffer(device_, sizeof(InjectParams) * injectsCapacity_,
                                   wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst, "Injects");
    }

    queue_.writeBuffer(*injectsBuffer, 0, sources.data(), sizeof(InjectParams) * count);
    return true;
}

void FluidState::resize(uint32_t w, uint32_t h) {
    width = w;
    height = h;

    auto buf = [&](std::string_view label, size_t sizeElement) {
        return WGPUHelper::makeBuffer(
            device_, width * height * sizeElement,
            wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc, label);
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

void FluidState::resizeDye(uint32_t w, uint32_t h) {
    dye_width = w;
    dye_height = h;

    auto buf = [&](std::string_view label, size_t sizeElement) {
        return WGPUHelper::makeBuffer(
            device_, dye_width * dye_height * sizeElement,
            wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::CopySrc, label);
    };

    dye = buf("dye", 4 * sizeof(float));
    dye_next = buf("dye_next", 4 * sizeof(float));

    uint32_t params[2] = {dye_width, dye_height};
    queue_.writeBuffer(*paramsBuffer, offsetof(Params, dye_width), params, sizeof(params));
}

void FluidState::clear() {
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
