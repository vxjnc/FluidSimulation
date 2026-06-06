#pragma once
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <vector>

#include <clip.h>
#include <stb_image_write.h>
#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.hpp>

#include "src/compute/wgpu_helper.hpp"
#include "src/ui/fluid_viewport.hpp"
#include "src/wgpu_context.hpp"

class ScreenshotCapture {
public:
    enum class Mode : uint8_t {
        File,
        Clipboard,
    };

    void request(const FluidViewport& viewport, Mode mode, std::filesystem::path path = {}) {
        if (pending_) {
            return;
        }
        mode_ = mode;
        filePath = std::move(path);

        WGPUContext& ctx = WGPUContext::instance();
        wgpu::Device device = ctx.device();
        wgpu::Queue queue = ctx.queue();

        w = viewport.w;
        h = viewport.h;
        isBGRA = (ctx.surfaceFormat() == wgpu::TextureFormat::BGRA8Unorm ||
                  ctx.surfaceFormat() == wgpu::TextureFormat::BGRA8UnormSrgb);

        bytesPerRow = alignUp(w * 4, 256);

        stagingBuffer =
            WGPUHelper::makeBuffer(device, static_cast<uint64_t>(bytesPerRow) * h,
                                   wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead, "staging");

        wgpu::TexelCopyTextureInfo src{};
        src.texture = *viewport.texture;
        src.mipLevel = 0;
        src.origin = {0, 0, 0};
        src.aspect = wgpu::TextureAspect::All;

        wgpu::TexelCopyBufferInfo dst{};
        dst.buffer = *stagingBuffer;
        dst.layout.offset = 0;
        dst.layout.bytesPerRow = bytesPerRow;
        dst.layout.rowsPerImage = h;

        wgpu::Extent3D extent{w, h, 1};

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
        wgpuBufferMapAsync(*stagingBuffer, wgpu::MapMode::Read, 0, stagingBuffer->getSize(), callbackInfo);

        pending_ = true;
    }

private:
    bool pending_ = false;
    Mode mode_ = Mode::Clipboard;
    std::filesystem::path filePath;

    bool isBGRA = false;
    uint32_t w = 0, h = 0;
    uint32_t bytesPerRow = 0;
    wgpu::raii::Buffer stagingBuffer;

    static uint32_t alignUp(uint32_t v, uint32_t align) { return (v + align - 1) & ~(align - 1); }

    void onMapped() {
        const std::byte* src = static_cast<const std::byte*>(
            stagingBuffer->getConstMappedRange(0, static_cast<uint64_t>(bytesPerRow) * h));

        std::vector<std::byte> pixels(w * h * 4);
        for (uint32_t y = 0; y < h; ++y) {
            const std::byte* row = src + y * bytesPerRow;
            std::byte* dst = pixels.data() + y * w * 4;
            if (isBGRA) {
                for (uint32_t x = 0; x < w; ++x) {
                    dst[x * 4 + 0] = row[x * 4 + 2];
                    dst[x * 4 + 1] = row[x * 4 + 1];
                    dst[x * 4 + 2] = row[x * 4 + 0];
                    dst[x * 4 + 3] = row[x * 4 + 3];
                }
            }
            else {
                std::memcpy(dst, row, w * 4);
            }
        }

        stagingBuffer->unmap();

        if (mode_ == Mode::Clipboard) {
            saveToClipboard(pixels);
        }
        else {
            saveToFile(pixels);
        }

        pending_ = false;
    }

    void saveToClipboard(std::span<const std::byte> pixels) {

        clip::image_spec spec;
        spec.width = w;
        spec.height = h;
        spec.bits_per_pixel = 32;
        spec.bytes_per_row = w * 4;
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
    }

    void saveToFile(std::span<const std::byte> pixels) {
        stbi_write_png(filePath.string().c_str(), static_cast<int>(w), static_cast<int>(h), 4, pixels.data(),
                       static_cast<int>(w * 4));
    }
};
