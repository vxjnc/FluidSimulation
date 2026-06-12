#pragma once
#include <concepts>
#include <span>

#include <webgpu/webgpu-raii.hpp>

#include "src/wgpu_context.hpp"

namespace GpuReadback {
    template <typename T>
    concept ReadbackCallback = std::invocable<T, std::span<std::byte>>;

    inline uint32_t alignUp(uint32_t v, uint32_t align) { return (v + align - 1) & ~(align - 1); }

    template <ReadbackCallback Callback>
    inline void request(wgpu::Buffer buffer, uint64_t size, Callback callback) {
        WGPUContext& ctx = WGPUContext::instance();

        struct State {
            wgpu::raii::Buffer staging;
            uint64_t size;
            Callback callback;
        };

        wgpu::BufferDescriptor desc{};
        desc.size = size;
        desc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;

        auto* state = new State{ctx.device().createBuffer(desc), size, std::move(callback)};

        wgpu::raii::CommandEncoder enc = ctx.device().createCommandEncoder({});
        enc->copyBufferToBuffer(buffer, 0, *state->staging, 0, size);
        wgpu::raii::CommandBuffer cmd = enc->finish({});
        ctx.queue().submit(1, &*cmd);

        mapAsync(state);
    }

    template <ReadbackCallback Callback>
    inline void request(wgpu::Texture texture, uint32_t w, uint32_t h, wgpu::TextureFormat,
                        Callback callback) {
        WGPUContext& ctx = WGPUContext::instance();

        uint32_t bytesPerRow = alignUp(w * 4, 256);
        uint64_t size = static_cast<uint64_t>(bytesPerRow) * h;

        struct State {
            wgpu::raii::Buffer staging;
            uint64_t size;
            Callback callback;
        };

        wgpu::BufferDescriptor desc{};
        desc.size = size;
        desc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;

        auto* state = new State{ctx.device().createBuffer(desc), size, std::move(callback)};

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

        wgpu::raii::CommandEncoder enc = ctx.device().createCommandEncoder({});
        enc->copyTextureToBuffer(src, dst, {w, h, 1});
        wgpu::raii::CommandBuffer cmd = enc->finish({});
        ctx.queue().submit(1, &*cmd);

        mapAsync(state);
    }

    template <typename State> inline void mapAsync(State* state) {
        wgpu::BufferMapCallbackInfo info{};
        info.mode = wgpu::CallbackMode::AllowSpontaneous;
        info.callback = [](WGPUMapAsyncStatus status, WGPUStringView, void* userdata1, void*) {
            State* s = static_cast<State*>(userdata1);
            if (status == wgpu::MapAsyncStatus::Success) {
                const auto* ptr = static_cast<const std::byte*>(s->staging->getConstMappedRange(0, s->size));
                s->callback(std::span(ptr, s->size));
                s->staging->unmap();
            }
            delete s;
        };
        info.userdata1 = state;
        wgpuBufferMapAsync(*state->staging, wgpu::MapMode::Read, 0, state->size, info);
    }
};
