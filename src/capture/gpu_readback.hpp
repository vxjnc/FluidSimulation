#pragma once
#include <concepts>
#include <span>

#include <webgpu/webgpu-raii.hpp>

#include "src/compute/wgpu_helper.hpp"

namespace GpuReadback {
    template <typename T, typename Callback>
    concept ReadbackCallback = std::invocable<Callback, std::span<const T>>;

    inline uint32_t alignUp(uint32_t v, uint32_t align) { return (v + align - 1) & ~(align - 1); }

    template <typename Callback> struct State {
        wgpu::raii::Buffer staging;
        uint64_t size;
        Callback callback;
    };

    template <typename T = std::byte, typename Callback>
        requires ReadbackCallback<T, Callback>
    inline State<Callback>* record(wgpu::CommandEncoder enc, wgpu::Device device, wgpu::Buffer buffer,
                                   uint64_t size, Callback callback) {
        auto* state = new State<Callback>{
            WGPUHelper::makeBuffer(device, size, wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead,
                                   "ReadbackBuffer"),
            size, std::move(callback)};
        enc.copyBufferToBuffer(buffer, 0, *state->staging, 0, size);
        return state;
    }

    template <typename T = std::byte, typename Callback> inline void startMap(State<Callback>* state) {
        mapAsync<T, Callback>(state);
    }

    template <typename T = std::byte, typename Callback>
        requires ReadbackCallback<T, Callback>
    inline void request(wgpu::Device device, wgpu::Buffer buffer, uint64_t size, Callback callback) {
        wgpu::raii::CommandEncoder enc = device.createCommandEncoder({});
        auto* state = record<T, Callback>(*enc, device, buffer, size, std::move(callback));
        wgpu::raii::CommandBuffer cmd = enc->finish({});
        device.getQueue().submit(1, &*cmd);

        startMap<T, Callback>(state);
    }

    template <typename T = std::byte, typename Callback>
        requires ReadbackCallback<T, Callback>
    inline void request(wgpu::Device device, wgpu::Texture texture, uint32_t w, uint32_t h,
                        wgpu::TextureFormat, Callback callback) {
        uint32_t bytesPerRow = alignUp(w * 4, 256);
        uint64_t size = static_cast<uint64_t>(bytesPerRow) * h;

        auto* state = new State<Callback>{
            WGPUHelper::makeBuffer(device, size, wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead,
                                   "ReadbackBuffer"),
            size, std::move(callback)};

        wgpu::TexelCopyTextureInfo src{};
        src.texture = texture;
        src.mipLevel = 0;
        src.origin = {0, 0, 0};
        src.aspect = wgpu::TextureAspect::All;

        wgpu::TexelCopyBufferInfo dst{};
        dst.buffer = *state->staging;
        dst.layout.offset = 0;
        dst.layout.bytesPerRow = bytesPerRow;
        dst.layout.rowsPerImage = h;

        wgpu::raii::CommandEncoder enc = device.createCommandEncoder({});
        enc->copyTextureToBuffer(src, dst, {w, h, 1});
        wgpu::raii::CommandBuffer cmd = enc->finish({});
        device.getQueue().submit(1, &*cmd);

        mapAsync<T, Callback>(state);
    }

    template <typename T, typename Callback> inline void mapAsync(State<Callback>* state) {
        wgpu::BufferMapCallbackInfo info{};
        info.mode = wgpu::CallbackMode::AllowSpontaneous;
        info.callback = [](WGPUMapAsyncStatus status, WGPUStringView, void* userdata1, void*) {
            auto* s = static_cast<State<Callback>*>(userdata1);
            if (status == wgpu::MapAsyncStatus::Success) {
                const auto* ptr = static_cast<const T*>(s->staging->getConstMappedRange(0, s->size));
                s->callback(std::span<const T>(ptr, s->size / sizeof(T)));
                s->staging->unmap();
            }
            delete s;
        };
        info.userdata1 = state;
        wgpuBufferMapAsync(*state->staging, wgpu::MapMode::Read, 0, state->size, info);
    }
};
