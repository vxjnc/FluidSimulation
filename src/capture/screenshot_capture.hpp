#pragma once
#include <cstdint>
#include <filesystem>

struct FluidViewport;

namespace ScreenshotCapture {
    enum class Mode : uint8_t {
        File,
        Clipboard,
    };

    void request(const FluidViewport& viewport, Mode mode, std::filesystem::path path = {});
};
