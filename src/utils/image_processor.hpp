#pragma once
#include <cstdint>
#include <vector>

namespace ImageProcessor {
    std::vector<float> resizeRGBA(const float* srcData, uint32_t srcW, uint32_t srcH, uint32_t targetW,
                                  uint32_t targetH, bool flipY = true);

    std::vector<std::byte> processRawGPUData(const std::byte* rawData, uint32_t w, uint32_t h,
                                             uint32_t bytesPerRow, bool isBGRA);
}
