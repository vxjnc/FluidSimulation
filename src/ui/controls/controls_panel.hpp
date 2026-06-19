#pragma once
#include <vector>

#include <imgui.h>

#include "src/ui/controls/brush_widget.hpp"
#include "src/ui/controls/render_widget.hpp"
#include "src/ui/controls/simulation_widget.hpp"
#include "src/ui/controls/source_widget.hpp"

class ControlsPanel {
public:
    void render(bool& open, FluidViewport& viewport, FluidSim& sim, AppSettings& settings,
                std::vector<FluidSource>& sources);

private:
    SimulationWidget simulationWidget;
    RenderWidget renderWidget;
    BrushWidget brushWidget;
    SourceWidget sourceWidget;
};
