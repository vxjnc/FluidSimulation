#pragma once
#include <cstdint>

#include <imgui.h>
#include <webgpu/webgpu-raii.hpp>

struct FluidViewport {
    wgpu::raii::Texture texture;
    wgpu::raii::TextureView view;
    ImTextureID texId = ImTextureID_Invalid;
    uint32_t w = 0, h = 0;

    void init(wgpu::Device device, uint32_t width, uint32_t height, wgpu::TextureFormat format) {
        wgpu::TextureDescriptor texDesc{};
        texDesc.size = {width, height, 1};
        texDesc.mipLevelCount = 1;
        texDesc.sampleCount = 1;
        texDesc.format = format;
        texDesc.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding;
        texture = device.createTexture(texDesc);
        view = texture->createView();
        texId = reinterpret_cast<ImTextureID>(static_cast<WGPUTextureView>(*view));
        w = width;
        h = height;
    }
};
