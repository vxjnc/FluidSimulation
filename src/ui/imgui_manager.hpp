#pragma once

#include <array>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_wgpu.h>
#include <imgui_internal.h>
#include <nfd.hpp>
#include <sigslot/signal.hpp>
#include <stb_image_resize2.h>
#include <webgpu/webgpu.hpp>

#include "src/app_settings.hpp"
#include "src/compute/fluid_sim.hpp"
#include "src/ui/controls/controls_panel.hpp"
#include "src/ui/fluid_viewport.hpp"
#include "src/ui/imgui_serialization.hpp"
#include "src/ui/imgui_style.hpp"
#include "src/ui/import/import_panel.hpp"
#include "src/ui/random_splat/splat_panel.hpp"
#include "src/ui/stats/stats_panel.hpp"
#include "src/utils/image_processor.hpp"
#include "src/wgpu_context.hpp"

enum class ImportTarget : uint8_t { Dye, Velocity, Obstacles };

struct MouseState {
    float x = 0, y = 0;
    float dx = 0, dy = 0;
    bool rightPressed = false;
    bool leftPressed = false;
    bool leftJustPressed = false;
};

struct PanelVisibility {
    bool controls = true;
    bool randomSplat = false;
    bool import = false;
    bool stats = true;
};

class ImGuiManager {
    friend class ImguiSerialization;

public:
    sigslot::signal<std::vector<FluidSource>> onSplats;
    sigslot::signal<ImportTarget, std::vector<float>, uint32_t, uint32_t> onImport;
    sigslot::signal<std::string> onSaveRequested;
    sigslot::signal<std::string> onLoadRequested;
    sigslot::signal<> onScreenshotClipboard;
    sigslot::signal<std::string> onScreenshotFile;

    void init(GLFWwindow* window, wgpu::Device device, wgpu::TextureFormat surfaceFormat,
              AppSettings* settings) {
        this->settings = settings;

        ImGui::CreateContext();
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;

        ImGuiSettingsHandler handler{};
        handler.TypeName = "FluidSim";
        handler.TypeHash = ImHashStr("FluidSim");
        handler.ReadOpenFn = [](ImGuiContext*, ImGuiSettingsHandler*, const char* name) -> void* {
            char section_name[64];
            if (std::sscanf(name, "[%63[^]]", section_name) == 1) {
                return (void*)(intptr_t)ImHashStr(section_name);
            }
            return (void*)(intptr_t)ImHashStr(name);
        };
        handler.ReadLineFn = ImguiSerialization::ImguiReadLine;
        handler.WriteAllFn = ImguiSerialization::ImguiWriteAll;
        handler.UserData = this;
        ImGui::AddSettingsHandler(&handler);

        ImGui_ImplGlfw_InitForOther(window, true);

        AppStyle::apply();

        ImGui_ImplWGPU_InitInfo wgpuInfo{};
        wgpuInfo.Device = device;
        wgpuInfo.RenderTargetFormat = static_cast<WGPUTextureFormat>(surfaceFormat);
        wgpuInfo.NumFramesInFlight = 2;
        ImGui_ImplWGPU_Init(&wgpuInfo);
    }

    ~ImGuiManager() {
        ImGui_ImplWGPU_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void beginFrame() {
        ImGui_ImplWGPU_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void renderUI(FluidViewport& viewport, MouseState& mouse, FluidSim& sim, Render& render,
                  std::vector<FluidSource>& sources, const GpuProfiler<>& uiProfiler) {
        ImGuiIO& io = ImGui::GetIO();

        ImGuiID dockspaceId = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);

        if (!settings->ui.dockInitialized) {
            settings->ui.dockInitialized = true;

            ImGui::DockBuilderRemoveNode(dockspaceId);
            ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->Size);

            ImGuiID dockLeft, dockRight;
            ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.25f, &dockLeft, &dockRight);

            ImGuiID dockLeftTop, dockLeftBottom;
            ImGui::DockBuilderSplitNode(dockLeft, ImGuiDir_Up, 0.2f, &dockLeftTop, &dockLeftBottom);

            ImGui::DockBuilderDockWindow("Stats", dockLeftTop);
            ImGui::DockBuilderGetNode(dockLeftTop)->LocalFlags |= ImGuiDockNodeFlags_AutoHideTabBar;

            ImGui::DockBuilderDockWindow("Controls", dockLeftBottom);
            ImGui::DockBuilderGetNode(dockLeftBottom)->LocalFlags |= ImGuiDockNodeFlags_AutoHideTabBar;

            ImGui::DockBuilderDockWindow("Viewport", dockRight);
            ImGui::DockBuilderGetNode(dockRight)->LocalFlags |=
                ImGuiDockNodeFlags_AutoHideTabBar | ImGuiDockNodeFlags_NoUndocking;

            ImGui::DockBuilderDockWindow("Random Splat", dockLeftBottom);

            ImGui::DockBuilderDockWindow("Import", dockLeftBottom);

            ImGui::DockBuilderFinish(dockspaceId);
        }

