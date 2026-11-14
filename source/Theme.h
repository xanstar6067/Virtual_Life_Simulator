#pragma once

#include "imgui.h"
#include "implot.h"
#include <algorithm>
#include <array>

namespace vlsui
{
// Presentation colors only. World/object colors describe simulation data and
// stay in the engines. Layout, fonts and widget sizes are independent of a theme.
struct ThemePalette
{
    ImVec4 accent{.14f, .48f, .40f, 1}, accentHovered{.19f, .58f, .48f, 1}, accentActive{.10f, .38f, .32f, 1};
    ImVec4 success{.20f, .78f, .55f, 1}, warning{.96f, .62f, .24f, 1};
    ImVec4 danger{.54f, .22f, .19f, 1}, confirmation{.62f, .22f, .19f, 1};
    ImVec4 muted{.76f, .81f, .81f, 1}, panel{.10f, .14f, .15f, 1}, sectionTitle{.80f, .87f, .86f, 1};
    ImVec4 desktop{0, 0, 0, 1}, logBackground{.1f, .1f, .3f, 1};
    ImVec4 diagnostic{1, 0, 1, 1};
    ImVec4 predator{1, .48f, .43f, 1}, organicFeeder{.45f, .68f, 1, 1}, mixedFeeder{.85f, .71f, 1, 1};
    ImVec4 lowEnergy{1, .85f, .3f, 1}, apples{.94f, .77f, .34f, 1}, organics{.64f, .60f, .88f, 1};
    ImVec4 chartEvent{.70f, .77f, .77f, .55f};
    ImVec4 brainCanvas{.065f, .095f, .105f, 1}, positiveConnection{.90f, .48f, .38f, 1}, negativeConnection{.40f, .65f, .94f, 1};
    std::array<ImVec4, 6> neuronTypes{{{.58f, .67f, .68f, 1}, {.88f, .70f, .39f, 1},
        {.32f, .77f, .62f, 1}, {.43f, .65f, .93f, 1}, {.84f, .87f, .88f, 1}, {.70f, .51f, .83f, 1}}};
};

struct Theme
{
    ThemePalette palette;
    std::array<ImVec4, ImGuiCol_COUNT> imguiColors;
    std::array<ImVec4, ImPlotCol_COUNT> implotColors;
};

// Builds a complete value without requiring an ImGui/ImPlot context.
inline Theme MakeDefaultTheme()
{
    Theme theme;
    ImGuiStyle gui;
    ImGui::StyleColorsDark(&gui);
    std::copy_n(gui.Colors, ImGuiCol_COUNT, theme.imguiColors.begin());
    ImPlotStyle plot;
    ImPlot::StyleColorsAuto(&plot);
    std::copy_n(plot.Colors, ImPlotCol_COUNT, theme.implotColors.begin());
    ImVec4* colors = theme.imguiColors.data();
    colors[ImGuiCol_Text] = ImVec4(0.92f, 0.94f, 0.97f, 1.00f);
    colors[ImGuiCol_TextDisabled] = theme.palette.muted;
    colors[ImGuiCol_WindowBg] = theme.palette.panel;
    colors[ImGuiCol_ChildBg] = ImVec4(0.085f, 0.12f, 0.13f, 1.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.14f, 0.15f, 1.00f);
    colors[ImGuiCol_Border] = ImVec4(0.22f, 0.28f, 0.29f, 0.75f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.14f, 0.19f, 0.20f, 1.00f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.27f, 0.28f, 1.00f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.23f, 0.33f, 0.32f, 1.00f);
    colors[ImGuiCol_TitleBg] = theme.palette.panel;
    colors[ImGuiCol_TitleBgActive] = theme.palette.panel;
    colors[ImGuiCol_TitleBgCollapsed] = theme.palette.panel;
    colors[ImGuiCol_MenuBarBg] = theme.palette.panel;
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.07f, 0.10f, 0.11f, 0.80f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.29f, 0.37f, 0.38f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.38f, 0.47f, 0.47f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive] = theme.palette.accent;
    colors[ImGuiCol_CheckMark] = theme.palette.accent;
    colors[ImGuiCol_SliderGrab] = theme.palette.accent;
    colors[ImGuiCol_SliderGrabActive] = theme.palette.accentHovered;
    colors[ImGuiCol_Button] = ImVec4(0.16f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.22f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.26f, 0.36f, 0.35f, 1.00f);
    colors[ImGuiCol_Header] = ImVec4(0.16f, 0.22f, 0.23f, 1.00f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.22f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.36f, 0.35f, 1.00f);
    colors[ImGuiCol_Separator] = ImVec4(0.22f, 0.28f, 0.29f, 0.80f);
    colors[ImGuiCol_SeparatorHovered] = theme.palette.accent;
    colors[ImGuiCol_SeparatorActive] = theme.palette.accentHovered;
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.22f, 0.62f, 0.48f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.22f, 0.62f, 0.48f, 0.65f);
    colors[ImGuiCol_ResizeGripActive] = theme.palette.accent;
    colors[ImGuiCol_Tab] = ImVec4(0.12f, 0.17f, 0.18f, 1.00f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.22f, 0.30f, 0.30f, 1.00f);
    colors[ImGuiCol_TabActive] = ImVec4(0.16f, 0.30f, 0.27f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.10f, 0.14f, 0.15f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.15f, 0.23f, 0.23f, 1.00f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.14f, 0.20f, 0.21f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.22f, 0.28f, 0.29f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.19f, 0.25f, 0.26f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.14f, 0.20f, 0.21f, 0.45f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.22f, 0.62f, 0.48f, 0.35f);
    colors[ImGuiCol_NavHighlight] = theme.palette.accent;
    colors[ImGuiCol_PlotHistogram] = theme.palette.success;
    return theme;
}

namespace detail
{
inline Theme& ThemeStorage()
{
    static Theme theme = MakeDefaultTheme();
    return theme;
}
}

inline const Theme& GetTheme() { return detail::ThemeStorage(); }
inline const ThemePalette& Palette() { return GetTheme().palette; }

// Call on the UI thread between frames, outside any PushStyleColor scope.
// The application owns a copy: callers may release their theme after applying it.
// Selection survives engine switches and context recreation; it is not world state.
inline void ApplyTheme(const Theme& theme)
{
    detail::ThemeStorage() = theme;
    if (ImGui::GetCurrentContext())
        std::copy(theme.imguiColors.begin(), theme.imguiColors.end(), ImGui::GetStyle().Colors);
    if (ImPlot::GetCurrentContext())
        std::copy(theme.implotColors.begin(), theme.implotColors.end(), ImPlot::GetStyle().Colors);
}
} // namespace vlsui
