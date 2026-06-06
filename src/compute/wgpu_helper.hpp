#pragma once
#include <array>
#include <format>
#include <span>
#include <variant>

#include <webgpu/webgpu-raii.hpp>

namespace WGPUHelper {
    using BindGroupResource = std::variant<wgpu::Buffer, wgpu::TextureView, wgpu::Sampler>;

    template <typename Pipeline, size_t N>
    inline wgpu::BindGroup makeBindGroup(wgpu::Device device, Pipeline& pipeline,
                                         std::span<const BindGroupResource, N> resources,
                                         std::string_view label) {
        wgpu::raii::BindGroupLayout layout = pipeline->getBindGroupLayout(0);

        std::array<wgpu::BindGroupEntry, N> entries;

        uint32_t binding = 0;
        for (const auto& res : resources) {
            wgpu::BindGroupEntry e{};
            e.binding = binding++;
            std::visit(
                [&e](auto&& arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, wgpu::Buffer>) {
                        e.buffer = arg;
                        e.size = arg.getSize();
                    }
                    else if constexpr (std::is_same_v<T, wgpu::TextureView>) {
                        e.textureView = arg;
                    }
                    else if constexpr (std::is_same_v<T, wgpu::Sampler>) {
                        e.sampler = arg;
                    }
                },
                res);
            entries[binding - 1] = std::move(e);
        }

        wgpu::BindGroupDescriptor desc{};
        desc.layout = *layout;
        desc.entryCount = entries.size();
        desc.entries = entries.data();
        desc.label = wgpu::StringView(label);
        return device.createBindGroup(desc);
    }
    template <typename Pipeline, size_t N>
    inline wgpu::BindGroup makeBindGroup(wgpu::Device device, Pipeline& pipeline,
                                         const BindGroupResource (&resources)[N], std::string_view label) {
        return makeBindGroup(device, pipeline, std::span<const BindGroupResource, N>(resources), label);
    }

    inline wgpu::Buffer makeBuffer(wgpu::Device device, size_t bytes, wgpu::BufferUsage usage,
                                   std::string_view label, bool mappedAtCreation = false) {
        wgpu::BufferDescriptor desc{};
        desc.label = wgpu::StringView(label);
        desc.size = bytes;
        desc.usage = usage;
        desc.mappedAtCreation = mappedAtCreation;
        return device.createBuffer(desc);
    }

    inline wgpu::Texture makeTexture(wgpu::Device device, wgpu::Extent3D size, wgpu::TextureFormat format,
                                     wgpu::TextureUsage usage, std::string_view label,
                                     wgpu::TextureDimension dimension = wgpu::TextureDimension::_2D,
                                     uint32_t mipLevelCount = 1, uint32_t sampleCount = 1) {
        wgpu::TextureDescriptor texDesc = {};
        texDesc.label = wgpu::StringView(label);
        texDesc.dimension = dimension;
        texDesc.size = size;
        texDesc.format = format;
        texDesc.mipLevelCount = mipLevelCount;
        texDesc.sampleCount = sampleCount;
        texDesc.usage = usage;

        return device.createTexture(texDesc);
    }

    inline wgpu::ShaderModule makeShaderModule(wgpu::Device device, std::string_view wgsl,
                                               std::string_view label) {
        WGPUShaderSourceWGSL wgslDesc{};
        wgslDesc.chain.sType = wgpu::SType::ShaderSourceWGSL;
        wgslDesc.code = wgpu::StringView(wgsl);

        wgpu::ShaderModuleDescriptor desc{};
        desc.label = wgpu::StringView(label);
        desc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&wgslDesc);
        return device.createShaderModule(desc);
    }

    inline wgpu::raii::ComputePipeline makeComputePipeline(wgpu::Device device, std::string_view wgsl,
                                                           std::string_view label) {
        wgpu::raii::ShaderModule shader = makeShaderModule(device, wgsl, std::format("Shader{}", label));

        wgpu::ComputePipelineDescriptor pipeDesc{};
        pipeDesc.label = wgpu::StringView(label);
        pipeDesc.layout = nullptr;
        pipeDesc.compute.module = *shader;
        pipeDesc.compute.entryPoint = wgpu::StringView("main");

        return device.createComputePipeline(pipeDesc);
    }
}
