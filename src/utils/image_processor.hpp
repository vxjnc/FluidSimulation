#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

#include <stb_image_resize2.h>

#include "src/ui/import/import_panel.hpp"

namespace ImageProcessor {
    inline std::vector<float> resizeRGBA(const float* srcData, uint32_t srcW, uint32_t srcH, uint32_t targetW,
                                         uint32_t targetH, bool flipY = true) {
        std::vector<float> result(targetW * targetH * 4);

        stbir_resize_float_linear(srcData, static_cast<int>(srcW), static_cast<int>(srcH), 0, result.data(),
                                  static_cast<int>(targetW), static_cast<int>(targetH), 0, STBIR_RGBA);

        if (flipY) {
            uint32_t stride = targetW * 4;
            for (uint32_t y = 0; y < targetH / 2; ++y) {
                float* row1 = result.data() + y * stride;
                float* row2 = result.data() + (targetH - 1 - y) * stride;
                std::swap_ranges(row1, row1 + stride, row2);
            }
        }

        return result;
    }

    inline std::vector<std::byte> processRawGPUData(const std::byte* rawData, uint32_t w, uint32_t h,
                                                    uint32_t bytesPerRow, bool isBGRA) {
        std::vector<std::byte> pixels(w * h * 4);

        constexpr auto srgbToLinear = [](std::byte b) {
            float x = static_cast<float>(static_cast<uint8_t>(b)) / 255.f;
            float l = x <= 0.04045f ? x / 12.92f : std::pow((x + 0.055f) / 1.055f, 2.4f);
            return static_cast<std::byte>(static_cast<uint8_t>(l * 255.f + 0.5f));
        };

        for (uint32_t y = 0; y < h; ++y) {
            const std::byte* srcRow = rawData + y * bytesPerRow;
            std::byte* dstRow = pixels.data() + y * w * 4;

            for (uint32_t x = 0; x < w; ++x) {
                dstRow[x * 4 + 0] = srgbToLinear(isBGRA ? srcRow[x * 4 + 2] : srcRow[x * 4 + 0]); // R
                dstRow[x * 4 + 1] = srgbToLinear(srcRow[x * 4 + 1]);                              // G
                dstRow[x * 4 + 2] = srgbToLinear(isBGRA ? srcRow[x * 4 + 0] : srcRow[x * 4 + 2]); // B
                dstRow[x * 4 + 3] = srcRow[x * 4 + 3];                                            // A
            }
        }

        return pixels;
    }
}
