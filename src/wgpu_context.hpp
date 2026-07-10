#pragma once

#include <GLFW/glfw3.h>
#include <webgpu/webgpu-raii.hpp>

class WGPUContext {
public:
    static WGPUContext& instance() {
        static WGPUContext ctx;
        return ctx;
    }

    ~WGPUContext() {
        if (initialized_) {
            surface_->unconfigure();
        }
    }

    void init(GLFWwindow* window, uint32_t width, uint32_t height);

    void resize(uint32_t width, uint32_t height);

    void present() { surface_->present(); }
    void processEvents() {
#ifdef WEBGPU_BACKEND_DAWN
        device_->tick();
#endif
#ifdef WEBGPU_BACKEND_WGPU
        device_->poll(false, nullptr);
#endif
    }

    wgpu::Device device() const { return *device_; }
    wgpu::Queue queue() const { return *queue_; }
    wgpu::Surface surface() const { return *surface_; }
    wgpu::TextureFormat surfaceFormat() const { return surfaceFormat_; }
    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }

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
