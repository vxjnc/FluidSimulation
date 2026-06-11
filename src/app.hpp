#pragma once
#include <cstdint>
#include <vector>

#include <GLFW/glfw3.h>
#include <webgpu/webgpu-raii.hpp>
#include <webgpu/webgpu.hpp>

#include "src/app_settings.hpp"
#include "src/capture/screenshot_capture.hpp"
#include "src/compute/fluid_sim.hpp"
#include "src/compute/fluid_source.hpp"
#include "src/render/render.hpp"
#include "src/save/save_manager.hpp"
#include "src/ui/fluid_viewport.hpp"
#include "src/ui/imgui_manager.hpp"
#include "src/utils/color_generator.hpp"
#include "src/utils/deffered_queue.hpp"

class Application {
public:
    Application(uint32_t width, uint32_t height, std::string_view title) {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to init GLFW");
        }
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), title.data(), nullptr,
                                  nullptr);
        if (!window) {
            glfwTerminate();
            throw std::runtime_error("Failed to create window");
        }

        WGPUContext& ctx = WGPUContext::instance();
        ctx.init(window, width, height);

        imguiManager.init(window, ctx.device(), ctx.surfaceFormat(), &settings);

        glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int w, int h) {
            WGPUContext::instance().resize(static_cast<uint32_t>(w), static_cast<uint32_t>(h));
        });

        renderer.init();
        viewport.init(ctx.device(), width, height, ctx.surfaceFormat());

        simulation.init(ctx.device(), ctx.queue(),
                        static_cast<uint32_t>(static_cast<float>(width) * settings.simScale),
                        static_cast<uint32_t>(static_cast<float>(height) * settings.simScale),
                        static_cast<uint32_t>(static_cast<float>(width) * settings.dyeScale),
                        static_cast<uint32_t>(static_cast<float>(height) * settings.dyeScale));

        // --- signals ---
        settings.dt.onChange.connect(&FluidSim::setDt, &simulation);
        settings.dt.onChange(static_cast<float>(settings.dt));

        settings.dyeDissipation.onChange.connect(&FluidSim::setDyeDissipation, &simulation);
        settings.dyeDissipation.onChange(static_cast<float>(settings.dyeDissipation));

        settings.velDissipation.onChange.connect(&FluidSim::setVelDissipation, &simulation);
        settings.velDissipation.onChange(static_cast<float>(settings.velDissipation));

        settings.curlStrength.onChange.connect(&FluidSim::setCurlStrength, &simulation);
        settings.curlStrength.onChange(static_cast<float>(settings.curlStrength));
    };

    ~Application() {
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    void run() {
        WGPUContext& ctx = WGPUContext::instance();
        std::vector<FluidSource> frameSources;

        imguiManager.onSplats.connect([&](auto splats) {
            preSimQueue_.push([&, splats = std::move(splats)]() {
                frameSources.insert(frameSources.end(), splats.begin(), splats.end());
            });
        });
        imguiManager.onImport.connect([&](auto target, auto pixels, auto w, auto h) {
            postSubmitQueue_.push([&, target, pixels = std::move(pixels), w, h]() {
                switch (target) {
                case ImportTarget::Dye:
                    ctx.queue().writeBuffer(*simulation.getCurrentDye(), 0, pixels.data(),
                                            pixels.size() * sizeof(float));
                    break;
                case ImportTarget::Velocity: {
                    // RGBA -> RG
                    std::vector<float> rg;
                    rg.reserve(w * h * 2);
                    for (size_t i = 0; i < pixels.size(); i += 4) {
                        rg.emplace_back(pixels[i]);
                        rg.emplace_back(pixels[i + 1]);
                    }
                    ctx.queue().writeBuffer(*simulation.state.velocity, 0, rg.data(),
                                            rg.size() * sizeof(float));
                    break;
                }
                case ImportTarget::Obstacles: {
                    // RGBA -> uint32 (R > 0.5 => 1)
                    auto obs = std::ranges::to<std::vector<uint32_t>>(
                        pixels | std::views::stride(4) |
                        std::views::transform([](float v) { return v > 0.5f ? 1u : 0u; }));
                    ctx.queue().writeBuffer(*simulation.state.obstacles, 0, obs.data(),
                                            obs.size() * sizeof(uint32_t));
                    break;
                }
                }
            });
        });

        imguiManager.onSaveRequested.connect(
            [&](auto path) { SaveManager::save(path, simulation, sources, settings); });
        imguiManager.onLoadRequested.connect(
            [&](auto path) { SaveManager::load(path, simulation, sources, settings); });

        imguiManager.onScreenshotClipboard.connect(
            [&]() { ScreenshotCapture::request(viewport, ScreenshotCapture::Mode::Clipboard); });
        imguiManager.onScreenshotFile.connect(
            [&](auto path) { ScreenshotCapture::request(viewport, ScreenshotCapture::Mode::File, path); });

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            ctx.processEvents();
            imguiManager.beginFrame();

            wgpu::raii::CommandEncoder enc = ctx.device().createCommandEncoder();

            if (!settings.paused) {
                frameSources = sources;
            }
            else {
                frameSources.clear();
            }
            preSimQueue_.flush();

            processInput(enc, frameSources);
            update(enc, frameSources);
            imguiManager.renderUI(viewport, mouse, simulation, sources);
            auto [target, targetView] = render(enc);

            wgpu::raii::CommandBuffer cmd = enc->finish({});
            ctx.queue().submit(1, &*cmd);

            postSubmitQueue_.flush();

            ctx.present();
        }
    }

