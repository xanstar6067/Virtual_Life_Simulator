#pragma once

#include "imgui.h"
#include "implot.h"
#include <algorithm>
#include <array>
#include <cstring>

namespace vlsui
{
// World/object colors describe simulation data and stay in the engines.
// Themes own presentation colors, widget geometry, font choice and graph style.
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
    ImVec4 brainPulsePositive{1, .45f, .35f, 1}, brainPulseNegative{.4f, .65f, 1, 1};
    std::array<ImVec4, 6> neuronTypes{{{.58f, .67f, .68f, 1}, {.88f, .70f, .39f, 1},
        {.32f, .77f, .62f, 1}, {.43f, .65f, .93f, 1}, {.84f, .87f, .88f, 1}, {.70f, .51f, .83f, 1}}};
};

enum class ThemeId { Custom, Dark, Light, Retro };
inline constexpr ThemeId BuiltinThemes[] = {ThemeId::Dark, ThemeId::Light, ThemeId::Retro};
enum class ThemeFont { Interface, Technical };

struct ThemeStyle
{
    float windowRounding = 10, childRounding = 8, frameRounding = 7, popupRounding = 8;
    float scrollbarRounding = 9, grabRounding = 6, tabRounding = 7;
    float windowBorder = 1, childBorder = 1, popupBorder = 1, frameBorder = 0;
    ThemeFont font = ThemeFont::Interface;
    bool squareNeurons = false, straightConnections = false, retroBrainEffects = false;
    float neuronRadius = 8, connectionOpacity = .40f;
};

inline const char* ThemeName(ThemeId id)
{
    return id == ThemeId::Retro ? "Ретрофутуризм" : id == ThemeId::Light ? "Светлая" : id == ThemeId::Dark ? "Тёмная" : "Другая";
}

struct Theme
{
    ThemeId id = ThemeId::Custom;
    ThemePalette palette;
    ThemeStyle style;
    std::array<ImVec4, ImGuiCol_COUNT> imguiColors;
    std::array<ImVec4, ImPlotCol_COUNT> implotColors;
};

