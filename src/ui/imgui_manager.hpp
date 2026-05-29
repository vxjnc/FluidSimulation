#pragma once
#include <GLFW/glfw3.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_wgpu.h>
#include <webgpu/webgpu.hpp>

#include "src/compute/fluid_sim.hpp"
#include "src/ui/fluid_viewport.hpp"
#include "src/wgpu_context.hpp"

struct MouseState {
    double x = 0, y = 0;
    double dx = 0, dy = 0;
    bool pressed = false;
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

    void renderUI(FluidViewport& viewport, MouseState& mouse, FluidSim& sim) {
        ImGuiIO& io = ImGui::GetIO();
        float screen_w = io.DisplaySize.x;
        float screen_h = io.DisplaySize.y;

        // --- Controls Panel ---
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(m_panelWidth), screen_h));
        ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
        {
            ImGui::Text("FPS: %.1f", io.Framerate);
            ImGui::Text("Sim size: %ux%u = %u", viewport.w, viewport.h, viewport.w * viewport.h);
        }
        ImGui::End();

        // --- Viewport Panel ---
        ImGui::SetNextWindowPos(ImVec2(static_cast<float>(m_panelWidth), 0));
        ImGui::SetNextWindowSize(ImVec2(screen_w - static_cast<float>(m_panelWidth), screen_h));

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
                    sim.resize(vw, vh);
                }

                if (ImGui::IsWindowHovered()) {
                    ImVec2 origin = ImGui::GetCursorScreenPos();
                    ImVec2 mpos = io.MousePos;
                    mouse.dx = io.MouseDelta.x;
                    mouse.dy = io.MouseDelta.y;
                    mouse.x = mpos.x - origin.x;
                    mouse.y = mpos.y - origin.y;
                    mouse.pressed = ImGui::IsMouseDown(ImGuiMouseButton_Left);
                }
                else {
                    mouse.pressed = false;
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

    uint32_t panelWidth() const { return m_panelWidth; }

private:
    static constexpr uint32_t m_panelWidth = 280;
};
