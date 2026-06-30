#pragma once

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_wgpu.h>
#include <imgui_internal.h>
#include <sigslot/signal.hpp>
#include <stb_image_resize2.h>
#include <webgpu/webgpu.hpp>

#include "src/app_settings.hpp"
#include "src/compute/fluid_sim.hpp"
#include "src/scripting/scripting_engine.hpp"
#include "src/ui/controls/controls_panel.hpp"
#include "src/ui/fluid_viewport.hpp"
#include "src/ui/imgui_serialization.hpp"
#include "src/ui/import/import_panel.hpp"
#include "src/ui/random_splat/splat_panel.hpp"
#include "src/ui/script/plugin/plugins_panel.hpp"
#include "src/ui/script/script_ide.hpp"
#include "src/ui/stats/stats_panel.hpp"

enum class ImportTarget : uint8_t { Dye, Velocity, Obstacles };

class PluginManager;

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
    bool scriptIDE = false;
    bool plugins = false;
    Observable<bool> stats = true;
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
              AppSettings* settings);

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
                  std::vector<FluidSource>& sources, const GpuProfiler<>& uiProfiler,
                  ScriptingEngine& scripting, std::vector<ScriptSource>& scripts,
                  PluginManager& pluginManager);

    void endFrame(WGPURenderPassEncoder passEncoder) {
        ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), passEncoder);
    }

    PanelVisibility visibility;
    ControlsPanel controlsPanel;
    SplatPanel splatPanel;
    ImportPanel importPanel;
    StatsPanel statsPanel;

    ScriptIDE scriptIDE;
    PluginsPanel pluginsPanel;

private:
    AppSettings* settings;

    void renderMenuBar(ScriptingEngine& engine);

    void renderSettingsModal();
    void renderAboutModal();

    void drawSourceOverlay(ImVec2 origin, ImVec2 size, std::span<const FluidSource> sources);
};
