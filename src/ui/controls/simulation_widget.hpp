#pragma once

class AppSettings;
class FluidSim;
class FluidViewport;

class SimulationWidget {
public:
    void render(AppSettings& settings, FluidSim& sim, const FluidViewport& viewport);
};
