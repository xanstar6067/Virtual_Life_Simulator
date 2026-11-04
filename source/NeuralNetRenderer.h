#pragma once
#include "BotNeuralNet.h"
#include "NeuralGraphView.h"
#include "SDL.h"

struct NeuralNetRenderer final
{
	void Render(BotNeuralNet* brain, const ImVec2& size);
	void ResetSelection();

	vlsui::NeuralGraphView view;
	ImVec2 windowPosition, windowSize, displaySize = ImVec2(0, 0);
	Neuron* selectedNeuron = nullptr;
	BotNeuralNet* selectedBrain = nullptr;
	const char* selectedNeuronCaption = "";
	const char* selectedNeuronTypeName = "";
	float selectedNeuronValue = 0.0f;
	float selectedNeuronMemory = 0.0f;
};
