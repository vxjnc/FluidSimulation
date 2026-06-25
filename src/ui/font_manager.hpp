#pragma once

#include <imgui.h>

class FontManager {
public:
    static void init(ImGuiIO& io);
    static ImFont* ui() { return ui_; };
    static ImFont* mono() { return mono_; };

private:
    static inline ImFont* ui_ = nullptr;
    static inline ImFont* mono_ = nullptr;
};
