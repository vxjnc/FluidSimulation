
#pragma once
#include <cmath>
#include <cstdint>

#include <imgui.h>

namespace AppStyle {
    inline void apply() {
        auto& style{ImGui::GetStyle()};
        // Borders
        style.WindowBorderSize = 3.0f;

        // Rounding
        style.FrameRounding = 3.0f;
        style.PopupRounding = 3.0f;
        style.ScrollbarRounding = 3.0f;
        style.GrabRounding = 3.0f;

        // Docking
        style.DockingSeparatorSize = 3.0f;

        {
            constexpr auto ToRGBA = [](uint32_t argb) constexpr {
                ImVec4 color{};
                color.x = ((argb >> 16) & 0xFF) / 255.0f;
                color.y = ((argb >> 8) & 0xFF) / 255.0f;
                color.z = (argb & 0xFF) / 255.0f;
                color.w = ((argb >> 24) & 0xFF) / 255.0f;
                return color;
            };

            constexpr auto Lerp = [](const ImVec4& a, const ImVec4& b, float t) constexpr {
                return ImVec4{std::lerp(a.x, b.y, t), std::lerp(a.y, b.y, t), std::lerp(a.z, b.z, t),
                              std::lerp(a.w, b.w, t)};
            };

            auto colors{style.Colors};
            colors[ImGuiCol_Text] = ToRGBA(0xFFABB2BF);
            colors[ImGuiCol_TextDisabled] = ToRGBA(0xFF565656);
            colors[ImGuiCol_WindowBg] = ToRGBA(0xFF282C34);
            colors[ImGuiCol_ChildBg] = ToRGBA(0xFF21252B);
            colors[ImGuiCol_PopupBg] = ToRGBA(0xFF2E323A);
            colors[ImGuiCol_Border] = ToRGBA(0xFF2E323A);
            colors[ImGuiCol_BorderShadow] = ToRGBA(0x00000000);
            colors[ImGuiCol_FrameBg] = colors[ImGuiCol_ChildBg];
            colors[ImGuiCol_FrameBgHovered] = ToRGBA(0xFF484C52);
            colors[ImGuiCol_FrameBgActive] = ToRGBA(0xFF54575D);
            colors[ImGuiCol_TitleBg] = colors[ImGuiCol_WindowBg];
            colors[ImGuiCol_TitleBgActive] = colors[ImGuiCol_FrameBgActive];
            colors[ImGuiCol_TitleBgCollapsed] = ToRGBA(0x8221252B);
            colors[ImGuiCol_MenuBarBg] = colors[ImGuiCol_ChildBg];
            colors[ImGuiCol_ScrollbarBg] = colors[ImGuiCol_PopupBg];
            colors[ImGuiCol_ScrollbarGrab] = ToRGBA(0xFF3E4249);
            colors[ImGuiCol_ScrollbarGrabHovered] = ToRGBA(0xFF484C52);
            colors[ImGuiCol_ScrollbarGrabActive] = ToRGBA(0xFF54575D);
            colors[ImGuiCol_CheckMark] = colors[ImGuiCol_Text];
            colors[ImGuiCol_SliderGrab] = ToRGBA(0xFF353941);
            colors[ImGuiCol_SliderGrabActive] = ToRGBA(0xFF7A7A7A);
            colors[ImGuiCol_Button] = colors[ImGuiCol_SliderGrab];
            colors[ImGuiCol_ButtonHovered] = colors[ImGuiCol_FrameBgActive];
            colors[ImGuiCol_ButtonActive] = colors[ImGuiCol_ScrollbarGrabActive];
            colors[ImGuiCol_Header] = colors[ImGuiCol_ChildBg];
            colors[ImGuiCol_HeaderHovered] = ToRGBA(0xFF353941);
            colors[ImGuiCol_HeaderActive] = colors[ImGuiCol_FrameBgActive];
            colors[ImGuiCol_Separator] = colors[ImGuiCol_FrameBgActive];
            colors[ImGuiCol_SeparatorHovered] = ToRGBA(0xFF3E4452);
            colors[ImGuiCol_SeparatorActive] = colors[ImGuiCol_SeparatorHovered];
            colors[ImGuiCol_ResizeGrip] = colors[ImGuiCol_Separator];
            colors[ImGuiCol_ResizeGripHovered] = colors[ImGuiCol_SeparatorHovered];
            colors[ImGuiCol_ResizeGripActive] = colors[ImGuiCol_SeparatorActive];
            colors[ImGuiCol_InputTextCursor] = ToRGBA(0xFF528BFF);
            colors[ImGuiCol_TabHovered] = colors[ImGuiCol_HeaderHovered];
            colors[ImGuiCol_Tab] = colors[ImGuiCol_FrameBgActive];
            colors[ImGuiCol_TabSelected] = colors[ImGuiCol_HeaderHovered];
            colors[ImGuiCol_TabSelectedOverline] = colors[ImGuiCol_HeaderActive];
            colors[ImGuiCol_TabDimmed] = Lerp(colors[ImGuiCol_Tab], colors[ImGuiCol_TitleBg], 0.80f);
            colors[ImGuiCol_TabDimmedSelected] =
                Lerp(colors[ImGuiCol_TabSelected], colors[ImGuiCol_TitleBg], 0.40f);
            colors[ImGuiCol_TabDimmedSelectedOverline] = ImVec4{0.50f, 0.50f, 0.50f, 0.00f};
            colors[ImGuiCol_DockingPreview] = colors[ImGuiCol_ChildBg];
            colors[ImGuiCol_DockingEmptyBg] = colors[ImGuiCol_WindowBg];
            colors[ImGuiCol_PlotLines] = ImVec4{0.61f, 0.61f, 0.61f, 1.00f};
            colors[ImGuiCol_PlotLinesHovered] = ImVec4{1.00f, 0.43f, 0.35f, 1.00f};
            colors[ImGuiCol_PlotHistogram] = ImVec4{0.90f, 0.70f, 0.00f, 1.00f};
            colors[ImGuiCol_PlotHistogramHovered] = ImVec4{1.00f, 0.60f, 0.00f, 1.00f};
            colors[ImGuiCol_TableHeaderBg] = colors[ImGuiCol_ChildBg];
            colors[ImGuiCol_TableBorderStrong] = colors[ImGuiCol_SliderGrab];
            colors[ImGuiCol_TableBorderLight] = colors[ImGuiCol_FrameBgActive];
            colors[ImGuiCol_TableRowBg] = ImVec4{0.00f, 0.00f, 0.00f, 0.00f};
            colors[ImGuiCol_TableRowBgAlt] = ImVec4{1.00f, 1.00f, 1.00f, 0.06f};
            colors[ImGuiCol_TextLink] = ToRGBA(0xFF3F94CE);
            colors[ImGuiCol_TextSelectedBg] = ToRGBA(0xFF243140);
            colors[ImGuiCol_TreeLines] = colors[ImGuiCol_Text];
            colors[ImGuiCol_DragDropTarget] = colors[ImGuiCol_Text];
            colors[ImGuiCol_NavCursor] = colors[ImGuiCol_TextLink];
            colors[ImGuiCol_NavWindowingHighlight] = colors[ImGuiCol_Text];
            colors[ImGuiCol_NavWindowingDimBg] = ImVec4{0.80f, 0.80f, 0.80f, 0.20f};
            colors[ImGuiCol_ModalWindowDimBg] = ToRGBA(0xC821252B);
        };
    }
}
