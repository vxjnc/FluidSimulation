#include "wgpu_context.hpp"

#include <print>
#include <stdexcept>

#include <GLFW/glfw3native.h>
#include <glfw3webgpu.h>

void WGPUContext::init(GLFWwindow* window, uint32_t width, uint32_t height) {
    if (initialized_) {
        return;
    }

    wgpu::InstanceDescriptor instanceDesc{};

#ifdef WEBGPU_BACKEND_DAWN
    static const char* dawnTogglesList[] = {
#ifndef NDEBUG
        "use_user_defined_labels_in_backend", "dump_shaders", "validation_manual_triggering",
        "allow_unsafe_apis"
#else
        "allow_unsafe_apis"
#endif

    };

    static wgpu::DawnTogglesDescriptor dawnToggles{};
    dawnToggles.chain.sType = wgpu::SType::DawnTogglesDescriptor;
    dawnToggles.enabledToggleCount = sizeof(dawnTogglesList) / sizeof(dawnTogglesList[0]);
    dawnToggles.enabledToggles = dawnTogglesList;

    instanceDesc.nextInChain = &dawnToggles.chain;
#endif

#ifdef WEBGPU_BACKEND_WGPU
#ifndef NDEBUG
    static WGPUInstanceExtras extras{};
    extras.chain.sType = static_cast<WGPUSType>(WGPUNativeSType::WGPUSType_InstanceExtras);
    extras.flags = WGPUInstanceFlag_Debug | WGPUInstanceFlag_Validation;
    instanceDesc.nextInChain = &extras.chain;
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

        std::println(std::cerr, "wgpu device lost({}): {}", static_cast<int>(reason),
                     std::string_view(msg.data, msg.length));
    };

    deviceDesc.uncapturedErrorCallbackInfo.callback = [](WGPUDevice const*, WGPUErrorType type,
                                                         WGPUStringView msg, void*, void*) {
        std::println(std::cerr, "wgpu error ({}): {}", static_cast<int>(type),
                     std::string_view(msg.data, msg.length));
    };

    std::vector<WGPUFeatureName> requiredFeatures;
    if (adapter_->hasFeature(wgpu::FeatureName::TimestampQuery)) {
        requiredFeatures.emplace_back(wgpu::FeatureName::TimestampQuery);
    }
#ifdef WEBGPU_BACKEND_DAWN
    if (adapter_->hasFeature(wgpu::FeatureName::ChromiumExperimentalTimestampQueryInsidePasses)) {
        requiredFeatures.emplace_back(wgpu::FeatureName::ChromiumExperimentalTimestampQueryInsidePasses);
    }
#endif
#ifdef WEBGPU_BACKEND_WGPU
    if (adapter_->hasFeature(static_cast<WGPUFeatureName>(wgpu::NativeFeature::TimestampQueryInsidePasses))) {
        requiredFeatures.emplace_back(
            static_cast<WGPUFeatureName>(wgpu::NativeFeature::TimestampQueryInsidePasses));
    }
#endif

    deviceDesc.requiredFeatureCount = requiredFeatures.size();
    deviceDesc.requiredFeatures = requiredFeatures.data();

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

void WGPUContext::resize(uint32_t width, uint32_t height) {
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
