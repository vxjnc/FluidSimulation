#pragma once
#include <cstdint>
#include <vector>

#include <GLFW/glfw3.h>
#include <nfd.hpp>
#include <webgpu/webgpu-raii.hpp>

#include "src/app_settings.hpp"
#include "src/compute/fluid_sim.hpp"
#include "src/compute/gpu_profiler.hpp"
#include "src/render/render.hpp"
#include "src/save/save_manager.hpp"
#include "src/scripting/scripting_engine.hpp"
#include "src/ui/fluid_viewport.hpp"
#include "src/ui/imgui_manager.hpp"
#include "src/utils/deffered_queue.hpp"

class Application {
public:
    Application(uint32_t width, uint32_t height, std::string_view title);

    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void run();
    template <typename Self> auto&& getSources(this Self&& self) { return std::forward<Self>(self).sources; }

private:
    DeferredQueue preSimQueue_;
    DeferredQueue postSubmitQueue_;

    void processInput(wgpu::raii::CommandEncoder& enc, std::vector<FluidSource>& frameSources);

    void update(wgpu::raii::CommandEncoder& enc, std::span<const FluidSource> frameSources);

    std::pair<wgpu::raii::Texture, wgpu::raii::TextureView> render(wgpu::raii::CommandEncoder& enc);

    GpuProfiler<> uiProfiler;

    ScriptingEngine scripting;

    GLFWwindow* window = nullptr;
    [[maybe_unused]] NFD::Guard nfdGuard;

    AppSettings settings;

    FluidSim simulation;
    std::vector<FluidSource> sources;

    Render renderer;
    FluidViewport viewport;
    ImGuiManager imguiManager;
    MouseState mouse;
};
