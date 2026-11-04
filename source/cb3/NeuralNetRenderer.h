#pragma once
#include "../NeuralGraphView.h"
#include "Bot.h"
#include "../BrainEditHistory.h"
#include "SDL.h"
#include "../BrainGraphEditor.h"

namespace cb3
{

struct NeuralNetRenderer final
{
	void Render(const Bot::BrainView* brain, const ImVec2& size);
	void ResetSelection();
	void DrawEditor(const Bot::BrainView* brain, int version)
	{
		editor.Draw(view, *brain, history, version, layerCounts, editorRules);
	}

	vlsui::BrainEditHistory<Neuron, NumNeuronLayers, NumNeuronsInLayerMax> history;
	vlsui::NeuralGraphView view;
	vlsui::BrainGraphEditor editor;
	vlsui::BrainGraphRules editorRules{};
	std::vector<int> layerCounts;
	ImVec2 windowPosition, windowSize, displaySize = ImVec2(0, 0);
	Neuron* selectedNeuron = nullptr;
	// Neurons of the shown brain version (identifies bot and version)
	const NeuronRow* selectedBrain = nullptr;
	const char* selectedNeuronCaption = "";
	const char* selectedNeuronTypeName = "";
	float selectedNeuronValue = 0.0f;
	float selectedNeuronMemory = 0.0f;
};
} // namespace cb3
