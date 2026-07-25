#pragma once

#include "imgui.h"

namespace vlsui
{

inline ImVec4 Accent()
{
	return ImVec4(0.25f, 0.55f, 0.96f, 1.00f);
}

inline ImVec4 AccentHovered()
{
	return ImVec4(0.32f, 0.62f, 1.00f, 1.00f);
}

inline ImVec4 Success()
{
	return ImVec4(0.20f, 0.78f, 0.55f, 1.00f);
}

inline ImVec4 Warning()
{
	return ImVec4(0.96f, 0.62f, 0.24f, 1.00f);
}

inline ImVec4 Muted()
{
	return ImVec4(0.56f, 0.62f, 0.72f, 1.00f);
}

inline ImVec4 Panel()
{
	return ImVec4(0.055f, 0.075f, 0.115f, 1.00f);
}

inline void ApplyModernTheme()
{
	ImGui::StyleColorsDark();

	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowPadding = ImVec2(15.0f, 14.0f);
	style.FramePadding = ImVec2(10.0f, 7.0f);
	style.CellPadding = ImVec2(8.0f, 6.0f);
	style.ItemSpacing = ImVec2(8.0f, 8.0f);
	style.ItemInnerSpacing = ImVec2(7.0f, 5.0f);
	style.IndentSpacing = 18.0f;
	style.ScrollbarSize = 12.0f;
	style.GrabMinSize = 12.0f;
	style.WindowBorderSize = 1.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupBorderSize = 1.0f;
	style.FrameBorderSize = 0.0f;
	style.WindowRounding = 10.0f;
	style.ChildRounding = 8.0f;
	style.FrameRounding = 7.0f;
	style.PopupRounding = 8.0f;
	style.ScrollbarRounding = 9.0f;
	style.GrabRounding = 6.0f;
	style.TabRounding = 7.0f;

	ImVec4* colors = style.Colors;
	colors[ImGuiCol_Text] = ImVec4(0.92f, 0.94f, 0.97f, 1.00f);
	colors[ImGuiCol_TextDisabled] = Muted();
	colors[ImGuiCol_WindowBg] = Panel();
	colors[ImGuiCol_ChildBg] = ImVec4(0.070f, 0.095f, 0.145f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.075f, 0.100f, 0.150f, 0.98f);
	colors[ImGuiCol_Border] = ImVec4(0.18f, 0.24f, 0.34f, 0.75f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.105f, 0.145f, 0.215f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.145f, 0.205f, 0.305f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.18f, 0.27f, 0.41f, 1.00f);
	colors[ImGuiCol_TitleBg] = Panel();
	colors[ImGuiCol_TitleBgActive] = Panel();
	colors[ImGuiCol_TitleBgCollapsed] = Panel();
	colors[ImGuiCol_MenuBarBg] = Panel();
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.05f, 0.07f, 0.11f, 0.80f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.22f, 0.29f, 0.40f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.29f, 0.38f, 0.52f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = Accent();
	colors[ImGuiCol_CheckMark] = Accent();
	colors[ImGuiCol_SliderGrab] = Accent();
	colors[ImGuiCol_SliderGrabActive] = AccentHovered();
	colors[ImGuiCol_Button] = ImVec4(0.115f, 0.165f, 0.245f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.16f, 0.235f, 0.35f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.19f, 0.30f, 0.46f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.13f, 0.19f, 0.29f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.17f, 0.25f, 0.38f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.20f, 0.31f, 0.48f, 1.00f);
	colors[ImGuiCol_Separator] = ImVec4(0.18f, 0.24f, 0.34f, 0.80f);
	colors[ImGuiCol_SeparatorHovered] = Accent();
	colors[ImGuiCol_SeparatorActive] = AccentHovered();
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.25f, 0.55f, 0.96f, 0.25f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.25f, 0.55f, 0.96f, 0.65f);
	colors[ImGuiCol_ResizeGripActive] = Accent();
	colors[ImGuiCol_Tab] = ImVec4(0.085f, 0.12f, 0.18f, 1.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.17f, 0.25f, 0.38f, 1.00f);
	colors[ImGuiCol_TabActive] = ImVec4(0.15f, 0.25f, 0.39f, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.075f, 0.10f, 0.15f, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.12f, 0.19f, 0.29f, 1.00f);
	colors[ImGuiCol_TableHeaderBg] = ImVec4(0.10f, 0.145f, 0.22f, 1.00f);
	colors[ImGuiCol_TableBorderStrong] = ImVec4(0.18f, 0.24f, 0.34f, 1.00f);
	colors[ImGuiCol_TableBorderLight] = ImVec4(0.15f, 0.20f, 0.29f, 1.00f);
	colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.11f, 0.15f, 0.22f, 0.45f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 0.55f, 0.96f, 0.35f);
	colors[ImGuiCol_NavHighlight] = Accent();
}

inline void SectionTitle(const char* title, const char* hint = nullptr)
{
	ImGui::Spacing();
	ImGui::TextColored(ImVec4(0.78f, 0.84f, 0.93f, 1.00f), "%s", title);
	if (hint)
	{
		ImGui::SameLine();
		ImGui::TextDisabled("%s", hint);
	}
	ImGui::Separator();
}

inline bool ColoredButton(const char* label, const ImVec2& size, const ImVec4& color)
{
	ImVec4 hovered(
		(color.x + 0.10f > 1.0f) ? 1.0f : color.x + 0.10f,
		(color.y + 0.10f > 1.0f) ? 1.0f : color.y + 0.10f,
		(color.z + 0.10f > 1.0f) ? 1.0f : color.z + 0.10f,
		color.w);
	ImVec4 active(color.x * 0.82f, color.y * 0.82f, color.z * 0.82f, color.w);

	ImGui::PushStyleColor(ImGuiCol_Button, color);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hovered);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, active);
	bool pressed = ImGui::Button(label, size);
	ImGui::PopStyleColor(3);
	return pressed;
}

inline bool ModeButton(const char* label, bool selected, const ImVec2& size)
{
	if (selected)
	{
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.38f, 0.66f, 1.00f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.45f, 0.76f, 1.00f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.16f, 0.34f, 0.60f, 1.00f));
	}

	bool pressed = ImGui::Button(label, size);

	if (selected)
	{
		ImGui::PopStyleColor(3);
	}

	return pressed;
}

inline void ItemTooltip(const char* text)
{
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("%s", text);
	}
}

inline float HalfWidth()
{
	float width = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
	return (width > 1.0f) ? width : 1.0f;
}

inline void DrawStatus(bool running)
{
	const ImVec4 color = running ? Success() : Warning();
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	ImVec2 cursor = ImGui::GetCursorScreenPos();
	float radius = 4.5f;
	drawList->AddCircleFilled(ImVec2(cursor.x + radius, cursor.y + ImGui::GetTextLineHeight() * 0.5f), radius, ImGui::ColorConvertFloat4ToU32(color));
	ImGui::Dummy(ImVec2(radius * 2.0f + 7.0f, 1.0f));
	ImGui::SameLine(0.0f, 0.0f);
	ImGui::TextColored(color, "%s", running ? "Симуляция идет" : "Пауза");
}

}
