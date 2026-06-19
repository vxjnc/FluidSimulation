#include "screenshot_capture.hpp"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <vector>

#include <clip.h>
#include <stb_image_write.h>
#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.hpp>

#include "src/capture/gpu_readback.hpp"
#include "src/ui/fluid_viewport.hpp"
#include "src/utils/image_processor.hpp"
#include "src/wgpu_context.hpp"

namespace ScreenshotCapture {
    namespace {
        void saveToClipboard(std::span<const std::byte> pixels, uint32_t w, uint32_t h) {
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

        void saveToFile(std::filesystem::path filePath, std::span<const std::byte> pixels, uint32_t w,
                        uint32_t h) {
            stbi_write_png(filePath.string().c_str(), static_cast<int>(w), static_cast<int>(h), 4,
                           pixels.data(), static_cast<int>(w * 4));
        }
    }

    void request(const FluidViewport& viewport, Mode mode, std::filesystem::path path) {
        WGPUContext& ctx = WGPUContext::instance();

        uint32_t w = viewport.w;
        uint32_t h = viewport.h;
        bool isBGRA = ctx.surfaceFormat() == wgpu::TextureFormat::BGRA8Unorm ||
                      ctx.surfaceFormat() == wgpu::TextureFormat::BGRA8UnormSrgb;
        uint32_t bytesPerRow = GpuReadback::alignUp(w * 4, 256);

        GpuReadback::request(
            *viewport.texture, w, h, ctx.surfaceFormat(),
            [w, h, isBGRA, bytesPerRow, mode, path = std::move(path)](std::span<const std::byte> raw) {
                std::vector<std::byte> pixels =
                    ImageProcessor::processRawGPUData(raw.data(), w, h, bytesPerRow, isBGRA);

                if (mode == Mode::Clipboard) {
                    saveToClipboard(pixels, w, h);
                }
                else {
                    saveToFile(path, pixels, w, h);
                }
            });
    }
};
