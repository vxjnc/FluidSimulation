#include "import_panel.hpp"

#include <algorithm>
#include <ranges>

#include <magic_enum/magic_enum_utility.hpp>
#include <stb_image.h>

#include "src/compute/wgpu_helper.hpp"
#include "src/utils/file_dialog.hpp"
#include "src/wgpu_context.hpp"

void ImportPanel::render(bool& open) {
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

        LoadedImage img{pixels_, imgW_, imgH_};

        magic_enum::enum_for_each<Target>([&](Target target) {
            if (ImGui::Button(magic_enum::enum_name(target).data(), ImVec2(-1, 0))) {
                importRequested(target, img);
            }
        });
    }

    ImGui::End();
}

void ImportPanel::openDialog() {
    nfdu8filteritem_t filters[] = {{"Images", "png,jpg,jpeg,bmp,tga,hdr"}};
    FileDialog::Open(filters, [this](const char* outPath) {
        int w, h, channels;
        stbi_uc* data = stbi_load(outPath, &w, &h, &channels, 4);
        if (!data) {
            return;
        }

        imgW_ = static_cast<uint32_t>(w);
        imgH_ = static_cast<uint32_t>(h);

        pixels_.resize(imgW_ * imgH_ * 4);
        std::transform(data, data + pixels_.size(), pixels_.begin(), [](stbi_uc p) { return p / 255.f; });

        stbi_image_free(data);

        uploadPreview();
        loaded_ = true;
    });
}

void ImportPanel::uploadPreview() {
    WGPUContext& ctx = WGPUContext::instance();
    wgpu::Device device = ctx.device();

    wgpu::Extent3D size = {imgW_, imgH_, 1};
    previewTex_ = WGPUHelper::makeTexture(device, size, wgpu::TextureFormat::RGBA8Unorm,
                                          wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::TextureBinding,
                                          "PreviewTex");
    previewView_ = previewTex_->createView();
    previewTexId_ = reinterpret_cast<ImTextureID>(static_cast<WGPUTextureView>(*previewView_));

    auto raw = std::ranges::to<std::vector>(
        pixels_ | std::views::transform([](float p) { return static_cast<std::byte>(p * 255.f); }));

    wgpu::TexelCopyTextureInfo dst{};
    dst.texture = *previewTex_;

    wgpu::TexelCopyBufferLayout layout{};
    layout.bytesPerRow = imgW_ * 4;
    layout.rowsPerImage = imgH_;

    ctx.queue().writeTexture(dst, raw.data(), raw.size(), layout, size);
}
