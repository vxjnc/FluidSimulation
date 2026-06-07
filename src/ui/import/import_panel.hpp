#pragma once
#include <cstdint>
#include <filesystem>
#include <vector>

#include <imgui.h>
#include <nfd.hpp>
#include <stb_image.h>
#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.hpp>

#include "src/compute/wgpu_helper.hpp"
#include "src/wgpu_context.hpp"

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

    void render(std::span<const Action> actions, bool& open) {
        ImGui::Begin("Import", &open);

        if (ImGui::Button("Load Image...")) {
            openDialog();
        }

        if (loaded_) {
            ImGui::SameLine();
            ImGui::Text("%ux%u", imgW_, imgH_);

            float panelW = ImGui::GetContentRegionAvail().x;
            float aspect = static_cast<float>(imgH_) / static_cast<float>(imgW_);
            ImGui::Image(previewTexId_, ImVec2(panelW, panelW * aspect));

            ImGui::Separator();

            ImGui::TextUnformatted("Apply to:");
            for (const auto& action : actions) {
                if (ImGui::Button(action.label.data(), ImVec2(-1, 0))) {
                    LoadedImage img{pixels_, imgW_, imgH_};
                    action.callback(img);
                }
            }
        }

        ImGui::End();
    }

private:
    bool loaded_ = false;
    uint32_t imgW_ = 0, imgH_ = 0;
    std::vector<float> pixels_; // RGBA
    wgpu::raii::Texture previewTex_;
    wgpu::raii::TextureView previewView_;
    ImTextureID previewTexId_ = ImTextureID_Invalid;

    void openDialog() {
        std::string cwd = std::filesystem::current_path().string();

        NFD::UniquePath outPath;
        nfdu8filteritem_t filters[] = {{"Images", "png,jpg,jpeg,bmp,tga,hdr"}};
        if (NFD::OpenDialog(outPath, filters, 1, cwd.c_str()) != NFD_OKAY) {
            return;
        }

        int w, h, channels;
        stbi_uc* data = stbi_load(outPath.get(), &w, &h, &channels, 4);
        if (!data) {
            return;
        }

        imgW_ = static_cast<uint32_t>(w);
        imgH_ = static_cast<uint32_t>(h);

        pixels_.resize(imgW_ * imgH_ * 4);
        for (size_t i = 0; i < pixels_.size(); ++i) {
            pixels_[i] = static_cast<float>(data[i]) / 255.f;
        }

        stbi_image_free(data);

        uploadPreview();
        loaded_ = true;
    }

    void uploadPreview() {
        WGPUContext& ctx = WGPUContext::instance();
        wgpu::Device device = ctx.device();

        wgpu::Extent3D size = {imgW_, imgH_, 1};
        previewTex_ = WGPUHelper::makeTexture(
            device, size, wgpu::TextureFormat::RGBA8Unorm,
            wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::TextureBinding, "PreviewTex");
        previewView_ = previewTex_->createView();
        previewTexId_ = reinterpret_cast<ImTextureID>(static_cast<WGPUTextureView>(*previewView_));

        std::vector<std::byte> raw(imgW_ * imgH_ * 4);
        for (size_t i = 0; i < raw.size(); ++i) {
            raw[i] = static_cast<std::byte>(pixels_[i] * 255.f);
        }

        wgpu::TexelCopyTextureInfo dst{};
        dst.texture = *previewTex_;

        wgpu::TexelCopyBufferLayout layout{};
        layout.bytesPerRow = imgW_ * 4;
        layout.rowsPerImage = imgH_;

        ctx.queue().writeTexture(dst, raw.data(), raw.size(), layout, size);
    }
};
