#pragma once
#include <GLFW/glfw3.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_wgpu.h>
#include <imgui_internal.h>
#include <stb_image_resize2.h>
#include <webgpu/webgpu.hpp>

#include "src/app_settings.hpp"
#include "src/compute/fluid_sim.hpp"
#include "src/ui/controls/controls_panel.hpp"
#include "src/ui/fluid_viewport.hpp"
#include "src/ui/import/import_panel.hpp"
#include "src/ui/random_splat/splat_panel.hpp"
#include "src/wgpu_context.hpp"

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
};

class ImGuiManager {
public:
    void init(GLFWwindow* window, wgpu::Device device, wgpu::TextureFormat surfaceFormat) {
        ImGui::CreateContext();
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
        ImGui_ImplGlfw_InitForOther(window, true);

        ImGuiStyle& style = ImGui::GetStyle();
        style.FrameRounding = 5.f;
        style.ColorMarkerSize = 8.f;

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

    void renderUI(FluidViewport& viewport, MouseState& mouse, FluidSim& sim, AppSettings& settings,
                  std::vector<FluidSource>& sources) {
        ImGuiIO& io = ImGui::GetIO();

        ImGuiID dockspaceId = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);

        if (!dockInitialized) {
            dockInitialized = true;

            ImGui::DockBuilderRemoveNode(dockspaceId);
            ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->Size);

            ImGuiID dockLeft, dockRight;
            ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.25f, &dockLeft, &dockRight);

            ImGui::DockBuilderDockWindow("Controls", dockLeft);
            ImGui::DockBuilderGetNode(dockLeft)->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
            ImGui::DockBuilderDockWindow("Viewport", dockRight);
            ImGui::DockBuilderGetNode(dockRight)->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
            ImGui::DockBuilderFinish(dockspaceId);
        }

        if (menuBarVisible) {
            renderMenuBar();
        }

        renderSettingsModal(settings);

        if (visibility.controls) {
            controlsPanel.render(visibility.controls, viewport, sim, settings, sources);
        }
        if (visibility.randomSplat) {
            splatPanel.render(visibility.randomSplat, settings.splatSettings, viewport,
                              settings.ui.velocityMode);
        }
        if (visibility.import) {
            if (auto dye = importPanel.render(visibility.import)) {
                uint32_t dw = sim.state.dye_width;
                uint32_t dh = sim.state.dye_height;

                std::vector<float> resized(dw * dh * 4);
                stbir_resize_float_linear(dye->pixels.data(), static_cast<int>(dye->w),
                                          static_cast<int>(dye->h), 0, resized.data(), static_cast<int>(dw),
                                          static_cast<int>(dh), 0, STBIR_RGBA);
                for (uint32_t y = 0; y < dh / 2; ++y) {
                    float* row1 = resized.data() + y * dw * 4;
                    float* row2 = resized.data() + (dh - 1 - y) * dw * 4;
                    std::swap_ranges(row1, row1 + dw * 4, row2);
                }

                dyeToApply = std::move(resized);
                dyeToApplyW = dw;
                dyeToApplyH = dh;
            }
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
                    sim.resizeWithResample(static_cast<uint32_t>(static_cast<float>(vw) * settings.simScale),
                                           static_cast<uint32_t>(static_cast<float>(vh) * settings.simScale),
                                           static_cast<uint32_t>(static_cast<float>(vw) * settings.dyeScale),
                                           static_cast<uint32_t>(static_cast<float>(vh) * settings.dyeScale));
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
                }
                else {
                    mouse.leftPressed = false;
                    mouse.dx = mouse.dy = 0;
                }

                ImGui::Image(viewport.texId, size);
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

    bool menuBarVisible = true;
    bool settingsOpen = false;
    bool dockInitialized = false;

    bool screenshotRequested = false;
    bool saveScreenshotRequested = false;

    std::optional<std::vector<float>> dyeToApply;
    uint32_t dyeToApplyW = 0, dyeToApplyH = 0;

private:
    void renderMenuBar() {
        if (!ImGui::BeginMainMenuBar()) {
            return;
        }

        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save Screenshot")) {
                saveScreenshotRequested = true;
            }
            if (ImGui::MenuItem("Copy to Clipboard", "F12")) {
                screenshotRequested = true;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Settings")) {
                settingsOpen = true;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Controls", nullptr, &visibility.controls);
            ImGui::MenuItem("Random Splat", nullptr, &visibility.randomSplat);
            ImGui::MenuItem("Import", nullptr, &visibility.import);
            ImGui::Separator();
            if (ImGui::MenuItem("Reset Layout")) {
                dockInitialized = false;
                visibility = PanelVisibility();
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    void renderSettingsModal(AppSettings& settings) {
        if (settingsOpen) {
            ImGui::OpenPopup("Settings");
        }

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(300, 120), ImGuiCond_Appearing);

        if (!ImGui::BeginPopupModal("Settings", &settingsOpen, ImGuiWindowFlags_NoResize)) {
            return;
        }

        ImGui::Text("Velocity Input Mode");
        int mode = static_cast<int>(settings.ui.velocityMode);
        ImGui::RadioButton("XY", &mode, static_cast<int>(VelocityInputMode::XY));
        ImGui::SameLine();
        ImGui::RadioButton("Polar", &mode, static_cast<int>(VelocityInputMode::Polar));
        settings.ui.velocityMode = static_cast<VelocityInputMode>(mode);

        ImGui::EndPopup();
    }
};
