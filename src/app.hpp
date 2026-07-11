#pragma once
#include <cstdint>
#include <vector>

#include <GLFW/glfw3.h>
#include <nfd.hpp>
#include <webgpu/webgpu-raii.hpp>

#include "src/app_settings.hpp"
#include "src/compute/fluid_sim.hpp"
#include "src/compute/gpu_profiler.hpp"
#include "src/notification_manager.hpp"
#include "src/render/render.hpp"
#include "src/save/save_manager.hpp"
#include "src/scripting/plugin_manager.hpp"
#include "src/scripting/script_source.hpp"
#include "src/scripting/scripting_engine.hpp"
#include "src/scripting/scripting_host.hpp"
#include "src/scripting/scripting_loader.hpp"
#include "src/ui/fluid_viewport.hpp"
#include "src/ui/imgui_manager.hpp"
#include "src/utils/deffered_queue.hpp"

class Application : public ScriptHost {
public:
    Application(uint32_t width, uint32_t height, std::string_view title);

    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void run();
    std::vector<FluidSource>& getSources() override { return sources; }
    const std::vector<FluidSource>& getSources() const { return sources; }

    NotificationManager& notifications() override { return notificationManager; }
    const NotificationManager& notifications() const { return notificationManager; }

    uint32_t dyeWidth() const override { return simulation.state.dye_width; }
    uint32_t dyeHeight() const override { return simulation.state.dye_height; }

    uint32_t simWidth() const override { return simulation.state.width; }
    uint32_t simHeight() const override { return simulation.state.height; }

    void setObstacles(std::span<const uint32_t> data) override;

private:
    DeferredQueue preSimQueue_;
    DeferredQueue postSubmitQueue_;

    void processInput(wgpu::raii::CommandEncoder& enc, std::vector<FluidSource>& frameSources);

    void update(wgpu::raii::CommandEncoder& enc, std::span<const FluidSource> frameSources);

    std::pair<wgpu::raii::Texture, wgpu::raii::TextureView> render(wgpu::raii::CommandEncoder& enc);

    GpuProfiler<> uiProfiler;

    ScriptingLoader scripting;
    PluginManager pluginManager;

    NotificationManager notificationManager;

    GLFWwindow* window = nullptr;
    [[maybe_unused]] NFD::Guard nfdGuard;

    AppSettings settings;

    FluidSim simulation;
    std::vector<FluidSource> sources;
    std::vector<ScriptSource> scripts;

    Render renderer;
    FluidViewport viewport;
    ImGuiManager imguiManager;
    MouseState mouse;
};
