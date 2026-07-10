#pragma once
#include <cstdint>
#include <vector>

#include <imgui.h>
#include <sigslot/signal.hpp>
#include <webgpu/webgpu-raii.hpp>

class ImportPanel {
public:
    enum class Target : uint8_t { Dye, Velocity, Obstacles };

    struct LoadedImage {
        std::vector<float> pixels;
        uint32_t w, h;
    };

    sigslot::signal<Target, const LoadedImage&> importRequested;

    void render(bool& open);

private:
    bool loaded_ = false;
    uint32_t imgW_ = 0, imgH_ = 0;
    std::vector<float> pixels_; // RGBA
    wgpu::raii::Texture previewTex_;
    wgpu::raii::TextureView previewView_;
    ImTextureID previewTexId_ = ImTextureID_Invalid;

    void openDialog();

    void uploadPreview();
};
