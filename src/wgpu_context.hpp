#pragma once

#include <GLFW/glfw3.h>
#include <webgpu/webgpu-raii.hpp>

class WGPUContext {
public:
    static WGPUContext& instance();

    ~WGPUContext();

    void init(GLFWwindow* window, uint32_t width, uint32_t height);

    void resize(uint32_t width, uint32_t height);

    void present();
    void processEvents();

    wgpu::Device device() const;
    wgpu::Queue queue() const;
    wgpu::Surface surface() const;
    wgpu::TextureFormat surfaceFormat() const;
    uint32_t width() const;
    uint32_t height() const;

private:
    bool initialized_ = false;

    wgpu::raii::Instance instance_;
    wgpu::raii::Adapter adapter_;
    wgpu::raii::Device device_;
    wgpu::raii::Queue queue_;
    wgpu::raii::Surface surface_;

    wgpu::TextureFormat surfaceFormat_ = wgpu::TextureFormat::Undefined;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
};
