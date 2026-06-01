#pragma once
#include <GLFW/glfw3.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_wgpu.h>
#include <webgpu/webgpu.hpp>

#include "src/app_settings.hpp"
#include "src/compute/fluid_sim.hpp"
#include "src/render/render_settings.hpp"
#include "src/ui/fluid_viewport.hpp"
#include "src/wgpu_context.hpp"

struct MouseState {
    double x = 0, y = 0;
    double dx = 0, dy = 0;
    bool rightPressed = false;
    bool leftPressed = false;
};

class ImGuiManager {
public:
    void init(GLFWwindow* window, wgpu::Device device, wgpu::TextureFormat surfaceFormat) {
        ImGui::CreateContext();
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
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
        float screen_w = io.DisplaySize.x;
        float screen_h = io.DisplaySize.y;

        // --- Controls Panel ---
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(panelWidth_, screen_h));
        ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
        {
            ImGui::Text("FPS: %.1f", io.Framerate);
            ImGui::Text("Sim size: %ux%u = %u", sim.state.width, sim.state.height, sim.state.width * sim.state.height);

            ImGui::Separator();

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
                sim.resizeWithResample(static_cast<uint32_t>(viewport.w * settings.simScale),
                                       static_cast<uint32_t>(viewport.h * settings.simScale));
            }

            if (ImGui::SliderFloat("Sim dt", &settings.dt, 0.001f, 0.1f)) {
                // pass
            }

            if (ImGui::Button("Clear")) {
                sim.state.clear();
            }

            ImGui::Separator();

            ImGui::Text("Render Mode");
            static const char* modes[] = {"Dye", "Velocity", "Pressure", "Divergence"};
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
            ImGui::SliderFloat("Radius", &settings.brushRadius, 1.0f, 50.0f);
            if (settings.brushMode == BrushMode::Inject) {
                ImGui::SliderFloat("Strength", &settings.brushStrength, 1.0f, 100.0f);
            }

            ImGui::Separator();

            ImGui::Text("Sources");

            for (size_t i = 0; i < sim.sources.size(); ++i) {
                FluidSource& s = sim.sources[i];
                ImGui::PushID(static_cast<int>(i));

                ImGui::Checkbox("##active", &s.active);
                ImGui::SameLine();
                ImGui::Text("Source %zu", i + 1);

                float display_x = s.x / settings.simScale;
                if (ImGui::SliderFloat("X", &display_x, 0.0f, static_cast<float>(viewport.w))) {
                    s.x = display_x * settings.simScale;
                }

                float display_y = s.y / settings.simScale;
                if (ImGui::SliderFloat("Y", &display_y, 0.0f, static_cast<float>(viewport.h))) {
                    s.y = display_y * settings.simScale;
                }

                float display_vx = s.vx / settings.simScale;
                if (ImGui::SliderFloat("VX", &display_vx, -200.0f, 200.0f)) {
                    s.vx = display_vx * settings.simScale;
                }

                float display_vy = s.vy / settings.simScale;
                if (ImGui::SliderFloat("VY", &display_vy, -200.0f, 200.0f)) {
                    s.vy = display_vy * settings.simScale;
                }

                float display_r = s.radius / settings.simScale;
                if (ImGui::SliderFloat("Radius", &display_r, 1.0f, 20.0f)) {
                    s.radius = display_r * settings.simScale;
                }

                if (ImGui::Button("Remove")) {
                    sim.sources.erase(std::next(sim.sources.begin(), i));
                    ImGui::PopID();
                    break;
                }

                ImGui::PopID();
                ImGui::Separator();
            }

            if (ImGui::Button("Add Source")) {
                sim.sources.emplace_back(static_cast<float>(viewport.w) / 2.f * settings.simScale,
                                         static_cast<float>(viewport.h) / 2.f * settings.simScale, 0, 100, 4 * settings.simScale);
            }
        }
        ImGui::End();

        // --- Viewport Panel ---
        ImGui::SetNextWindowPos(ImVec2(panelWidth_, 0));
        ImGui::SetNextWindowSize(ImVec2(screen_w - panelWidth_, screen_h));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Viewport", nullptr,
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
        {
            ImVec2 size = ImGui::GetContentRegionAvail();

            if (size.x > 0.0f && size.y > 0.0f) {
                uint32_t vw = static_cast<uint32_t>(size.x);
                uint32_t vh = static_cast<uint32_t>(size.y);

                if (vw != viewport.w || vh != viewport.h) {
                    viewport.init(WGPUContext::instance().device(), vw, vh, WGPUContext::instance().surfaceFormat());
                    sim.resizeWithResample(static_cast<uint32_t>(static_cast<float>(vw) * settings.simScale),
                                           static_cast<uint32_t>(static_cast<float>(vh) * settings.simScale));
                }

                if (ImGui::IsWindowHovered()) {
                    ImVec2 origin = ImGui::GetCursorScreenPos();
                    ImVec2 mpos = io.MousePos;
                    mouse.dx = io.MouseDelta.x;
                    mouse.dy = io.MouseDelta.y;
                    mouse.x = mpos.x - origin.x;
                    mouse.y = mpos.y - origin.y;
                    mouse.leftPressed = ImGui::IsMouseDown(ImGuiMouseButton_Left);
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

    void endFrame(WGPURenderPassEncoder passEncoder) { ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), passEncoder); }

    float panelWidth() const { return panelWidth_; }

private:
    static constexpr float panelWidth_ = 280.f;
};