// Builds a complete value without requiring an ImGui/ImPlot context.
inline Theme MakeDefaultTheme()
{
    Theme theme;
    theme.id = ThemeId::Dark;
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

inline Theme MakeLightTheme()
{
    Theme theme = MakeDefaultTheme();
    theme.id = ThemeId::Light;
    auto& p = theme.palette;
    p.accent = ImVec4(.69f, .86f, .78f, 1);
    p.accentHovered = ImVec4(.61f, .81f, .72f, 1);
    p.accentActive = ImVec4(.55f, .76f, .66f, 1);
    p.success = ImVec4(.08f, .40f, .27f, 1);
    p.warning = ImVec4(.55f, .30f, .02f, 1);
    p.danger = ImVec4(.93f, .73f, .70f, 1);
    p.confirmation = ImVec4(.92f, .68f, .65f, 1);
    p.muted = ImVec4(.35f, .42f, .41f, 1);
    p.panel = ImVec4(.98f, .985f, .97f, 1);
    p.sectionTitle = ImVec4(.17f, .32f, .28f, 1);
    p.desktop = ImVec4(.87f, .91f, .89f, 1);
    p.logBackground = ImVec4(.92f, .95f, .98f, 1);
    p.diagnostic = ImVec4(.48f, .18f, .59f, 1);
    p.predator = ImVec4(.72f, .20f, .17f, 1);
    p.organicFeeder = ImVec4(.13f, .35f, .68f, 1);
    p.mixedFeeder = ImVec4(.48f, .25f, .67f, 1);
    p.lowEnergy = ImVec4(.55f, .38f, .02f, 1);
    p.apples = ImVec4(.58f, .41f, .03f, 1);
    p.organics = ImVec4(.45f, .32f, .62f, 1);
    p.chartEvent = ImVec4(.25f, .35f, .32f, .55f);
    p.brainCanvas = ImVec4(.94f, .96f, .94f, 1);
    p.positiveConnection = ImVec4(.72f, .24f, .16f, 1);
    p.negativeConnection = ImVec4(.13f, .36f, .72f, 1);
    p.neuronTypes = {{{.36f, .45f, .44f, 1}, {.63f, .43f, .12f, 1},
        {.10f, .48f, .33f, 1}, {.17f, .39f, .71f, 1}, {.23f, .30f, .29f, 1}, {.51f, .29f, .65f, 1}}};

    ImGuiStyle gui;
    ImGui::StyleColorsLight(&gui);
    std::copy_n(gui.Colors, ImGuiCol_COUNT, theme.imguiColors.begin());
    auto* c = theme.imguiColors.data();
    c[ImGuiCol_Text] = ImVec4(.12f, .19f, .17f, 1);
    c[ImGuiCol_TextDisabled] = p.muted;
    c[ImGuiCol_WindowBg] = c[ImGuiCol_PopupBg] = p.panel;
    c[ImGuiCol_ChildBg] = ImVec4(.94f, .96f, .94f, 1);
    c[ImGuiCol_Border] = ImVec4(.70f, .77f, .73f, 1);
    c[ImGuiCol_FrameBg] = ImVec4(.88f, .93f, .90f, 1);
    c[ImGuiCol_FrameBgHovered] = ImVec4(.80f, .88f, .83f, 1);
    c[ImGuiCol_FrameBgActive] = ImVec4(.73f, .84f, .77f, 1);
    c[ImGuiCol_TitleBg] = c[ImGuiCol_TitleBgCollapsed] = p.panel;
    c[ImGuiCol_TitleBgActive] = c[ImGuiCol_MenuBarBg] = ImVec4(.88f, .93f, .90f, 1);
    c[ImGuiCol_ScrollbarBg] = ImVec4(.91f, .94f, .92f, 1);
    c[ImGuiCol_ScrollbarGrab] = ImVec4(.58f, .68f, .62f, 1);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(.45f, .59f, .51f, 1);
    c[ImGuiCol_ScrollbarGrabActive] = p.success;
    c[ImGuiCol_CheckMark] = c[ImGuiCol_SliderGrab] = p.success;
    c[ImGuiCol_SliderGrabActive] = ImVec4(.05f, .31f, .20f, 1);
    c[ImGuiCol_Button] = c[ImGuiCol_Header] = ImVec4(.85f, .91f, .87f, 1);
    c[ImGuiCol_ButtonHovered] = c[ImGuiCol_HeaderHovered] = p.accentHovered;
    c[ImGuiCol_ButtonActive] = c[ImGuiCol_HeaderActive] = p.accentActive;
    c[ImGuiCol_Separator] = c[ImGuiCol_TableBorderStrong] = ImVec4(.70f, .77f, .73f, 1);
    c[ImGuiCol_SeparatorHovered] = c[ImGuiCol_SeparatorActive] = p.success;
    c[ImGuiCol_ResizeGrip] = ImVec4(.10f, .43f, .30f, .20f);
    c[ImGuiCol_ResizeGripHovered] = ImVec4(.10f, .43f, .30f, .50f);
    c[ImGuiCol_ResizeGripActive] = p.success;
    c[ImGuiCol_Tab] = c[ImGuiCol_TabUnfocused] = ImVec4(.88f, .93f, .90f, 1);
    c[ImGuiCol_TabHovered] = p.accentHovered;
    c[ImGuiCol_TabActive] = c[ImGuiCol_TabUnfocusedActive] = p.accent;
    c[ImGuiCol_TableHeaderBg] = ImVec4(.84f, .90f, .86f, 1);
    c[ImGuiCol_TableBorderLight] = ImVec4(.80f, .85f, .82f, 1);
    c[ImGuiCol_TableRowBgAlt] = ImVec4(.20f, .42f, .31f, .06f);
    c[ImGuiCol_TextSelectedBg] = ImVec4(.18f, .55f, .37f, .25f);
    c[ImGuiCol_NavHighlight] = p.success;
    c[ImGuiCol_PlotLines] = c[ImGuiCol_PlotHistogram] = p.success;
    return theme;
}

// Inspired by the pre-redesign neural display (before f7452c5): black canvas,
// square yellow I/O nodes, straight red/blue connections. Keep the current editor.
inline Theme MakeRetroTheme()
{
    Theme theme = MakeDefaultTheme();
    theme.id = ThemeId::Retro;
    auto& shape = theme.style;
    shape.windowRounding = shape.childRounding = shape.frameRounding = shape.popupRounding = 0;
    shape.scrollbarRounding = shape.grabRounding = shape.tabRounding = 0;
    shape.frameBorder = 1;
    shape.font = ThemeFont::Technical;
    shape.squareNeurons = shape.straightConnections = shape.retroBrainEffects = true;
    shape.neuronRadius = 12;
    shape.connectionOpacity = .90f;
    auto& p = theme.palette;
    p.accent = ImVec4(.12f, .29f, .30f, 1);
    p.accentHovered = ImVec4(.17f, .39f, .40f, 1);
    p.accentActive = ImVec4(.20f, .47f, .44f, 1);
    p.panel = ImVec4(.025f, .045f, .055f, 1);
    p.muted = ImVec4(.55f, .70f, .68f, 1);
    p.sectionTitle = ImVec4(.45f, .91f, .80f, 1);
    p.success = ImVec4(.36f, .90f, .63f, 1);
    p.warning = ImVec4(1, .71f, .26f, 1);
    p.desktop = ImVec4(.008f, .015f, .02f, 1);
    p.logBackground = ImVec4(.015f, .028f, .035f, 1);
    p.brainCanvas = ImVec4(0, 0, 0, 1);
    p.positiveConnection = ImVec4(.73f, .06f, .06f, 1);
    p.negativeConnection = ImVec4(.06f, .16f, .85f, 1);
    p.neuronTypes = {{{.59f, .59f, .59f, 1}, {.92f, .92f, 0, 1}, {.92f, .92f, 0, 1},
        {.08f, .22f, .90f, 1}, {1, 1, 1, 1}, {.70f, .08f, .70f, 1}}};
    auto* c = theme.imguiColors.data();
    c[ImGuiCol_Text] = ImVec4(.82f, .92f, .87f, 1);
    c[ImGuiCol_TextDisabled] = p.muted;
    c[ImGuiCol_WindowBg] = c[ImGuiCol_PopupBg] = p.panel;
    c[ImGuiCol_ChildBg] = p.logBackground;
    c[ImGuiCol_Border] = c[ImGuiCol_Separator] = ImVec4(.22f, .46f, .44f, 1);
    c[ImGuiCol_FrameBg] = ImVec4(.035f, .09f, .10f, 1);
    c[ImGuiCol_FrameBgHovered] = p.accent;
    c[ImGuiCol_FrameBgActive] = p.accentHovered;
    c[ImGuiCol_TitleBg] = c[ImGuiCol_TitleBgCollapsed] = p.panel;
    c[ImGuiCol_TitleBgActive] = c[ImGuiCol_MenuBarBg] = ImVec4(.06f, .15f, .16f, 1);
    c[ImGuiCol_ScrollbarBg] = p.panel;
    c[ImGuiCol_ScrollbarGrab] = p.accent;
    c[ImGuiCol_ScrollbarGrabHovered] = p.accentHovered;
    c[ImGuiCol_ScrollbarGrabActive] = p.sectionTitle;
    c[ImGuiCol_CheckMark] = c[ImGuiCol_SliderGrab] = p.sectionTitle;
    c[ImGuiCol_SliderGrabActive] = p.success;
    c[ImGuiCol_Button] = c[ImGuiCol_Header] = ImVec4(.05f, .13f, .14f, 1);
    c[ImGuiCol_ButtonHovered] = c[ImGuiCol_HeaderHovered] = p.accentHovered;
    c[ImGuiCol_ButtonActive] = c[ImGuiCol_HeaderActive] = p.accentActive;
    c[ImGuiCol_SeparatorHovered] = c[ImGuiCol_SeparatorActive] = p.sectionTitle;
    c[ImGuiCol_ResizeGrip] = c[ImGuiCol_ResizeGripHovered] = p.accentHovered;
    c[ImGuiCol_ResizeGripActive] = p.sectionTitle;
    c[ImGuiCol_Tab] = c[ImGuiCol_TabUnfocused] = c[ImGuiCol_FrameBg];
    c[ImGuiCol_TabHovered] = p.accentHovered;
    c[ImGuiCol_TabActive] = c[ImGuiCol_TabUnfocusedActive] = p.accent;
    c[ImGuiCol_TableHeaderBg] = c[ImGuiCol_FrameBg];
    c[ImGuiCol_TableBorderStrong] = c[ImGuiCol_Border];
    c[ImGuiCol_TableBorderLight] = ImVec4(.13f, .27f, .27f, 1);
    c[ImGuiCol_TableRowBgAlt] = ImVec4(.09f, .23f, .22f, .30f);
    c[ImGuiCol_TextSelectedBg] = ImVec4(.20f, .70f, .55f, .35f);
    c[ImGuiCol_NavHighlight] = p.sectionTitle;
    c[ImGuiCol_PlotLines] = c[ImGuiCol_PlotHistogram] = p.success;
    return theme;
}

inline Theme MakeBuiltinTheme(ThemeId id)
{
    if (id == ThemeId::Light) return MakeLightTheme();
    if (id == ThemeId::Retro) return MakeRetroTheme();
    return MakeDefaultTheme();
}

inline void ApplyThemeStyle(const ThemeStyle& theme)
{
    if (!ImGui::GetCurrentContext()) return;
    auto& style = ImGui::GetStyle();
    style.WindowRounding = theme.windowRounding;
    style.ChildRounding = theme.childRounding;
    style.FrameRounding = theme.frameRounding;
    style.PopupRounding = theme.popupRounding;
    style.ScrollbarRounding = theme.scrollbarRounding;
    style.GrabRounding = theme.grabRounding;
    style.TabRounding = theme.tabRounding;
    style.WindowBorderSize = theme.windowBorder;
    style.ChildBorderSize = theme.childBorder;
    style.PopupBorderSize = theme.popupBorder;
    style.FrameBorderSize = theme.frameBorder;
    auto& io = ImGui::GetIO();
    const char* name = theme.font == ThemeFont::Technical ? "Technical" : "Interface";
    io.FontDefault = nullptr;
    for (const auto& config : io.Fonts->ConfigData)
        if (!std::strcmp(config.Name, name)) io.FontDefault = config.DstFont;
    // Missing optional technical font falls back to the first interface font.
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
    ApplyThemeStyle(theme.style);
    if (ImGui::GetCurrentContext())
        std::copy(theme.imguiColors.begin(), theme.imguiColors.end(), ImGui::GetStyle().Colors);
    if (ImPlot::GetCurrentContext())
        std::copy(theme.implotColors.begin(), theme.implotColors.end(), ImPlot::GetStyle().Colors);
}
namespace detail
{
inline ThemeId pendingTheme = ThemeId::Custom;
}

// The menu queues its choice while ImGui has active windows/style scopes.
// Commit it before clearing the next SDL frame so the entire frame agrees.
inline void RequestTheme(ThemeId id)
{
    if (id == ThemeId::Dark || id == ThemeId::Light || id == ThemeId::Retro) detail::pendingTheme = id;
}
inline bool ApplyPendingTheme()
{
    const auto id = detail::pendingTheme;
    detail::pendingTheme = ThemeId::Custom;
    if (id == ThemeId::Custom) return false;
    ApplyTheme(MakeBuiltinTheme(id));
    return true;
}

} // namespace vlsui