private:
    DeferredQueue preSimQueue_;
    DeferredQueue postSubmitQueue_;

    void processInput(wgpu::raii::CommandEncoder& enc, std::vector<FluidSource>& frameSources) {
        float vw = static_cast<float>(viewport.w);
        float vh = static_cast<float>(viewport.h);

        float sx = mouse.x / vw;
        float sy = 1.f - (mouse.y / vh);
        float sdx = (mouse.dx / vh) / settings.dt;
        float sdy = (mouse.dy / vh) / settings.dt;
        float sr = settings.brushRadius;

        if (settings.brushMode == BrushMode::Inject) {
            if (mouse.leftPressed) {
                frameSources
                    .emplace_back(sx, sy, sdx * settings.brushStrength, -sdy * settings.brushStrength, sr,
                                  settings.brushColor)
                    .mode_mask = settings.brushModeMask;
            }
        }
        else if (settings.brushMode == BrushMode::PaintWall) {
            if (mouse.leftPressed || mouse.rightPressed) {
                simulation.paintObstacle(enc, sx, sy, sr, mouse.rightPressed);
            }
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Space)) {
            settings.paused = !settings.paused;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_D)) {
            settings.renderSettings.mode = RenderMode::Dye;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_V)) {
            settings.renderSettings.mode = RenderMode::Velocity;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_P)) {
            settings.renderSettings.mode = RenderMode::Pressure;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_G)) {
            settings.renderSettings.mode = RenderMode::Divergence;
        }
        if (ImGui::IsKeyPressed(ImGuiKey_C)) {
            settings.renderSettings.mode = RenderMode::Curl;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_LeftAlt) || ImGui::IsKeyPressed(ImGuiKey_RightAlt)) {
            imguiManager.menuBarVisible = !imguiManager.menuBarVisible;
        }

        if (ImGui::IsKeyPressed(ImGuiKey_F12)) {
            ScreenshotCapture::request(viewport, ScreenshotCapture::Mode::Clipboard);
        }

        if (settings.paused && ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
            settings.paused = false;
            update(enc, frameSources);
            settings.paused = true;
        }

        if (mouse.leftJustPressed) {
            static float hue = static_cast<float>(time(nullptr));
            settings.brushColor = ColorUtils::Generator::nextGoldenRatioColor(hue);
        }
    }

    void update(wgpu::raii::CommandEncoder& enc, std::span<const FluidSource> frameSources) {
        if (!frameSources.empty()) {
            simulation.inject(enc, frameSources);
        }
        if (!settings.paused) {
            simulation.step(enc);
        }
    }

    std::pair<wgpu::raii::Texture, wgpu::raii::TextureView> render(wgpu::raii::CommandEncoder& enc) {
        WGPUContext& ctx = WGPUContext::instance();

        renderer.draw(enc, viewport.view, simulation, settings.renderSettings);

        wgpu::SurfaceTexture surfaceTex{};
        ctx.surface().getCurrentTexture(&surfaceTex);
        wgpu::raii::Texture target(surfaceTex.texture);
        wgpu::raii::TextureView targetView = target->createView();

        wgpu::RenderPassColorAttachment att{};
        att.view = *targetView;
        att.loadOp = wgpu::LoadOp::Clear;
        att.storeOp = WGPUStoreOp_Store;
        att.clearValue = {0.0, 0.0, 0.0, 1.0};
        att.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;

        wgpu::RenderPassDescriptor passDesc{};
        passDesc.colorAttachmentCount = 1;
        passDesc.colorAttachments = &att;

        wgpu::raii::RenderPassEncoder pass = enc->beginRenderPass(passDesc);
        imguiManager.endFrame(*pass);
        pass->end();

        return {std::move(target), std::move(targetView)};
    }

    GLFWwindow* window = nullptr;
    NFD::Guard nfdGuard;

    AppSettings settings;

    FluidSim simulation;
    std::vector<FluidSource> sources;

    Render renderer;
    FluidViewport viewport;
    ImGuiManager imguiManager;
    MouseState mouse;
};
