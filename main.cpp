#include <iostream>

#include <GLFW/glfw3.h>
#include <webgpu/webgpu-raii.hpp>

#include "src/compute/fluid_sim.hpp"
#include "src/render/render.hpp"
#include "src/wgpu_context.hpp"

struct MouseState {
    double x = 0, y = 0;
    double dx = 0, dy = 0;
    bool pressed = false;
};

struct AppState {
    FluidSim& sim;
    MouseState mouse;
    int width, height;
};

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW\n";
        return -1;
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    constexpr int W = 800, H = 600;
    GLFWwindow* window = glfwCreateWindow(W, H, "Fluid", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }

    WGPUContext& ctx = WGPUContext::instance();
    ctx.init(window, W, H);

    FluidSim sim;
    sim.init(W, H);

    AppState state{sim, {}, W, H};
    glfwSetWindowUserPointer(window, &state);

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow* w, int width, int height) {
        AppState* s = static_cast<AppState*>(glfwGetWindowUserPointer(w));
        s->width = width;
        s->height = height;
        WGPUContext::instance().resize(width, height);
        s->sim.resize(width, height);
    });

    glfwSetMouseButtonCallback(window, [](GLFWwindow* w, int btn, int action, int) {
        AppState* s = static_cast<AppState*>(glfwGetWindowUserPointer(w));
        if (btn == GLFW_MOUSE_BUTTON_LEFT) {
            s->mouse.pressed = action == GLFW_PRESS;
        }
    });

    glfwSetCursorPosCallback(window, [](GLFWwindow* w, double x, double y) {
        AppState* s = static_cast<AppState*>(glfwGetWindowUserPointer(w));
        s->mouse.dx = x - s->mouse.x;
        s->mouse.dy = y - s->mouse.y;
        s->mouse.x = x;
        s->mouse.y = y;
    });

    Render render;

    double prev = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ctx.processEvents();

        MouseState& mouse = state.mouse;
        if (mouse.pressed) {
            // sim.inject(static_cast<float>(mouse.x), static_cast<float>(state.height - mouse.y), static_cast<float>(mouse.dx) * 10.0f,
            //            -static_cast<float>(mouse.dy) * 10.0f);
        }
        mouse.dx = 0;
        mouse.dy = 0;

        double now = glfwGetTime();
        float dt = static_cast<float>(now - prev);
        prev = now;
        sim.step(dt);

        wgpu::SurfaceTexture surfaceTex{};
        ctx.surface().getCurrentTexture(&surfaceTex);

        wgpu::raii::Texture target(surfaceTex.texture);
        wgpu::TextureViewDescriptor viewDesc{};
        viewDesc.mipLevelCount = 1;
        viewDesc.arrayLayerCount = 1;
        wgpu::raii::TextureView targetView = target->createView(viewDesc);

        render.draw(targetView, sim);
        ctx.present();

        // static int i = 0;
        // if (i++ == 1000) {
        //     exit(0);
        // }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
