#pragma once
#include <webgpu/webgpu-raii.hpp>

namespace WGPUHelper {
    inline wgpu::BindGroup makeBindGroup(wgpu::Device device, wgpu::raii::ComputePipeline& pipeline,
                                         std::initializer_list<wgpu::Buffer> buffers,
                                         std::string_view label) {
        wgpu::raii::BindGroupLayout layout = pipeline->getBindGroupLayout(0);

        std::vector<wgpu::BindGroupEntry> entries;
        entries.reserve(buffers.size());

        uint32_t binding = 0;
        for (auto& buf : buffers) {
            wgpu::BindGroupEntry e{};
            e.binding = binding++;
            e.buffer = buf;
            e.size = buf.getSize();
            entries.emplace_back(e);
        }

        wgpu::BindGroupDescriptor desc{};
        desc.layout = *layout;
        desc.entryCount = entries.size();
        desc.entries = entries.data();
        desc.label = wgpu::StringView(label);
        return device.createBindGroup(desc);
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
}
