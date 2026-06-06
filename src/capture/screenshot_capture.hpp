#pragma once
#include <cstdint>
#include <cstring>
#include <vector>

#include <clip.h>
#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.hpp>

#include "src/compute/wgpu_helper.hpp"
#include "src/ui/fluid_viewport.hpp"
#include "src/wgpu_context.hpp"

class ScreenshotCapture {
public:
    void request(const FluidViewport& viewport) {
        if (pending_) {
            return;
        }

        WGPUContext& ctx = WGPUContext::instance();
        wgpu::Device device = ctx.device();
        wgpu::Queue queue = ctx.queue();

        w_ = viewport.w;
        h_ = viewport.h;
        isBGRA_ = (ctx.surfaceFormat() == wgpu::TextureFormat::BGRA8Unorm ||
                   ctx.surfaceFormat() == wgpu::TextureFormat::BGRA8UnormSrgb);

        bytesPerRow_ = alignUp(w_ * 4, 256);

        stagingBuffer_ =
            WGPUHelper::makeBuffer(device, static_cast<uint64_t>(bytesPerRow_) * h_,
                                   wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead, "staging");

        wgpu::TexelCopyTextureInfo src{};
        src.texture = *viewport.texture;
        src.mipLevel = 0;
        src.origin = {0, 0, 0};
        src.aspect = wgpu::TextureAspect::All;

        wgpu::TexelCopyBufferInfo dst{};
        dst.buffer = *stagingBuffer_;
        dst.layout.offset = 0;
        dst.layout.bytesPerRow = bytesPerRow_;
        dst.layout.rowsPerImage = h_;

        wgpu::Extent3D extent{w_, h_, 1};

        wgpu::raii::CommandEncoder enc = device.createCommandEncoder({});
        enc->copyTextureToBuffer(src, dst, extent);
        wgpu::raii::CommandBuffer cmd = enc->finish({});
        queue.submit(1, &*cmd);

        wgpu::BufferMapCallbackInfo callbackInfo{};
        callbackInfo.mode = wgpu::CallbackMode::AllowSpontaneous;
        callbackInfo.callback = [](WGPUMapAsyncStatus status, WGPUStringView, void* userdata1, void*) {
            auto* self = static_cast<ScreenshotCapture*>(userdata1);
            if (status == wgpu::MapAsyncStatus::Success) {
                self->onMapped();
            }
            else {
                self->pending_ = false;
            }
        };
        callbackInfo.userdata1 = this;
        wgpuBufferMapAsync(*stagingBuffer_, wgpu::MapMode::Read, 0, stagingBuffer_->getSize(), callbackInfo);

        pending_ = true;
    }

private:
    bool pending_ = false;
    bool isBGRA_ = false;
    uint32_t w_ = 0, h_ = 0;
    uint32_t bytesPerRow_ = 0;
    wgpu::raii::Buffer stagingBuffer_;

    static uint32_t alignUp(uint32_t v, uint32_t align) { return (v + align - 1) & ~(align - 1); }

    void onMapped() {
        const std::byte* src = static_cast<const std::byte*>(
            stagingBuffer_->getConstMappedRange(0, static_cast<uint64_t>(bytesPerRow_) * h_));

        std::vector<std::byte> pixels(w_ * h_ * 4);
        for (uint32_t y = 0; y < h_; ++y) {
            const std::byte* row = src + y * bytesPerRow_;
            std::byte* dst = pixels.data() + y * w_ * 4;
            if (isBGRA_) {
                for (uint32_t x = 0; x < w_; ++x) {
                    dst[x * 4 + 0] = row[x * 4 + 2];
                    dst[x * 4 + 1] = row[x * 4 + 1];
                    dst[x * 4 + 2] = row[x * 4 + 0];
                    dst[x * 4 + 3] = row[x * 4 + 3];
                }
            }
            else {
                std::memcpy(dst, row, w_ * 4);
            }
        }

        stagingBuffer_->unmap();

        clip::image_spec spec;
        spec.width = w_;
        spec.height = h_;
        spec.bits_per_pixel = 32;
        spec.bytes_per_row = w_ * 4;
        spec.red_mask = 0x000000ff;
        spec.green_mask = 0x0000ff00;
        spec.blue_mask = 0x00ff0000;
        spec.alpha_mask = 0xff000000;
        spec.red_shift = 0;
        spec.green_shift = 8;
        spec.blue_shift = 16;
        spec.alpha_shift = 24;

        clip::image img(pixels.data(), spec);
        clip::set_image(img);

        pending_ = false;
    }
};
