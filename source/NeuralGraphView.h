#pragma once

#include "UITheme.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>
#include <utility>

namespace vlsui
{
struct GraphNeuron
{
	int layer, index, type;
	const char* caption;
	const char* typeName;
	float value;
};

struct GraphConnection
{
	int layer, index, destinationLayer, destinationIndex;
	float weight;
};

// Pure ImGui canvas: geometry, clipping and input follow the owning window.
// No SDL overlay and no fixed screen coordinates are used by the brain viewer.
struct NeuralGraphView
{
	int selectedLayer = -1, selectedIndex = -1;
	float zoomPercent = 100.0f;
	bool selectedConnectionsOnly = false;
	ImVec2 pan = ImVec2(0, 0);
	ImVec2 canvasMin, canvasMax;
	// Input events carry coordinates, never pointers into a bot's genome.
	bool contextRequested = false;
	int contextLayer = -1, contextIndex = -1, contextTargetLayer = -1, contextTargetIndex = -1;
	int linkSourceLayer = -1, linkSourceIndex = -1;
	int chosenTargetLayer = -1, chosenTargetIndex = -1;
	std::vector<std::pair<int, int>> linkTargets;

	bool IsConnecting() const { return linkSourceLayer >= 0; }
	bool CanLinkTo(int layer, int index) const
	{
		return std::find(linkTargets.begin(), linkTargets.end(), std::pair<int, int>(layer, index)) != linkTargets.end();
	}
	void CancelLink()
	{
		linkSourceLayer = linkSourceIndex = -1;
		linkTargets.clear();
	}

	void ClearSelection()
	{
		selectedLayer = selectedIndex = -1;
		CancelLink();
		contextRequested = false;
		chosenTargetLayer = chosenTargetIndex = -1;
	}
	void Fit()
	{
		zoomPercent = 100.0f;
		pan = ImVec2(0, 0);
	}

	ImVec2 NeuronPosition(int layer, int index) const
	{
		return ImVec2(center.x + pan.x + (layer * 90.0f - (counts.size() - 1) * 45.0f) * scale,
					  center.y + pan.y + (index - (counts[layer] - 1) * 0.5f) * 44.0f * scale);
	}

	static ImVec2 CurvePoint(const ImVec2& a, const ImVec2& b, float t)
	{
		const float bend = (b.x - a.x) * .40f;
		const float u = 1 - t;
		return ImVec2(u*u*u*a.x + 3*u*u*t*(a.x+bend) + 3*u*t*t*(b.x-bend) + t*t*t*b.x,
			u*u*u*a.y + 3*u*u*t*a.y + 3*u*t*t*b.y + t*t*t*b.y);
	}
	static void DrawConnection(ImDrawList* draw, const ImVec2& a, const ImVec2& b, ImU32 color, float thickness,
		bool arrow)
	{
		const float bend = (b.x - a.x) * .40f;
		draw->AddBezierCubic(a, ImVec2(a.x+bend, a.y), ImVec2(b.x-bend, b.y), b, color, thickness);
		if (arrow)
		{
			const ImVec2 tip = CurvePoint(a, b, .65f), tail = CurvePoint(a, b, .60f);
			const float length = (std::max)(.01f, std::hypot(tip.x-tail.x, tip.y-tail.y));
			const float dx = (tip.x-tail.x)/length, dy = (tip.y-tail.y)/length;
			draw->AddTriangleFilled(tip, ImVec2(tip.x-8*dx+3*dy, tip.y-8*dy-3*dx),
				ImVec2(tip.x-8*dx-3*dy, tip.y-8*dy+3*dx), color);
		}
	}

	static ImVec4 Positive()
	{
		return ImVec4(0.90f, 0.48f, 0.38f, 1);
	}
	static ImVec4 Negative()
	{
		return ImVec4(0.40f, 0.65f, 0.94f, 1);
	}

