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
        handler.ReadLineFn = ImguiReadLine;
        handler.WriteAllFn = ImguiWriteAll;
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

        if (menuBarVisible) {
            renderMenuBar();
        }

        renderSettingsModal();

        if (visibility.stats) {
            statsPanel.render(visibility.stats, sim, render);
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
                                         uint32_t vw = sim.state.width;
                                         uint32_t vh = sim.state.height;
                                         auto pixels = ImageProcessor::resizeRGBA(img.pixels.data(), img.w,
                                                                                  img.h, vw, vh, true);
                                         onImport(ImportTarget::Obstacles, std::move(pixels), vw, vh);
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
    StatsPanel statsPanel;

    bool menuBarVisible = true;
    bool settingsOpen = false;
    bool dockInitialized = false;

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
                settingsOpen = true;
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
                dockInitialized = false;
                visibility = PanelVisibility();
            }
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    void renderSettingsModal() {
        if (settingsOpen) {
            ImGui::OpenPopup("Settings");
        }

        ImVec2 center = ImGui::GetMainViewport()->GetCenter();
        ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_Appearing);

        if (!ImGui::BeginPopupModal("Settings", &settingsOpen)) {
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

    static void ImguiReadLine(ImGuiContext*, ImGuiSettingsHandler* h, void* entry_data, const char* line) {
        auto* self = static_cast<ImGuiManager*>(h->UserData);
        if (!entry_data) {
            return;
        }

        auto section_hash = static_cast<ImGuiID>((intptr_t)entry_data);
        int v = 0;
        float f = 0.0f;
        float f3[3] = {0.0f, 0.0f, 0.0f};

        if (section_hash == ImHashStr("Visibility")) {
            if (std::sscanf(line, "controls=%d", &v) == 1) {
                self->visibility.controls = (v != 0);
            }
            else if (std::sscanf(line, "randomSplat=%d", &v) == 1) {
                self->visibility.randomSplat = (v != 0);
            }
            else if (std::sscanf(line, "import=%d", &v) == 1) {
                self->visibility.import = (v != 0);
            }
            else if (std::sscanf(line, "stats=%d", &v) == 1) {
                self->visibility.stats = (v != 0);
            }
        }
        else if (section_hash == ImHashStr("AppSettings")) {
            if (std::sscanf(line, "paused=%d", &v) == 1) {
                self->settings->paused = (v != 0);
            }
            else if (std::sscanf(line, "dt=%f", &f) == 1) {
                self->settings->dt = f;
            }
            else if (std::sscanf(line, "velDissipation=%f", &f) == 1) {
                self->settings->velDissipation = f;
            }
            else if (std::sscanf(line, "dyeDissipation=%f", &f) == 1) {
                self->settings->dyeDissipation = f;
            }
            else if (std::sscanf(line, "curlStrength=%f", &f) == 1) {
                self->settings->curlStrength = f;
            }
            else if (std::sscanf(line, "brushMode=%d", &v) == 1) {
                self->settings->brushMode = static_cast<BrushMode>(v);
            }
            else if (std::sscanf(line, "brushModeMask=%d", &v) == 1) {
                self->settings->brushModeMask = v;
            }
            else if (std::sscanf(line, "brushRadius=%f", &f) == 1) {
                self->settings->brushRadius = f;
            }
            else if (std::sscanf(line, "brushStrength=%f", &f) == 1) {
                self->settings->brushStrength = f;
            }
            else if (std::sscanf(line, "brushColor=%f,%f,%f", &f3[0], &f3[1], &f3[2]) == 3) {
                self->settings->brushColor[0] = f3[0];
                self->settings->brushColor[1] = f3[1];
                self->settings->brushColor[2] = f3[2];
            }
            else if (std::sscanf(line, "simScale=%f", &f) == 1) {
                self->settings->simScale = f;
            }
            else if (std::sscanf(line, "dyeScale=%f", &f) == 1) {
                self->settings->dyeScale = f;
            }
            else if (std::sscanf(line, "dockInitialized=%d", &v) == 1) {
                self->dockInitialized = (v != 0);
            }
        }
        else if (section_hash == ImHashStr("RenderSettings")) {
            if (std::sscanf(line, "mode=%d", &v) == 1) {
                self->settings->renderSettings.mode = static_cast<RenderMode>(v);
            }
            else if (std::sscanf(line, "showObstacles=%d", &v) == 1) {
                self->settings->renderSettings.showObstacles = (v != 0);
            }
        }
        else if (section_hash == ImHashStr("SplatSettings")) {
            if (std::sscanf(line, "countMin=%d", &v) == 1) {
                self->settings->splatSettings.countMin = v;
            }
            else if (std::sscanf(line, "countMax=%d", &v) == 1) {
                self->settings->splatSettings.countMax = v;
            }
            else if (std::sscanf(line, "radiusMin=%f", &f) == 1) {
                self->settings->splatSettings.radiusMin = f;
            }
            else if (std::sscanf(line, "radiusMax=%f", &f) == 1) {
                self->settings->splatSettings.radiusMax = f;
            }
            else if (std::sscanf(line, "applyVelocity=%d", &v) == 1) {
                self->settings->splatSettings.applyVelocity = (v != 0);
            }
            else if (std::sscanf(line, "vxMin=%f", &f) == 1) {
                self->settings->splatSettings.vxMin = f;
            }
            else if (std::sscanf(line, "vxMax=%f", &f) == 1) {
                self->settings->splatSettings.vxMax = f;
            }
            else if (std::sscanf(line, "vyMin=%f", &f) == 1) {
                self->settings->splatSettings.vyMin = f;
            }
            else if (std::sscanf(line, "vyMax=%f", &f) == 1) {
                self->settings->splatSettings.vyMax = f;
            }
            else if (std::sscanf(line, "angleMin=%f", &f) == 1) {
                self->settings->splatSettings.angleMin = f;
            }
            else if (std::sscanf(line, "angleMax=%f", &f) == 1) {
                self->settings->splatSettings.angleMax = f;
            }
            else if (std::sscanf(line, "magMin=%f", &f) == 1) {
                self->settings->splatSettings.magMin = f;
            }
            else if (std::sscanf(line, "magMax=%f", &f) == 1) {
                self->settings->splatSettings.magMax = f;
            }
            else if (std::sscanf(line, "applyColor=%d", &v) == 1) {
                self->settings->splatSettings.applyColor = (v != 0);
            }
        }
        else if (section_hash == ImHashStr("UISettings")) {
            if (std::sscanf(line, "velocityMode=%d", &v) == 1) {
                self->settings->ui.velocityMode = static_cast<VelocityInputMode>(v);
            }
        }
    }

    static void ImguiWriteAll(ImGuiContext*, ImGuiSettingsHandler* h, ImGuiTextBuffer* buf) {
        auto* self = static_cast<ImGuiManager*>(h->UserData);
        const char* type_name = h->TypeName;

        buf->appendf("[%s][Visibility]\n", type_name);
        buf->appendf("controls=%d\n", self->visibility.controls ? 1 : 0);
        buf->appendf("randomSplat=%d\n", self->visibility.randomSplat ? 1 : 0);
        buf->appendf("import=%d\n", self->visibility.import ? 1 : 0);
        buf->appendf("stats=%d\n", self->visibility.stats ? 1 : 0);
        buf->append("\n");

        buf->appendf("[%s][AppSettings]\n", type_name);
        buf->appendf("paused=%d\n", self->settings->paused ? 1 : 0);
        buf->appendf("dt=%.6f\n", static_cast<float>(self->settings->dt));
        buf->appendf("velDissipation=%.6f\n", static_cast<float>(self->settings->velDissipation));
        buf->appendf("dyeDissipation=%.6f\n", static_cast<float>(self->settings->dyeDissipation));
        buf->appendf("curlStrength=%.6f\n", static_cast<float>(self->settings->curlStrength));
        buf->appendf("brushMode=%d\n", static_cast<int>(self->settings->brushMode));
        buf->appendf("brushModeMask=%d\n", self->settings->brushModeMask);
        buf->appendf("brushRadius=%.6f\n", self->settings->brushRadius);
        buf->appendf("brushStrength=%.6f\n", self->settings->brushStrength);
        buf->appendf("brushColor=%.6f,%.6f,%.6f\n", self->settings->brushColor[0],
                     self->settings->brushColor[1], self->settings->brushColor[2]);
        buf->appendf("simScale=%.6f\n", self->settings->simScale);
        buf->appendf("dyeScale=%.6f\n", self->settings->dyeScale);
        buf->appendf("dockInitialized=%d\n", self->dockInitialized ? 1 : 0);
        buf->append("\n");

        buf->appendf("[%s][RenderSettings]\n", type_name);
        buf->appendf("mode=%d\n", static_cast<int>(self->settings->renderSettings.mode));
        buf->appendf("showObstacles=%d\n", self->settings->renderSettings.showObstacles ? 1 : 0);
        buf->append("\n");

        buf->appendf("[%s][SplatSettings]\n", type_name);
        buf->appendf("countMin=%d\n", self->settings->splatSettings.countMin);
        buf->appendf("countMax=%d\n", self->settings->splatSettings.countMax);
        buf->appendf("radiusMin=%.6f\n", self->settings->splatSettings.radiusMin);
        buf->appendf("radiusMax=%.6f\n", self->settings->splatSettings.radiusMax);
        buf->appendf("applyVelocity=%d\n", self->settings->splatSettings.applyVelocity ? 1 : 0);
        buf->appendf("vxMin=%.6f\n", self->settings->splatSettings.vxMin);
        buf->appendf("vxMax=%.6f\n", self->settings->splatSettings.vxMax);
        buf->appendf("vyMin=%.6f\n", self->settings->splatSettings.vyMin);
        buf->appendf("vyMax=%.6f\n", self->settings->splatSettings.vyMax);
        buf->appendf("angleMin=%.6f\n", self->settings->splatSettings.angleMin);
        buf->appendf("angleMax=%.6f\n", self->settings->splatSettings.angleMax);
        buf->appendf("magMin=%.6f\n", self->settings->splatSettings.magMin);
        buf->appendf("magMax=%.6f\n", self->settings->splatSettings.magMax);
        buf->appendf("applyColor=%d\n", self->settings->splatSettings.applyColor ? 1 : 0);
        buf->append("\n");

        buf->appendf("[%s][UISettings]\n", type_name);
        buf->appendf("velocityMode=%d\n", static_cast<int>(self->settings->ui.velocityMode));
        buf->append("\n");
    }
};
