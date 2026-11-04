#pragma once

#include "imgui.h"
#include "UILayout.h"
#include <cfloat>

namespace vlsui
{

inline void AddInterfaceFont(ImGuiIO& io)
{
	// Keep the ranges alive until the atlas is built, including punctuation used
	// by Russian labels. The stock Cyrillic range omits bullets and dashes.
	static ImVector<ImWchar> ranges;
	if (ranges.empty())
	{
		ImFontGlyphRangesBuilder builder;
		builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic());
		builder.AddText("• — – →");
		builder.BuildRanges(&ranges);
	}
	io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 16.0f, nullptr, ranges.Data);
}

inline ImVec4 Accent()
{
	return ImVec4(0.14f, 0.48f, 0.40f, 1.00f);
}

inline ImVec4 AccentHovered()
{
	return ImVec4(0.19f, 0.58f, 0.48f, 1.00f);
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
	return ImVec4(0.65f, 0.72f, 0.72f, 1.00f);
}

inline ImVec4 Panel()
{
	return ImVec4(0.10f, 0.14f, 0.15f, 1.00f);
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
	colors[ImGuiCol_ChildBg] = ImVec4(0.085f, 0.12f, 0.13f, 1.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.14f, 0.15f, 1.00f);
	colors[ImGuiCol_Border] = ImVec4(0.22f, 0.28f, 0.29f, 0.75f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.14f, 0.19f, 0.20f, 1.00f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.27f, 0.28f, 1.00f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.23f, 0.33f, 0.32f, 1.00f);
	colors[ImGuiCol_TitleBg] = Panel();
	colors[ImGuiCol_TitleBgActive] = Panel();
	colors[ImGuiCol_TitleBgCollapsed] = Panel();
	colors[ImGuiCol_MenuBarBg] = Panel();
	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.07f, 0.10f, 0.11f, 0.80f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.29f, 0.37f, 0.38f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.38f, 0.47f, 0.47f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = Accent();
	colors[ImGuiCol_CheckMark] = Accent();
	colors[ImGuiCol_SliderGrab] = Accent();
	colors[ImGuiCol_SliderGrabActive] = AccentHovered();
	colors[ImGuiCol_Button] = ImVec4(0.16f, 0.22f, 0.23f, 1.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.22f, 0.30f, 0.30f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.26f, 0.36f, 0.35f, 1.00f);
	colors[ImGuiCol_Header] = ImVec4(0.16f, 0.22f, 0.23f, 1.00f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.22f, 0.30f, 0.30f, 1.00f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.36f, 0.35f, 1.00f);
	colors[ImGuiCol_Separator] = ImVec4(0.22f, 0.28f, 0.29f, 0.80f);
	colors[ImGuiCol_SeparatorHovered] = Accent();
	colors[ImGuiCol_SeparatorActive] = AccentHovered();
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.22f, 0.62f, 0.48f, 0.25f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.22f, 0.62f, 0.48f, 0.65f);
	colors[ImGuiCol_ResizeGripActive] = Accent();
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
	colors[ImGuiCol_NavHighlight] = Accent();
	colors[ImGuiCol_PlotHistogram] = Success();
}

inline void PrepareToolWindow(const ImVec2& preferredSize, const ImVec2& minimumSize)
{
	const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
	const float margin = 24.0f;

	ImVec2 maximumSize = preferredSize;
	if (displaySize.x > margin * 2.0f)
		maximumSize.x = displaySize.x - margin * 2.0f;
	if (displaySize.y > margin * 2.0f)
		maximumSize.y = displaySize.y - margin * 2.0f;

	ImVec2 clampedMinimum = minimumSize;
	if (clampedMinimum.x > maximumSize.x)
		clampedMinimum.x = maximumSize.x;
	if (clampedMinimum.y > maximumSize.y)
		clampedMinimum.y = maximumSize.y;

	ImVec2 initialSize = preferredSize;
	if (initialSize.x > maximumSize.x)
		initialSize.x = maximumSize.x;
	if (initialSize.y > maximumSize.y)
		initialSize.y = maximumSize.y;
	if (initialSize.x < clampedMinimum.x)
		initialSize.x = clampedMinimum.x;
	if (initialSize.y < clampedMinimum.y)
		initialSize.y = clampedMinimum.y;

	ImGui::SetNextWindowSizeConstraints(clampedMinimum, maximumSize);
	ImGui::SetNextWindowSize(initialSize, ImGuiCond_Appearing);

	if (displaySize.x > 0.0f && displaySize.y > 0.0f)
	{
		ImGui::SetNextWindowPos(
			ImVec2(displaySize.x * 0.5f, displaySize.y * 0.5f),
			ImGuiCond_Appearing,
			ImVec2(0.5f, 0.5f));
	}
}

inline void SectionTitle(const char* title, const char* hint = nullptr)
{
	ImGui::Spacing();
	ImGui::TextColored(ImVec4(0.80f, 0.87f, 0.86f, 1.00f), "%s", title);
	if (hint)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, Muted());
		ImGui::TextWrapped("%s", hint);
		ImGui::PopStyleColor();
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
		ImGui::PushStyleColor(ImGuiCol_Button, Accent());
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, AccentHovered());
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.10f, 0.38f, 0.32f, 1.00f));
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

