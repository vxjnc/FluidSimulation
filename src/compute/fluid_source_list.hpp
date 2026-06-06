#pragma once
#include <ranges>
#include <vector>

#include <webgpu/webgpu-raii.hpp>

#include "src/compute/fluid_source.hpp"
#include "src/compute/fluid_state.hpp"

class FluidSourceList {
public:
    void setDevice(wgpu::Device device) { this->device = device; }

    void add(FluidSource s) {
        sources.emplace_back(std::move(s));
        buffers.emplace_back(makeBuffer());
    }

    void erase(size_t i) {
        sources.erase(sources.begin() + i);
        buffers.erase(buffers.begin() + i);
    }

    FluidSource& operator[](size_t i) { return sources[i]; }
    const FluidSource& operator[](size_t i) const { return sources[i]; }

    size_t size() const noexcept { return sources.size(); }
    bool empty() const noexcept { return sources.empty(); }

    auto begin() { return sources.begin(); }
    auto end() { return sources.end(); }
    auto begin() const { return sources.begin(); }
    auto end() const { return sources.end(); }

    auto zip() { return std::ranges::views::zip(sources, buffers); }

private:
    std::vector<FluidSource> sources;
    std::vector<wgpu::raii::Buffer> buffers;
    wgpu::Device device;

    wgpu::raii::Buffer makeBuffer() {
        wgpu::BufferDescriptor desc{};
        desc.size = sizeof(FluidState::InjectParams);
        desc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
        desc.label = wgpu::StringView("InjectBuffer");
        return device.createBuffer(desc);
    }
};