	void Draw(const std::vector<GraphNeuron>& neurons, const std::vector<GraphConnection>& connections,
			  const std::vector<int>& layerCounts, const ImVec2& size)
	{
		contextRequested = false;
		chosenTargetLayer = chosenTargetIndex = -1;
		counts = layerCounts;
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.065f, 0.095f, 0.105f, 1));
		ImGui::BeginChild("##NeuralGraphCanvas", size, true,
						  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		canvasMin = ImGui::GetCursorScreenPos();
		ImVec2 available = ImGui::GetContentRegionAvail();
		available.x = (std::max)(1.0f, available.x);
		available.y = (std::max)(1.0f, available.y);
		canvasMax = ImVec2(canvasMin.x + available.x, canvasMin.y + available.y);
		// Reserve a small strip for the mouse hint above the network.
		center = ImVec2((canvasMin.x + canvasMax.x) * 0.5f, (canvasMin.y + canvasMax.y + 24.0f) * 0.5f + 8.0f);
		ImGui::InvisibleButton("##GraphInput", available,
							   ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
		const bool hovered = ImGui::IsItemHovered();
		const ImVec2 mouse = ImGui::GetIO().MousePos;
		if (hovered && ImGui::GetIO().MouseWheel != 0)
		{
			const float oldZoom = zoomPercent;
			zoomPercent = std::clamp(zoomPercent * std::pow(1.12f, ImGui::GetIO().MouseWheel), 50.0f, 250.0f);
			const float ratio = zoomPercent / oldZoom;
			pan = ImVec2(mouse.x - center.x - (mouse.x - center.x - pan.x) * ratio,
						 mouse.y - center.y - (mouse.y - center.y - pan.y) * ratio);
		}
		if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
		{
			pan.x += ImGui::GetIO().MouseDelta.x;
			pan.y += ImGui::GetIO().MouseDelta.y;
		}
		int maxRows = 1;
		for (int count : counts)
			maxRows = (std::max)(maxRows, count);
		const float fitX = (available.x - 24.0f) / ((counts.size() - 1) * 90.0f + 64.0f);
		const float fitY = (available.y - 48.0f) / ((maxRows - 1) * 44.0f + 80.0f);
		scale = (std::max)(0.10f, (std::min)(1.0f, (std::min)(fitX, fitY))) * zoomPercent / 100.0f;
		const float radius = 8.0f * scale;
		const GraphNeuron* hoveredNeuron = nullptr;
		float nearest = (std::max)(11.0f, radius + 4.0f);
		const bool overPlot = hovered && mouse.y >= canvasMin.y + 24.0f;
		if (overPlot)
		{
			for (const auto& node : neurons)
			{
				const ImVec2 p = NeuronPosition(node.layer, node.index);
				const float distance = std::hypot(p.x - mouse.x, p.y - mouse.y);
				if (distance < nearest)
				{
					nearest = distance;
					hoveredNeuron = &node;
				}
			}
		}
		const GraphConnection* hoveredConnection = nullptr;
		float nearestEdge = 6.0f;
		if (overPlot && !hoveredNeuron && !IsConnecting())
			for (const auto& edge : connections)
			{
				if (!Visible(edge)) continue;
				const ImVec2 a = NeuronPosition(edge.layer, edge.index), b = NeuronPosition(edge.destinationLayer, edge.destinationIndex);
				ImVec2 previous = a;
				for (int segment = 1; segment <= 32; ++segment)
				{
					const ImVec2 point = CurvePoint(a, b, segment / 32.0f);
					const float dx = point.x - previous.x, dy = point.y - previous.y;
					const float t = std::clamp(((mouse.x-previous.x)*dx + (mouse.y-previous.y)*dy) /
						(std::max)(.01f, dx*dx+dy*dy), 0.0f, 1.0f);
					const float distance = std::hypot(mouse.x-previous.x-t*dx, mouse.y-previous.y-t*dy);
					if (distance < nearestEdge) { nearestEdge = distance; hoveredConnection = &edge; }
					previous = point;
				}
			}
		if (IsConnecting() && ImGui::IsKeyPressed(ImGuiKey_Escape)) CancelLink();
		if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
		{
			if (IsConnecting()) CancelLink();
			else
			{
				contextRequested = true;
				contextLayer = contextIndex = contextTargetLayer = contextTargetIndex = -1;
				if (hoveredNeuron)
				{
					selectedLayer = contextLayer = hoveredNeuron->layer;
					selectedIndex = contextIndex = hoveredNeuron->index;
				}
				else if (hoveredConnection)
				{
					selectedLayer = contextLayer = hoveredConnection->layer;
					selectedIndex = contextIndex = hoveredConnection->index;
					contextTargetLayer = hoveredConnection->destinationLayer;
					contextTargetIndex = hoveredConnection->destinationIndex;
				}
			}
		}
		if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			if (IsConnecting())
			{
				if (hoveredNeuron && CanLinkTo(hoveredNeuron->layer, hoveredNeuron->index))
				{
					chosenTargetLayer = hoveredNeuron->layer;
					chosenTargetIndex = hoveredNeuron->index;
				}
			}
			else
			{
				ClearSelection();
				if (hoveredNeuron) { selectedLayer = hoveredNeuron->layer; selectedIndex = hoveredNeuron->index; }
			}
		}
		ImDrawList* draw = ImGui::GetWindowDrawList();
		draw->PushClipRect(ImVec2(canvasMin.x, canvasMin.y+24.0f), canvasMax, true);
		for (const auto& edge : connections)
		{
			if (!Visible(edge)) continue;
			const bool attached = (edge.layer == selectedLayer && edge.index == selectedIndex) ||
				(edge.destinationLayer == selectedLayer && edge.destinationIndex == selectedIndex);
			ImVec4 color = edge.weight > 0 ? Positive() : edge.weight < 0 ? Negative() : Muted();
			color.w = &edge == hoveredConnection ? 1.0f : selectedLayer >= 0 ? (attached ? .95f : .14f) : .40f;
			DrawConnection(draw, NeuronPosition(edge.layer, edge.index), NeuronPosition(edge.destinationLayer, edge.destinationIndex),
				ImGui::ColorConvertFloat4ToU32(color), &edge == hoveredConnection ? 3.0f : attached ? 1.8f : 1.0f, attached || &edge == hoveredConnection);
		}
		if (IsConnecting())
		{
			const ImVec2 target = hoveredNeuron ? NeuronPosition(hoveredNeuron->layer, hoveredNeuron->index) : mouse;
			const bool valid = hoveredNeuron && CanLinkTo(hoveredNeuron->layer, hoveredNeuron->index);
			DrawConnection(draw, NeuronPosition(linkSourceLayer, linkSourceIndex), target,
				ImGui::ColorConvertFloat4ToU32(valid ? Accent() : Muted()), 2.0f, true);
			if (hovered) ImGui::SetMouseCursor(valid ? ImGuiMouseCursor_Hand : ImGuiMouseCursor_NotAllowed);
		}
		for (int layer = 0; layer < (int)counts.size(); ++layer)
		{
			char label[32];
			if (layer == 0)
				snprintf(label, sizeof(label), "Входы");
			else if (layer == (int)counts.size() - 1)
				snprintf(label, sizeof(label), "Действия");
			else if (scale < .35f)
				continue;
			else if (scale < .65f)
				snprintf(label, sizeof(label), "%d", layer + 1);
			else
				snprintf(label, sizeof(label), "Слой %d", layer + 1);
			const ImVec2 position = NeuronPosition(layer, 0);
			const float top = center.y + pan.y - (maxRows - 1) * 22.0f * scale - 30.0f;
			draw->AddText(ImVec2(position.x - ImGui::CalcTextSize(label).x * 0.5f, top),
						  ImGui::ColorConvertFloat4ToU32(Muted()), label);
		}
		for (const auto& node : neurons)
		{
			const ImVec4 colors[] = {ImVec4(.58f, .67f, .68f, 1), ImVec4(.88f, .70f, .39f, 1),
									 ImVec4(.32f, .77f, .62f, 1), ImVec4(.43f, .65f, .93f, 1),
									 ImVec4(.84f, .87f, .88f, 1), ImVec4(.70f, .51f, .83f, 1)};
			const ImVec2 p = NeuronPosition(node.layer, node.index);
			draw->AddCircleFilled(p, radius, ImGui::ColorConvertFloat4ToU32(colors[std::clamp(node.type, 0, 5)]));
			if (IsConnecting() && CanLinkTo(node.layer, node.index))
				draw->AddCircle(p, radius + 6.0f, ImGui::ColorConvertFloat4ToU32(Accent()), 0, 2.0f);
			if (node.layer == selectedLayer && node.index == selectedIndex)
				draw->AddCircle(p, radius + 4.0f, ImGui::GetColorU32(ImGuiCol_Text), 0, 1.5f);
		}
		draw->PopClipRect();
		draw->PushClipRect(canvasMin, canvasMax, true);
		draw->AddText(ImVec2(canvasMin.x+2, canvasMin.y+2), ImGui::ColorConvertFloat4ToU32(Muted()),
			IsConnecting() ? "ЛКМ по подсвеченной цели · Esc / ПКМ — отмена" : "ПКМ — меню нейрона или связи");
		draw->PopClipRect();
		if (hoveredNeuron)
		{
			ImGui::BeginTooltip();
			ImGui::Text("Слой %d · нейрон %d", hoveredNeuron->layer + 1, hoveredNeuron->index + 1);
			ImGui::TextUnformatted(hoveredNeuron->typeName);
			if (hoveredNeuron->caption && *hoveredNeuron->caption)
				ImGui::TextUnformatted(hoveredNeuron->caption);
			ImGui::Text("Значение: %.3f", hoveredNeuron->value);
			if (IsConnecting())
				ImGui::TextUnformatted(CanLinkTo(hoveredNeuron->layer, hoveredNeuron->index) ? "ЛКМ — выбрать цель" :
					"Недоступная цель: нужен следующий слой и новая связь.");
			ImGui::EndTooltip();
		}
		else if (hoveredConnection)
		{
			ImGui::BeginTooltip();
			ImGui::Text("%d:%d → %d:%d · вес %+.3f", hoveredConnection->layer+1, hoveredConnection->index+1,
				hoveredConnection->destinationLayer+1, hoveredConnection->destinationIndex+1, hoveredConnection->weight);
			ImGui::TextUnformatted("ПКМ — изменить вес или удалить связь");
			ImGui::EndTooltip();
		}
		ImGui::EndChild();
		ImGui::PopStyleColor();
	}

  private:
	bool Visible(const GraphConnection& edge) const
	{
		if (edge.destinationLayer < 0 || edge.destinationLayer >= (int)counts.size() || edge.destinationIndex < 0 ||
			edge.destinationIndex >= counts[edge.destinationLayer]) return false;
		const bool attached = (edge.layer == selectedLayer && edge.index == selectedIndex) ||
			(edge.destinationLayer == selectedLayer && edge.destinationIndex == selectedIndex);
		return !selectedConnectionsOnly || selectedLayer < 0 || attached;
	}
	std::vector<int> counts;
	ImVec2 center;
	float scale = 1.0f;
};
} // namespace vlsui
