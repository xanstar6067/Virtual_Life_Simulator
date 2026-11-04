#pragma once

// Shared screen geometry for the SDL world, hit testing, and both ImGui shells.
// Simulation dimensions and save coordinates are independent of this layout.
namespace vlsui
{
struct Rect
{
    int x, y, w, h;
};

struct WorkspaceLayout
{
    Rect header, tools, world, inspector, footer;
    bool compact;
};

inline WorkspaceLayout CalculateLayout(int width, int height)
{
    const int gap = 10;
    const bool compact = width < 1100;
    const int headerHeight = 64;
    const int toolsWidth = 110;
    const int inspectorWidth = compact ? 280 : 320;
    const int footerHeight = compact ? 136 : 110;
    const int top = gap + headerHeight + gap;
    const int contentHeight = height - top - footerHeight - gap * 2;
    const int worldX = gap;
    const int toolsX = width - toolsWidth - gap;
    const int inspectorX = toolsX - gap - inspectorWidth;
    return {
        {gap, gap, width - gap * 2, headerHeight},
        {toolsX, top, toolsWidth, contentHeight},
        {worldX, top, inspectorX - gap - worldX, contentHeight},
        {inspectorX, top, inspectorWidth, contentHeight},
        {gap, height - gap - footerHeight, width - gap * 2, footerHeight},
        compact
    };
}
}