inline void BeginWorkspaceWindow(const char* name, const Rect& rect)
{
	ImGui::SetNextWindowPos(ImVec2((float)rect.x, (float)rect.y));
	ImGui::SetNextWindowSize(ImVec2((float)rect.w, (float)rect.h));
	ImGui::Begin(name, nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
}

struct HeaderActions
{
	bool seed = false, pause = false, step = false, saves = false;
};

inline HeaderActions DrawWorkspaceHeader(const WorkspaceLayout& layout, const char* mode,
	bool running, int& tpsLimit, int maxTPS, int& renderMode)
{
	HeaderActions actions;
	BeginWorkspaceWindow("##WorkspaceHeader", layout.header);
	ImGui::TextUnformatted("VIRTUAL LIFE");
	ImGui::SameLine();
	ImGui::TextDisabled("/ %s", mode);
	ImGui::SameLine((float)layout.header.w - 210.0f);
	ImGui::SetNextItemWidth(180.0f);
	ImGui::Combo("##WorldDisplay", &renderMode,
		"Родственные линии\0Источники питания\0Запас энергии\0Без отрисовки\0");
	ItemTooltip("Режим отображения мира · Num 1–4");
	ImGui::Spacing();
	actions.seed = ColoredButton("Заселить мир", ImVec2(155.0f, 36.0f), Accent());
	ItemTooltip("F1 · Добавить стартовую группу случайных ботов к текущему миру");
	ImGui::SameLine();
	actions.pause = ImGui::Button(running ? "Пауза" : "Запустить", ImVec2(108.0f, 36.0f));
	ItemTooltip("Space / Pause · запуск и остановка симуляции");
	ImGui::SameLine();
	ImGui::BeginDisabled(running);
	actions.step = ImGui::Button("Один шаг", ImVec2(100.0f, 36.0f));
	ImGui::EndDisabled();
	ItemTooltip("Num + · шаг симуляции на паузе");
	ImGui::SameLine();
	if (!layout.compact)
	{
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 188.0f);
		ImGui::SliderInt("##HeaderTPS", &tpsLimit, 0, maxTPS, "%d тиков/с");
		ItemTooltip("Лимит скорости симуляции · 0 = без ограничения");
		ImGui::SameLine();
	}
	actions.saves = ImGui::Button("Сохранения", ImVec2(180.0f, 36.0f));
	ItemTooltip("Z · сохранение и загрузка миров и отдельных ботов");
	if (layout.compact)
	{
		ImGui::AlignTextToFramePadding();
		ImGui::TextDisabled("Скорость");
		ImGui::SameLine();
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::SliderInt("##HeaderTPS", &tpsLimit, 0, maxTPS, "%d тиков/с");
		ItemTooltip("0 = без ограничения скорости");
	}
	ImGui::End();
	return actions;
}

inline int DrawWorkspaceTools(const WorkspaceLayout& layout, int tool, int& brush, int minimumBrush)
{
	BeginWorkspaceWindow("##WorkspaceTools", layout.tools);
	ImGui::TextDisabled("МЫШЬ");
	const char* names[] = {"Выбор", "Удалить", "Камень", "Из файла", "Мутация"};
	const char* hints[] = {
		"Нажмите на бота, чтобы изучить его",
		"Удалять объекты в области кисти",
		"Размещать камни в области кисти",
		"Размещать выбранного в сохранениях бота",
		"Мутировать ботов в области кисти"
	};
	const int order[] = {0, 2, 3, 4, 1};
	for (int value : order)
	{
		if (ModeButton(names[value], tool == value, ImVec2(-1.0f, 34.0f)))
			tool = value;
		ItemTooltip(hints[value]);
	}
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::TextWrapped("Радиус кисти");
	ImGui::SetNextItemWidth(-1.0f);
	ImGui::SliderInt("##ToolBrush", &brush, minimumBrush, 50, "%d");
	ImGui::TextWrapped("%s", hints[tool]);
	ImGui::End();
	return tool;
}

inline bool DrawEmptyWorld(const Rect& world)
{
	const float width = (world.w < 430) ? (float)world.w : 430.0f;
	const float height = (world.h < 230) ? (float)world.h : 230.0f;
	Rect card = {world.x + (world.w - (int)width) / 2, world.y + (world.h - (int)height) / 2,
		(int)width, (int)height};
	BeginWorkspaceWindow("##FirstPopulation", card);
	ImGui::TextWrapped(world.w < 300 ? "Мир пока пуст" : "Всё начинается с первой популяции");
	ImGui::Spacing();
	ImGui::TextWrapped(world.w < 300 ? "Создайте первую популяцию." : "Заселите мир случайными ботами, затем запустите симуляцию.");
	ImGui::Spacing();
	const bool seed = ColoredButton("Заселить мир", ImVec2(-1.0f, 36.0f), Accent());
	ItemTooltip("F1 · существующие объекты и параметры мира сохраняются");
	if (height >= 220.0f)
		ImGui::TextWrapped("Или откройте мир через «Сохранения».");
	ImGui::End();
	return seed;
}

inline void DrawPopulationPreview(const float* values, int count, const ImVec2& size)
{
	if (count > 1)
	{
		ImGui::PushStyleColor(ImGuiCol_PlotLines, Success());
		ImGui::PlotLines("##PopulationPreview", values, count, 0, nullptr, 0.0f, FLT_MAX, size);
		ImGui::PopStyleColor();
	}
	else
	{
		ImGui::TextDisabled("Ожидание данных");
		ItemTooltip("История популяции обновляется во время симуляции");
	}
}

}
