#pragma once

#include <span>

#include <webgpu/webgpu-raii.hpp>

class FluidState {
public:
    struct Params {
        uint32_t width, height;
        uint32_t dye_width, dye_height;
        float dt;
        float vel_dissipation;
        float dye_dissipation;
        float curl_strength;
    };

    struct FillCircleParams {
        float cx, cy;
        float radius;
        uint32_t val;
        uint32_t width, height;
    };

    struct InjectParams {
        float color[4];
        float x, y;
        float vx, vy;
        float radius;
        uint32_t mode_mask;
        uint32_t form;
        uint32_t _pad;
    };

    void init(wgpu::Device device, wgpu::Queue queue, uint32_t w, uint32_t h, uint32_t dye_w, uint32_t dye_h);

    bool uploadInjects(std::span<const InjectParams> sources);

    void resize(uint32_t w, uint32_t h);
    void resizeDye(uint32_t w, uint32_t h);

    void clear();

    uint32_t width = 0, height = 0;
    uint32_t dye_width = 0, dye_height = 0;

    wgpu::raii::Buffer velocity, velocity_next;
    wgpu::raii::Buffer pressure, pressure_next;
    wgpu::raii::Buffer divergence;
    wgpu::raii::Buffer curl;
    wgpu::raii::Buffer dye, dye_next;
    wgpu::raii::Buffer obstacles;

    wgpu::raii::Buffer paramsBuffer;
    wgpu::raii::Buffer injectCountBuffer;
    wgpu::raii::Buffer injectsBuffer;
    wgpu::raii::Buffer fillCircleBuffer;

private:
    uint32_t injectsCapacity_ = 0;
    wgpu::Device device_;
    wgpu::Queue queue_;
};
