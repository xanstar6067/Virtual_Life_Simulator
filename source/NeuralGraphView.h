#pragma once

#include "UITheme.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

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

	void ClearSelection()
	{
		selectedLayer = selectedIndex = -1;
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
		counts = layerCounts;
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.065f, 0.095f, 0.105f, 1));
		ImGui::BeginChild("##NeuralGraphCanvas", size, true,
						  ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		canvasMin = ImGui::GetCursorScreenPos();
		ImVec2 available = ImGui::GetContentRegionAvail();
		available.x = (std::max)(1.0f, available.x);
		available.y = (std::max)(1.0f, available.y);
		canvasMax = ImVec2(canvasMin.x + available.x, canvasMin.y + available.y);
		center = ImVec2((canvasMin.x + canvasMax.x) * 0.5f, (canvasMin.y + canvasMax.y) * 0.5f + 8.0f);
		ImGui::InvisibleButton("##GraphInput", available,
							   ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle);
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
		const float fitY = (available.y - 24.0f) / ((maxRows - 1) * 44.0f + 80.0f);
		scale = (std::max)(0.10f, (std::min)(1.0f, (std::min)(fitX, fitY))) * zoomPercent / 100.0f;
		const float radius = 8.0f * scale;
		const GraphNeuron* hoveredNeuron = nullptr;
		float nearest = (std::max)(11.0f, radius + 4.0f);
		if (hovered)
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
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
			{
				ClearSelection();
				if (hoveredNeuron)
				{
					selectedLayer = hoveredNeuron->layer;
					selectedIndex = hoveredNeuron->index;
				}
			}
		}
		ImDrawList* draw = ImGui::GetWindowDrawList();
		draw->PushClipRect(canvasMin, canvasMax, true);
		for (const auto& edge : connections)
		{
			if (edge.destinationLayer < 0 || edge.destinationLayer >= (int)counts.size() || edge.destinationIndex < 0 ||
				edge.destinationIndex >= counts[edge.destinationLayer])
				continue;
			const bool attached = (edge.layer == selectedLayer && edge.index == selectedIndex) ||
								  (edge.destinationLayer == selectedLayer && edge.destinationIndex == selectedIndex);
			if (selectedConnectionsOnly && selectedLayer >= 0 && !attached)
				continue;
			ImVec4 color = edge.weight > 0.15f ? Positive() : edge.weight < -0.15f ? Negative() : Muted();
			color.w = selectedLayer >= 0 ? (attached ? 0.95f : 0.14f) : 0.40f;
			const ImVec2 a = NeuronPosition(edge.layer, edge.index);
			const ImVec2 b = NeuronPosition(edge.destinationLayer, edge.destinationIndex);
			const float bend = (b.x - a.x) * 0.40f;
			draw->AddBezierCubic(a, ImVec2(a.x + bend, a.y), ImVec2(b.x - bend, b.y), b,
								 ImGui::ColorConvertFloat4ToU32(color), attached ? 1.8f : 1.0f);
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
			if (node.layer == selectedLayer && node.index == selectedIndex)
				draw->AddCircle(p, radius + 4.0f, ImGui::GetColorU32(ImGuiCol_Text), 0, 1.5f);
		}
		draw->PopClipRect();
		if (hoveredNeuron)
		{
			ImGui::BeginTooltip();
			ImGui::Text("Слой %d · нейрон %d", hoveredNeuron->layer + 1, hoveredNeuron->index + 1);
			ImGui::TextUnformatted(hoveredNeuron->typeName);
			if (hoveredNeuron->caption && *hoveredNeuron->caption)
				ImGui::TextUnformatted(hoveredNeuron->caption);
			ImGui::Text("Значение: %.3f", hoveredNeuron->value);
			ImGui::EndTooltip();
		}
		ImGui::EndChild();
		ImGui::PopStyleColor();
	}

  private:
	std::vector<int> counts;
	ImVec2 center;
	float scale = 1.0f;
};
} // namespace vlsui