        if (settings->ui.menuBarVisible) {
            renderMenuBar();
        }

        renderSettingsModal();

        if (visibility.stats) {
            statsPanel.render(visibility.stats, sim, render, uiProfiler);
        }
        if (visibility.controls) {
            controlsPanel.render(visibility.controls, viewport, sim, *settings, sources);
        }
        if (visibility.randomSplat) {
            splatPanel.render(visibility.randomSplat, settings->splatSettings, settings->ui.velocityMode);
            if (auto splats = splatPanel.takeSplats()) {
                onSplats(std::move(*splats));
            }
        }
        if (visibility.import) {
            importPanel.render(
                {ImportPanel::Action{"Dye",
                                     [&](const ImportPanel::LoadedImage& img) {
                                         uint32_t dw = sim.state.dye_width;
                                         uint32_t dh = sim.state.dye_height;
                                         auto pixels = ImageProcessor::resizeRGBA(img.pixels.data(), img.w,
                                                                                  img.h, dw, dh, true);
                                         onImport(ImportTarget::Dye, std::move(pixels), dw, dh);
                                     }},
                 ImportPanel::Action{"Velocity",
                                     [&](const ImportPanel::LoadedImage& img) {
                                         uint32_t vw = sim.state.width;
                                         uint32_t vh = sim.state.height;
                                         auto pixels = ImageProcessor::resizeRGBA(img.pixels.data(), img.w,
                                                                                  img.h, vw, vh, true);
                                         onImport(ImportTarget::Velocity, std::move(pixels), vw, vh);
                                     }},
                 ImportPanel::Action{"Obstacles",
                                     [&](const ImportPanel::LoadedImage& img) {
                                         uint32_t ow = sim.state.width;
                                         uint32_t oh = sim.state.height;
                                         auto pixels = ImageProcessor::resizeRGBA(img.pixels.data(), img.w,
                                                                                  img.h, ow, oh, true);
                                         onImport(ImportTarget::Obstacles, std::move(pixels), ow, oh);
                                     }}},
                visibility.import);
        }

        // --- Viewport Panel ---
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoTitleBar);
        {
            ImVec2 size = ImGui::GetContentRegionAvail();

            if (size.x > 0.0f && size.y > 0.0f) {
                uint32_t vw = static_cast<uint32_t>(size.x);
                uint32_t vh = static_cast<uint32_t>(size.y);

                if (vw != viewport.w || vh != viewport.h) {
                    viewport.init(WGPUContext::instance().device(), vw, vh,
                                  WGPUContext::instance().surfaceFormat());
                    sim.resizeWithResample(
                        static_cast<uint32_t>(static_cast<float>(vw) * settings->simScale),
                        static_cast<uint32_t>(static_cast<float>(vh) * settings->simScale),
                        static_cast<uint32_t>(static_cast<float>(vw) * settings->dyeScale),
                        static_cast<uint32_t>(static_cast<float>(vh) * settings->dyeScale));
                }

                if (ImGui::IsWindowHovered()) {
                    ImVec2 origin = ImGui::GetCursorScreenPos();
                    ImVec2 mpos = io.MousePos;
                    mouse.dx = io.MouseDelta.x;
                    mouse.dy = io.MouseDelta.y;
                    mouse.x = mpos.x - origin.x;
                    mouse.y = mpos.y - origin.y;
                    bool wasLeft = mouse.leftPressed;
                    mouse.leftPressed = ImGui::IsMouseDown(ImGuiMouseButton_Left);
                    mouse.leftJustPressed = !wasLeft && mouse.leftPressed;
                    mouse.rightPressed = ImGui::IsMouseDown(ImGuiMouseButton_Right);

                    if (io.MouseWheel != 0.f) {
                        settings->brushRadius =
                            std::clamp(settings->brushRadius * (1.f + io.MouseWheel * 0.1f), 0.f, 1.f);
                    }
                }
                else {
                    mouse.leftPressed = false;
                    mouse.dx = mouse.dy = 0;
                }

                ImVec2 origin = ImGui::GetCursorScreenPos();
                ImGui::Image(viewport.texId, size);
                if (settings->ui.showSourceOverlay) {
                    drawSourceOverlay(origin, size, sources);
                }
            }
        }
        ImGui::End();
        ImGui::PopStyleVar();

        ImGui::Render();
    }

    void endFrame(WGPURenderPassEncoder passEncoder) {
        ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), passEncoder);
    }

    PanelVisibility visibility;
    ControlsPanel controlsPanel;
    SplatPanel splatPanel;
    ImportPanel importPanel;
    StatsPanel statsPanel;

