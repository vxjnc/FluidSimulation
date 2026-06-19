#include "stats_widget.hpp"

#include <imgui.h>

#include "src/compute/fluid_sim.hpp"
#include "src/compute/gpu_profiler.hpp"
#include "src/render/render.hpp"
#include "src/utils/process_stats.hpp"

void StatsWidget::render(const FluidSim& sim, const Render& render, const GpuProfiler<>& uiProfiler) {
    ImGuiIO& io = ImGui::GetIO();

    ImGui::Text("FPS: %.1f", io.Framerate);
    ImGui::Text("Frame time: %.3f ms", 1000.0 / static_cast<double>(io.Framerate));

    if (!sim.profiler.enabled) {
        ImGui::TextDisabled("GPU timing not enabled");
    }
    else if (auto stats = sim.profiler.getStats()) {
        ImGui::Text("Physics (GPU): %.3f ms", stats->avgNs / 1e6);
    }
    else {
        ImGui::TextDisabled("GPU timing not supported");
    }

    if (!render.profiler.enabled) {
        ImGui::TextDisabled("GPU timing not enabled");
    }
    else if (auto stats = render.profiler.getStats()) {
        ImGui::Text("Render (GPU): %.3f ms", stats->avgNs / 1e6);
    }
    else {
        ImGui::TextDisabled("GPU timing not supported");
    }

    if (!uiProfiler.enabled) {
        ImGui::TextDisabled("GPU timing not enabled");
    }
    else if (auto stats = uiProfiler.getStats()) {
        ImGui::Text("Render UI (GPU): %.3f ms", stats->avgNs / 1e6);
    }
    else {
        ImGui::TextDisabled("GPU timing not supported");
    }

    ImGui::Separator();

    ImGui::Text("Sim size: %ux%u = %u", sim.state.width, sim.state.height,
                sim.state.width * sim.state.height);
    ImGui::Text("Dye size: %ux%u = %u", sim.state.dye_width, sim.state.dye_height,
                sim.state.dye_width * sim.state.dye_height);

    ImGui::Separator();

    ProcessStats::Stats processStats = ProcessStats::Monitor::sample();
    ImGui::Text("CPU: %.1f%%", processStats.cpuPercent);
    ImGui::Text("Memory: %.3f MB", static_cast<float>(processStats.rssBytes) / 1024.f / 1024.f);
}
