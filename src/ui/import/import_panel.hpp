#pragma once
#include <cstdint>
#include <span>
#include <vector>

#include <imgui.h>
#include <webgpu/webgpu-raii.hpp>

class ImportPanel {
public:
    struct LoadedImage {
        std::vector<float> pixels;
        uint32_t w, h;
    };

    struct Action {
        std::string label;
        std::function<void(const LoadedImage&)> callback;
    };

    template <size_t N> void render(const Action (&actions)[N], bool& open) {
        return render(std::span<const Action, N>(actions), open);
    }

    void render(std::span<const Action> actions, bool& open);

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
