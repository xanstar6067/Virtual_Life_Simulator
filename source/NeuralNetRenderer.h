#pragma once
#include "Bot.h"
#include "NeuralGraphView.h"
#include "SDL.h"

struct NeuralNetRenderer final
{
	void Render(const Bot::BrainView* brain, const ImVec2& size);
	void ResetSelection();

	vlsui::NeuralGraphView view;
	ImVec2 windowPosition, windowSize, displaySize = ImVec2(0, 0);
	Neuron* selectedNeuron = nullptr;
	// Neurons of the shown brain version (identifies bot and version)
	const NeuronRow* selectedBrain = nullptr;
	const char* selectedNeuronCaption = "";
	const char* selectedNeuronTypeName = "";
	float selectedNeuronValue = 0.0f;
	float selectedNeuronMemory = 0.0f;
};
