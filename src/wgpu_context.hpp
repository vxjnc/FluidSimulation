#pragma once

#include <iostream>
#include <stdexcept>

#include <webgpu/webgpu-raii.hpp>

#include "webgpu/webgpu.hpp"

#if defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#elif defined(_WIN32)
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#endif
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

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
        WGPUInstanceExtras extras{};
        extras.chain.sType = static_cast<WGPUSType>(WGPUNativeSType::WGPUSType_InstanceExtras);
        extras.flags = WGPUInstanceFlag_Debug | WGPUInstanceFlag_Validation;
        instanceDesc.nextInChain = &extras.chain;
#endif

        instance_ = wgpu::createInstance(instanceDesc);
        if (!instance_) {
            throw std::runtime_error("wgpu: failed to create instance");
        }

        surface_ = createSurface(window);
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
        deviceDesc.deviceLostCallbackInfo.callback = [](WGPUDevice const*, WGPUDeviceLostReason reason, WGPUStringView msg, void*, void*) {
            if (reason == wgpu::DeviceLostReason::Destroyed) {
                return;
            }

            std::cerr << "wgpu device lost (" << reason << "): " << std::string_view(msg.data, msg.length) << "\n";
        };

        deviceDesc.uncapturedErrorCallbackInfo.callback = [](WGPUDevice const*, WGPUErrorType type, WGPUStringView msg, void*, void*) {
            std::cerr << "wgpu error (" << type << "): " << std::string_view(msg.data, msg.length) << "\n";
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
    void processEvents() { device_->tick(); }

    wgpu::Device device() const { return *device_; }
    wgpu::Queue queue() const { return *queue_; }
    wgpu::Surface surface() const { return *surface_; }
    wgpu::TextureFormat surfaceFormat() const { return surfaceFormat_; }
    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }

    wgpu::Buffer createBuffer(size_t bytes, wgpu::BufferUsage usage, std::string_view label, bool mappedAtCreation = false) {
        wgpu::BufferDescriptor desc{};
        desc.label = wgpu::StringView(label);
        desc.size = bytes;
        desc.usage = usage;
        desc.mappedAtCreation = mappedAtCreation;
        return device_->createBuffer(desc);
    }

    wgpu::BindGroup makeBindGroup(wgpu::raii::ComputePipeline& pipeline, std::initializer_list<std::pair<wgpu::Buffer, uint64_t>> buffers,
                                  std::string_view label) {
        wgpu::raii::BindGroupLayout layout = pipeline->getBindGroupLayout(0);

        std::vector<wgpu::BindGroupEntry> entries;
        uint32_t binding = 0;
        for (auto& [buf, size] : buffers) {
            wgpu::BindGroupEntry e{};
            e.binding = binding++;
            e.buffer = buf;
            e.size = size;
            entries.emplace_back(e);
        }

        wgpu::BindGroupDescriptor desc{};
        desc.layout = *layout;
        desc.entryCount = entries.size();
        desc.entries = entries.data();
        desc.label = wgpu::StringView(label);
        return device_->createBindGroup(desc);
    }

private:
    WGPUContext() = default;

    wgpu::Surface createSurface(GLFWwindow* window) {
#if defined(__linux__)
        wgpu::SurfaceSourceXlibWindow xlibSrc = wgpu::Default;
        xlibSrc.display = glfwGetX11Display();
        xlibSrc.window = glfwGetX11Window(window);

        wgpu::SurfaceDescriptor desc{};
        desc.label = wgpu::StringView("Surface");
        desc.nextInChain = &xlibSrc.chain;
        return instance_->createSurface(desc);

#elif defined(_WIN32)
        wgpu::SurfaceSourceWindowsHWND hwndSrc{};
        hwndSrc.hinstance = GetModuleHandle(nullptr);
        hwndSrc.hwnd = glfwGetWin32Window(window);

        wgpu::SurfaceDescriptor desc{};
        desc.label = wgpu::StringView("Surface");
        desc.nextInChain = &hwndSrc.chain;
        return instance_.createSurface(desc);

#elif defined(__APPLE__)
        wgpu::SurfaceSourceMetalLayer metalSrc{};
        metalSrc.layer = glfwGetCocoaWindow(window);

        wgpu::SurfaceDescriptor desc{};
        desc.label = wgpu::StringView("Surface");
        desc.nextInChain = &metalSrc.chain;
        return instance_.createSurface(desc);
#endif
    }

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