private:
    AppSettings* settings;

    void renderMenuBar() {
        if (!ImGui::BeginMainMenuBar()) {
            return;
        }

        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save Simulation...", "Ctrl+S")) {
                std::string cwd = std::filesystem::current_path().string();
                NFD::UniquePath outPath;
                nfdu8filteritem_t filters[] = {{"Fluid Simulation", "fsim"}};
                if (NFD::SaveDialog(outPath, filters, 1, cwd.c_str(), "simulation.fsim") == NFD_OKAY) {
                    onSaveRequested(outPath.get());
                }
            }
            if (ImGui::MenuItem("Open Simulation...", "Ctrl+O")) {
                std::string cwd = std::filesystem::current_path().string();
                NFD::UniquePath outPath;
                nfdu8filteritem_t filters[] = {{"Fluid Simulation", "fsim"}};
                if (NFD::OpenDialog(outPath, filters, 1, cwd.c_str()) == NFD_OKAY) {
                    onLoadRequested(outPath.get());
                }
            }

            if (ImGui::MenuItem("Save Screenshot")) {
                std::string cwd = std::filesystem::current_path().string();
                NFD::UniquePath outPath;
                nfdu8filteritem_t filters[] = {{"PNG Image", "png"}};
                if (NFD::SaveDialog(outPath, filters, 1, cwd.c_str(), "screenshot.png") == NFD_OKAY) {
                    onScreenshotFile(outPath.get());
                }
            }
            if (ImGui::MenuItem("Copy to Clipboard", "F12")) {
                onScreenshotClipboard();
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Settings")) {
                settings->ui.settingsOpen = true;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Controls", nullptr, &visibility.controls);
            ImGui::MenuItem("Random Splat", nullptr, &visibility.randomSplat);
            ImGui::MenuItem("Import", nullptr, &visibility.import);
            ImGui::MenuItem("Stats", nullptr, &visibility.stats);
            ImGui::Separator();
            if (ImGui::MenuItem("Reset Layout")) {
                settings->ui.dockInitialized = false;
                visibility = PanelVisibility();
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    void renderSettingsModal() {
        if (settings->ui.settingsOpen) {
            ImGui::OpenPopup("Settings");
        }

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_Appearing);

        if (!ImGui::BeginPopupModal("Settings", &settings->ui.settingsOpen)) {
            return;
        }

        if (ImGui::BeginTabBar("SettingsTabs")) {
            if (ImGui::BeginTabItem("General")) {
                ImGui::Text("Velocity Input Mode");
                int mode = static_cast<int>(settings->ui.velocityMode);
                ImGui::RadioButton("XY", &mode, static_cast<int>(VelocityInputMode::XY));
                ImGui::SameLine();
                ImGui::RadioButton("Polar", &mode, static_cast<int>(VelocityInputMode::Polar));
                settings->ui.velocityMode = static_cast<VelocityInputMode>(mode);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Style")) {
                ImGui::ShowStyleEditor();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::EndPopup();
    }

    void drawSourceOverlay(ImVec2 origin, ImVec2 size, const std::vector<FluidSource>& sources) {
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        for (const FluidSource& source : sources) {
            ImVec2 pos{origin.x + source.x * size.x, origin.y + (1.f - source.y) * size.y};

            float alpha = source.active ? 1.0f : 0.35f;
            ImU32 col = ImGui::ColorConvertFloat4ToU32(
                ImVec4(source.color[0], source.color[1], source.color[2], alpha));

            ImVec2 dir{source.vx, -source.vy};
            float speed = std::hypot(dir.x, dir.y);
            ImVec2 normDir = speed > 1e-6f ? ImVec2{dir.x / speed, dir.y / speed} : ImVec2{1.f, 0.f};

            if (source.form == FluidSource::Form::CIRCLE) {
                drawList->AddCircle(pos, source.radius * size.y, col, 0, 1.5f);
            }
            else {
                ImVec2 perp{-normDir.y, normDir.x};
                float halfLen = source.radius * size.y;
                ImVec2 a{pos.x - perp.x * halfLen, pos.y - perp.y * halfLen};
                ImVec2 b{pos.x + perp.x * halfLen, pos.y + perp.y * halfLen};
                drawList->AddLine(a, b, col, 2.5f);
            }

            drawList->AddCircleFilled(pos, 4.0f, col);

            if (speed > 1e-6f) {
                constexpr float kMinLen = 0.01f;
                constexpr float kMaxLen = 0.1f;
                constexpr float kRefSpeed = 50.0f;

                float t = std::log1p(speed) / std::log1p(kRefSpeed);
                t = std::clamp(t, 0.f, 1.f);
                float arrowLen = std::lerp(kMinLen, kMaxLen, t) * size.y;

                ImVec2 end{pos.x + normDir.x * arrowLen, pos.y + normDir.y * arrowLen};
                drawList->AddLine(pos, end, col, 2.0f);
            }
        }
    }
};
