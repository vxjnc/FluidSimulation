#include <iostream>

#include <GLFW/glfw3.h>

#include "src/wgpu_context.hpp"

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to init GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    constexpr uint32_t W = 800, H = 600;
    GLFWwindow* window = glfwCreateWindow(W, H, "Window", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }

    auto& ctx = WGPUContext::instance();
    ctx.init(window, W, H);

    glfwSetFramebufferSizeCallback(window, [](GLFWwindow*, int w, int h) { WGPUContext::instance().resize(w, h); });

    while (!glfwWindowShouldClose(window)) {
        ctx.processEvents();
        ctx.present();
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
