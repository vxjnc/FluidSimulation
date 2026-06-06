#pragma once
#include <GLFW/glfw3.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_wgpu.h>
#include <imgui_internal.h>
#include <webgpu/webgpu.hpp>

#include "src/app_settings.hpp"
#include "src/compute/fluid_sim.hpp"
#include "src/render/render_settings.hpp"
#include "src/ui/controls/source_widget.hpp"
#include "src/ui/fluid_viewport.hpp"
#include "src/wgpu_context.hpp"

struct MouseState {
    float x = 0, y = 0;
    float dx = 0, dy = 0;
    bool rightPressed = false;
    bool leftPressed = false;

    bool leftJustPressed = false;
};

class ImGuiManager {
public:
    void init(GLFWwindow* window, wgpu::Device device, wgpu::TextureFormat surfaceFormat) {
        ImGui::CreateContext();
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;
        ImGui::GetIO().IniFilename = nullptr;
        ImGui_ImplGlfw_InitForOther(window, true);

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

    void renderUI(FluidViewport& viewport, MouseState& mouse, FluidSim& sim, AppSettings& settings) {
        ImGuiIO& io = ImGui::GetIO();

        ImGuiID dockspaceId = ImGui::DockSpaceOverViewport(ImGui::GetMainViewport()->ID);

        if (!dockInitialized_) {
            dockInitialized_ = true;

            ImGui::DockBuilderRemoveNode(dockspaceId);
            ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->Size);

            ImGuiID dockLeft, dockRight;
            ImGui::DockBuilderSplitNode(dockspaceId, ImGuiDir_Left, 0.22f, &dockLeft, &dockRight);

            ImGui::DockBuilderDockWindow("Controls", dockLeft);
            ImGui::DockBuilderDockWindow("Viewport", dockRight);
            ImGui::DockBuilderFinish(dockspaceId);
        }

        // --- Controls Panel ---
        ImGui::Begin("Controls");
        {
            ImGui::Text("FPS: %.1f", io.Framerate);
            ImGui::Text("Sim size: %ux%u = %u", sim.state.width, sim.state.height,
                        sim.state.width * sim.state.height);
            ImGui::Text("Dye size: %ux%u = %u", sim.state.dye_width, sim.state.dye_height,
                        sim.state.dye_width * sim.state.dye_height);

            ImGui::Separator();
            ImGui::Text("Simulation settings");

            float prevScale = settings.simScale;
            if (ImGui::SliderFloat("Sim Scale", &settings.simScale, 0.1f, 1.0f)) {
                float ratio = settings.simScale / prevScale;
                for (auto& s : sim.sources) {
                    s.x *= ratio;
                    s.y *= ratio;
                    s.vx *= ratio;
                    s.vy *= ratio;
                    s.radius *= ratio;
                }
                sim.resizeWithResample(
                    static_cast<uint32_t>(static_cast<float>(viewport.w) * settings.simScale),
                    static_cast<uint32_t>(static_cast<float>(viewport.h) * settings.simScale));
            }

            if (ImGui::SliderFloat("Dye Scale", &settings.dyeScale, settings.simScale, 1.0f)) {
                sim.resizeWithResample(
                    sim.state.width, sim.state.height,
                    static_cast<uint32_t>(static_cast<float>(viewport.w) * settings.dyeScale),
                    static_cast<uint32_t>(static_cast<float>(viewport.h) * settings.dyeScale));
            }

            ImGui::SliderFloat("Sim dt", &settings.dt, 0.001f, 0.1f);
            ImGui::SliderFloat("Velocity Dissipation", &settings.velDissipation, 0.0f, 4.0f);
            ImGui::SliderFloat("Dye Dissipation", &settings.dyeDissipation, 0.0f, 4.0f);
            ImGui::SliderFloat("Curl Strength", &settings.curlStrength, 0.0f, 50.0f);

            if (ImGui::Button("Clear")) {
                sim.state.clear();
            }

            ImGui::Separator();

            ImGui::Text("Render Mode");
            static const char* modes[] = {"Dye", "Velocity", "Pressure", "Divergence", "Curl"};
            int current = static_cast<int>(settings.renderSettings.mode);
            if (ImGui::Combo("##render_mode", &current, modes, sizeof(modes) / sizeof(modes[0]))) {
                settings.renderSettings.mode = static_cast<RenderMode>(current);
            }

            ImGui::Checkbox("Show Obstacles", &settings.renderSettings.showObstacles);

            ImGui::Separator();

            ImGui::Text("Brush");
            static const char* brushModes[] = {"Inject", "Paint Wall"};
            int bm = static_cast<int>(settings.brushMode);
            if (ImGui::Combo("##brush", &bm, brushModes, sizeof(brushModes) / sizeof(brushModes[0]))) {
                settings.brushMode = static_cast<BrushMode>(bm);
            }
            ImGui::SliderFloat("Radius", &settings.brushRadius, 1.0f, 100.0f);
            if (settings.brushMode == BrushMode::Inject) {
                ImGui::SliderFloat("Strength", &settings.brushStrength, 1.0f, 100.0f);
            }

            ImGui::Separator();

            ImGui::Text("Sources");

            for (size_t i = 0; i < sim.sources.size();) {
                if (sourceWidget_.render(sim.sources[i], i, viewport, settings)) {
                    ++i;
                }
                else {
                    sim.sources.erase(i);
                }
            }

            if (ImGui::Button("Add Source")) {
                sim.sources.add(FluidSource(static_cast<float>(viewport.w) / 2.f * settings.simScale,
                                            static_cast<float>(viewport.h) / 2.f * settings.simScale, 0, 100,
                                            10 * settings.simScale, std::array{1.f, 1.f, 1.f}));
            }
        }
        ImGui::End();

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

    bool dockInitialized_ = false;

    void endFrame(WGPURenderPassEncoder passEncoder) {
        ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), passEncoder);
    }

private:
    SourceWidget sourceWidget_;
};
