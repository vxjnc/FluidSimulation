#pragma once
#include <GLFW/glfw3.h>
#include <imgui.h>
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

        ImGuiSettingsHandler handler{};
        handler.TypeName = "FluidSim";
        handler.TypeHash = ImHashStr("FluidSim");
        handler.ReadOpenFn = [](ImGuiContext*, ImGuiSettingsHandler*, const char*) -> void* {
            return (void*)1;
        };

        handler.ReadLineFn = [](ImGuiContext*, ImGuiSettingsHandler* h, void*, const char* line) {
            auto* self = static_cast<ImGuiManager*>(h->UserData);
            int v = 0;
            if (sscanf(line, "controls=%d", &v) == 1) {
                self->visibility.controls = v;
            }
            if (sscanf(line, "randomSplat=%d", &v) == 1) {
                self->visibility.randomSplat = v;
            }
            if (sscanf(line, "import=%d", &v) == 1) {
                self->visibility.import = v;
            }
            if (sscanf(line, "dockInitialized=%d", &v) == 1) {
                self->dockInitialized = v;
            }
        };
        handler.WriteAllFn = [](ImGuiContext*, ImGuiSettingsHandler* h, ImGuiTextBuffer* buf) {
            auto* self = static_cast<ImGuiManager*>(h->UserData);
            buf->appendf("[FluidSim][Visibility]\n");
            buf->appendf("controls=%d\n", self->visibility.controls ? 1 : 0);
            buf->appendf("randomSplat=%d\n", self->visibility.randomSplat ? 1 : 0);
            buf->appendf("import=%d\n", self->visibility.import ? 1 : 0);
            buf->appendf("dockInitialized=%d\n", self->dockInitialized ? 1 : 0);
        };
        handler.UserData = this;
        ImGui::AddSettingsHandler(&handler);

        ImGui_ImplGlfw_InitForOther(window, true);

        applyStyle();

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

    void applyStyle() {
        auto& style{ImGui::GetStyle()};
        // Borders
        style.WindowBorderSize = 3.0f;

        // Rounding
        style.FrameRounding = 3.0f;
        style.PopupRounding = 3.0f;
        style.ScrollbarRounding = 3.0f;
        style.GrabRounding = 3.0f;

        // Docking
        style.DockingSeparatorSize = 3.0f;

        {
            constexpr auto ToRGBA = [](uint32_t argb) constexpr {
                ImVec4 color{};
                color.x = ((argb >> 16) & 0xFF) / 255.0f;
                color.y = ((argb >> 8) & 0xFF) / 255.0f;
                color.z = (argb & 0xFF) / 255.0f;
                color.w = ((argb >> 24) & 0xFF) / 255.0f;
                return color;
            };

            constexpr auto Lerp = [](const ImVec4& a, const ImVec4& b, float t) constexpr {
                return ImVec4{std::lerp(a.x, b.y, t), std::lerp(a.y, b.y, t), std::lerp(a.z, b.z, t),
                              std::lerp(a.w, b.w, t)};
            };

            auto colors{style.Colors};
            colors[ImGuiCol_Text] = ToRGBA(0xFFABB2BF);
            colors[ImGuiCol_TextDisabled] = ToRGBA(0xFF565656);
            colors[ImGuiCol_WindowBg] = ToRGBA(0xFF282C34);
            colors[ImGuiCol_ChildBg] = ToRGBA(0xFF21252B);
            colors[ImGuiCol_PopupBg] = ToRGBA(0xFF2E323A);
            colors[ImGuiCol_Border] = ToRGBA(0xFF2E323A);
            colors[ImGuiCol_BorderShadow] = ToRGBA(0x00000000);
            colors[ImGuiCol_FrameBg] = colors[ImGuiCol_ChildBg];
            colors[ImGuiCol_FrameBgHovered] = ToRGBA(0xFF484C52);
            colors[ImGuiCol_FrameBgActive] = ToRGBA(0xFF54575D);
            colors[ImGuiCol_TitleBg] = colors[ImGuiCol_WindowBg];
            colors[ImGuiCol_TitleBgActive] = colors[ImGuiCol_FrameBgActive];
            colors[ImGuiCol_TitleBgCollapsed] = ToRGBA(0x8221252B);
            colors[ImGuiCol_MenuBarBg] = colors[ImGuiCol_ChildBg];
            colors[ImGuiCol_ScrollbarBg] = colors[ImGuiCol_PopupBg];
            colors[ImGuiCol_ScrollbarGrab] = ToRGBA(0xFF3E4249);
            colors[ImGuiCol_ScrollbarGrabHovered] = ToRGBA(0xFF484C52);
            colors[ImGuiCol_ScrollbarGrabActive] = ToRGBA(0xFF54575D);
            colors[ImGuiCol_CheckMark] = colors[ImGuiCol_Text];
            colors[ImGuiCol_SliderGrab] = ToRGBA(0xFF353941);
            colors[ImGuiCol_SliderGrabActive] = ToRGBA(0xFF7A7A7A);
            colors[ImGuiCol_Button] = colors[ImGuiCol_SliderGrab];
            colors[ImGuiCol_ButtonHovered] = colors[ImGuiCol_FrameBgActive];
            colors[ImGuiCol_ButtonActive] = colors[ImGuiCol_ScrollbarGrabActive];
            colors[ImGuiCol_Header] = colors[ImGuiCol_ChildBg];
            colors[ImGuiCol_HeaderHovered] = ToRGBA(0xFF353941);
            colors[ImGuiCol_HeaderActive] = colors[ImGuiCol_FrameBgActive];
            colors[ImGuiCol_Separator] = colors[ImGuiCol_FrameBgActive];
            colors[ImGuiCol_SeparatorHovered] = ToRGBA(0xFF3E4452);
            colors[ImGuiCol_SeparatorActive] = colors[ImGuiCol_SeparatorHovered];
            colors[ImGuiCol_ResizeGrip] = colors[ImGuiCol_Separator];
            colors[ImGuiCol_ResizeGripHovered] = colors[ImGuiCol_SeparatorHovered];
            colors[ImGuiCol_ResizeGripActive] = colors[ImGuiCol_SeparatorActive];
            colors[ImGuiCol_InputTextCursor] = ToRGBA(0xFF528BFF);
            colors[ImGuiCol_TabHovered] = colors[ImGuiCol_HeaderHovered];
            colors[ImGuiCol_Tab] = colors[ImGuiCol_FrameBgActive];
            colors[ImGuiCol_TabSelected] = colors[ImGuiCol_HeaderHovered];
            colors[ImGuiCol_TabSelectedOverline] = colors[ImGuiCol_HeaderActive];
            colors[ImGuiCol_TabDimmed] = Lerp(colors[ImGuiCol_Tab], colors[ImGuiCol_TitleBg], 0.80f);
            colors[ImGuiCol_TabDimmedSelected] =
                Lerp(colors[ImGuiCol_TabSelected], colors[ImGuiCol_TitleBg], 0.40f);
            colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4{0.50f, 0.50f, 0.50f, 0.00f};
            colors[ImGuiCol_DockingPreview] = colors[ImGuiCol_ChildBg];
            colors[ImGuiCol_DockingEmptyBg] = colors[ImGuiCol_WindowBg];
            colors[ImGuiCol_PlotLines] = ImVec4{0.61f, 0.61f, 0.61f, 1.00f};
            colors[ImGuiCol_PlotLinesHovered] = ImVec4{1.00f, 0.43f, 0.35f, 1.00f};
            colors[ImGuiCol_PlotHistogram] = ImVec4{0.90f, 0.70f, 0.00f, 1.00f};
            colors[ImGuiCol_PlotHistogramHovered] = ImVec4{1.00f, 0.60f, 0.00f, 1.00f};
            colors[ImGuiCol_TableHeaderBg] = colors[ImGuiCol_ChildBg];
            colors[ImGuiCol_TableBorderStrong] = colors[ImGuiCol_SliderGrab];
            colors[ImGuiCol_TableBorderLight] = colors[ImGuiCol_FrameBgActive];
            colors[ImGuiCol_TableRowBg] = ImVec4{0.00f, 0.00f, 0.00f, 0.00f};
            colors[ImGuiCol_TableRowBgAlt] = ImVec4{1.00f, 1.00f, 1.00f, 0.06f};
            colors[ImGuiCol_TextLink] = ToRGBA(0xFF3F94CE);
            colors[ImGuiCol_TextSelectedBg] = ToRGBA(0xFF243140);
            colors[ImGuiCol_TreeLines] = colors[ImGuiCol_Text];
            colors[ImGuiCol_DragDropTarget] = colors[ImGuiCol_Text];
            colors[ImGuiCol_NavCursor] = colors[ImGuiCol_TextLink];
            colors[ImGuiCol_NavWindowingHighlight] = colors[ImGuiCol_Text];
            colors[ImGuiCol_NavWindowingDimBg] = ImVec4{0.80f, 0.80f, 0.80f, 0.20f};
            colors[ImGuiCol_ModalWindowDimBg] = ToRGBA(0xC821252B);
        };
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
            ImGui::DockBuilderGetNode(dockLeft)->LocalFlags |= ImGuiDockNodeFlags_AutoHideTabBar;

            ImGui::DockBuilderDockWindow("Viewport", dockRight);
            ImGui::DockBuilderGetNode(dockRight)->LocalFlags |= ImGuiDockNodeFlags_AutoHideTabBar |
                                                                ImGuiDockNodeFlags_NoDockingOverMe |
                                                                ImGuiDockNodeFlags_NoUndocking;

            ImGui::DockBuilderDockWindow("Random Splat", dockLeft);

            ImGui::DockBuilderDockWindow("Import", dockLeft);

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
