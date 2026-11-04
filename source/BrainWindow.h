#pragma once

#include "NeuralGraphView.h"

namespace vlsui
{
inline constexpr const char* BrainWindowTitle = "Мозг бота###BotBrainWindow";

template <class Brain, class Renderer, class Summary>
void DrawBrainWindow(bool& open, int& version, Brain* current, Brain* initial, Renderer& renderer,
					 const Summary& summary, float biasScale, float weightScale, bool allowBrainMutation)
{
	if (!open)
		return;
	const ImVec2 display = ImGui::GetIO().DisplaySize;
	const ImVec2 maximum((std::max)(1.0f, display.x - 24.0f), (std::max)(1.0f, display.y - 24.0f));
	ImGui::SetNextWindowSizeConstraints(ImVec2((std::min)(480.0f, maximum.x), (std::min)(320.0f, maximum.y)), maximum);
	ImGui::SetNextWindowSize(ImVec2((std::min)(800.0f, maximum.x), (std::min)(540.0f, maximum.y)),
							 ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos(ImVec2(display.x * 0.5f, display.y * 0.5f), ImGuiCond_FirstUseEver, ImVec2(.5f, .5f));
	if (renderer.displaySize.x > 0 && (renderer.displaySize.x != display.x || renderer.displaySize.y != display.y))
	{
		const ImVec2 size((std::min)(renderer.windowSize.x, maximum.x), (std::min)(renderer.windowSize.y, maximum.y));
		ImGui::SetNextWindowPos(
			ImVec2(std::clamp(renderer.windowPosition.x, 12.0f, (std::max)(12.0f, display.x - size.x - 12.0f)),
				   std::clamp(renderer.windowPosition.y, 12.0f, (std::max)(12.0f, display.y - size.y - 12.0f))));
	}
	if (!ImGui::Begin(BrainWindowTitle, &open, ImGuiWindowFlags_NoCollapse))
	{
		ImGui::End();
		return;
	}
	renderer.windowPosition = ImGui::GetWindowPos();
	renderer.windowSize = ImGui::GetWindowSize();
	renderer.displaySize = display;
	if (!open)
	{
		ImGui::End();
		return;
	}

	// Compact segmented switch, with both choices visible, in the graph's toolbar.
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(2, 8));
	if (ModeButton("Текущий", version == 0, ImVec2(92, 28)))
		version = 0;
	ImGui::SameLine();
	if (ModeButton("Начальный", version == 1, ImVec2(104, 28)))
		version = 1;
	ImGui::PopStyleVar();
	ImGui::SameLine();
	if (ImGui::Button("Вписать", ImVec2(82, 28)))
		renderer.view.Fit();
	ItemTooltip("Вписать схему в окно. Колесо — масштаб, средняя кнопка мыши — сдвиг.");
	ImGui::SameLine();
	ImGui::SetNextItemWidth((std::min)(170.0f, ImGui::GetContentRegionAvail().x));
	ImGui::SliderFloat("##BrainZoom", &renderer.view.zoomPercent, 50, 250, "%.0f %%", ImGuiSliderFlags_AlwaysClamp);
	ItemTooltip("Масштаб схемы относительно размера окна");
	ImGui::Checkbox("Только связи выбранного нейрона", &renderer.view.selectedConnectionsOnly);

	auto details = [&]() {
		auto neuron = renderer.selectedNeuron;
		if (neuron)
		{
			ImGui::Text("Слой %d · нейрон %d", renderer.view.selectedLayer + 1, renderer.view.selectedIndex + 1);
			ImGui::TextUnformatted(renderer.selectedNeuronTypeName);
			if (*renderer.selectedNeuronCaption)
				ImGui::TextWrapped("%s", renderer.selectedNeuronCaption);
			ImGui::Text("Значение: %.3f", renderer.selectedNeuronValue);
			if ((int)neuron->type == 5)
				ImGui::Text("Память: %.3f", renderer.selectedNeuronMemory);
			ImGui::Text("Смещение: %.3f", neuron->bias * biasScale);
			SectionTitle("Связи");
			if (ImGui::BeginTable("##NeuronConnections", 3, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_RowBg))
			{
				ImGui::TableSetupColumn("Слой");
				ImGui::TableSetupColumn("Нейрон");
				ImGui::TableSetupColumn("Вес");
				ImGui::TableHeadersRow();
				for (unsigned i = 0; i < neuron->numConnections; ++i)
				{
					const auto& edge = neuron->allConnections[i];
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::Text("%d", edge.dest_layer + 1);
					ImGui::TableSetColumnIndex(1);
					ImGui::Text("%d", edge.dest_neuron + 1);
					ImGui::TableSetColumnIndex(2);
					ImGui::Text("%+.3f", edge.weight * weightScale);
				}
				ImGui::EndTable();
			}
			const bool editable = (int)neuron->type != 1 && (int)neuron->type != 2;
			ImGui::BeginDisabled(!editable);
			if (ImGui::Button("Случайный", ImVec2(HalfWidth(), 30)))
				neuron->SetRandom();
			ImGui::SameLine();
			if (ImGui::Button("Обнулить", ImVec2(-1, 30)))
				neuron->SetZero();
			ImGui::EndDisabled();
		}
		else
		{
			ImGui::TextUnformatted("Выберите нейрон");
			ImGui::TextWrapped("Нажмите на узел схемы: здесь появятся его параметры и связи.");
		}
		if (allowBrainMutation && ImGui::Button("Мутировать мозг", ImVec2(-1, 30)))
			(version == 0 ? current : initial)->Mutate();
		if (ImGui::CollapsingHeader("Сводка бота"))
		{
			ImGui::Text("Простые: %d", summary.simple);
			ImGui::Text("Радиальные: %d", summary.radialBasis);
			ImGui::Text("Случайные: %d", summary.random);
			ImGui::Text("Память: %d", summary.memory);
			ImGui::Text("Нейронов: %d", summary.neurons);
			ImGui::Text("Тупиковых: %d", summary.deadend);
			ImGui::Text("Связей: %d", summary.connections);
		}
	};
	const bool wide = ImGui::GetContentRegionAvail().x >= 720.0f;
	const float bodyHeight =
		(std::max)(120.0f, ImGui::GetContentRegionAvail().y - 2.0f * ImGui::GetStyle().CellPadding.y - 4.0f);
	if (wide && ImGui::BeginTable("##BrainBody", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp))
	{
		ImGui::TableSetupColumn("##Graph", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("##Neuron", ImGuiTableColumnFlags_WidthFixed, 230.0f);
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		renderer.Render(version == 0 ? current : initial, ImVec2(0, (std::max)(100.0f, bodyHeight - 28.0f)));
		ImGui::TextColored(NeuralGraphView::Positive(), "+ Связи");
		ImGui::SameLine();
		ImGui::TextColored(NeuralGraphView::Negative(), "- Связи");
		ImGui::TableSetColumnIndex(1);
		ImGui::BeginChild("##NeuronDetails", ImVec2(0, bodyHeight), false);
		details();
		ImGui::EndChild();
		ImGui::EndTable();
	}
	else if (!wide)
	{
		renderer.Render(version == 0 ? current : initial,
						ImVec2(0, (std::max)(120.0f, ImGui::GetContentRegionAvail().y * .58f)));
		ImGui::BeginChild("##NeuronDetails", ImVec2(0, 0), false);
		details();
		ImGui::EndChild();
	}
	ImGui::End();
}
} // namespace vlsui
