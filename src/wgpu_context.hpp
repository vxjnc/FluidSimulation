#pragma once

#include <iostream>
#include <print>
#include <stdexcept>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <glfw3webgpu.h>
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

    void init(GLFWwindow* window, uint32_t width, uint32_t height) {
        if (initialized_) {
            return;
        }

        wgpu::InstanceDescriptor instanceDesc{};

#ifndef NDEBUG

#ifdef WEBGPU_BACKEND_WGPU
        WGPUInstanceExtras extras{};
        extras.chain.sType = static_cast<WGPUSType>(WGPUNativeSType::WGPUSType_InstanceExtras);
        extras.flags = WGPUInstanceFlag_Debug | WGPUInstanceFlag_Validation;
        instanceDesc.nextInChain = &extras.chain;
#endif

#ifdef WEBGPU_BACKEND_DAWN

        static const char* enabledToggles[] = {
            "use_user_defined_labels_in_backend",
            "dump_shaders",
            "validation_manual_triggering",
        };
        static wgpu::DawnTogglesDescriptor dawnToggles{};
        dawnToggles.chain.sType = wgpu::SType::DawnTogglesDescriptor;
        dawnToggles.enabledToggleCount = sizeof(enabledToggles) / sizeof(enabledToggles[0]);
        dawnToggles.enabledToggles = enabledToggles;
        instanceDesc.nextInChain = &dawnToggles.chain;
#endif

#endif

        instance_ = wgpu::createInstance(instanceDesc);
        if (!instance_) {
            throw std::runtime_error("wgpu: failed to create instance");
        }

        surface_ = wgpu::raii::Surface(glfwCreateWindowWGPUSurface(*instance_, window));
        if (!surface_) {
            throw std::runtime_error("wgpu: failed to create surface");
        }

        wgpu::RequestAdapterOptions adapterOpts{};
        adapterOpts.compatibleSurface = *surface_;
        adapterOpts.powerPreference = wgpu::PowerPreference::HighPerformance;

        adapter_ = instance_->requestAdapter(adapterOpts);
        if (!adapter_) {
            throw std::runtime_error("wgpu: failed to get adapter");
        }

        wgpu::DeviceDescriptor deviceDesc{};
        deviceDesc.deviceLostCallbackInfo.mode = wgpu::CallbackMode::AllowSpontaneous;
        deviceDesc.deviceLostCallbackInfo.callback = [](WGPUDevice const*, WGPUDeviceLostReason reason,
                                                        WGPUStringView msg, void*, void*) {
            if (reason == wgpu::DeviceLostReason::Destroyed) {
                return;
            }

            std::print(std::cerr, "wgpu device lost({}): {}\n", static_cast<int>(reason),
                       std::string_view(msg.data, msg.length));
        };

        deviceDesc.uncapturedErrorCallbackInfo.callback = [](WGPUDevice const*, WGPUErrorType type,
                                                             WGPUStringView msg, void*, void*) {
            std::print(std::cerr, "wgpu error ({}): {}\n", static_cast<int>(type),
                       std::string_view(msg.data, msg.length));
        };

        device_ = adapter_->requestDevice(deviceDesc);
        if (!device_) {
            throw std::runtime_error("wgpu: failed to get device");
        }

        queue_ = device_->getQueue();

        wgpu::SurfaceCapabilities caps{};
        surface_->getCapabilities(*adapter_, &caps);
        surfaceFormat_ = caps.formats[0];
        initialized_ = true;
        resize(width, height);
    }

    void resize(uint32_t width, uint32_t height) {
        if (!initialized_ || (width == width_ && height == height_)) {
            return;
        }

        wgpu::SurfaceConfiguration surfaceConfig{};
        surfaceConfig.device = *device_;
        surfaceConfig.format = surfaceFormat_;
        surfaceConfig.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopyDst;
        surfaceConfig.width = width;
        surfaceConfig.height = height;
        surfaceConfig.presentMode = wgpu::PresentMode::Fifo;
        surface_->configure(surfaceConfig);

        width_ = width;
        height_ = height;
    }

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
    WGPUContext() = default;

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
