#include "NeuralNetRenderer.h"

namespace cb3
{

void NeuralNetRenderer::ResetSelection()
{
	view.ClearSelection();
	selectedNeuron = nullptr;
	selectedBrain = nullptr;
	selectedNeuronCaption = selectedNeuronTypeName = "";
	selectedNeuronValue = selectedNeuronMemory = 0.0f;
}

void NeuralNetRenderer::Render(const Bot::BrainView* brain, const ImVec2& size)
{
	if (selectedBrain != brain->allNeurons)
		ResetSelection();
	selectedBrain = brain->allNeurons;

	std::vector<int> counts;
	std::vector<vlsui::GraphNeuron> neurons;
	std::vector<vlsui::GraphConnection> connections;
	for (int layer = 0; layer < (int)NumNeuronLayers; ++layer)
	{
		const int count = neuronsInLayer[layer];
		counts.push_back(count);
		for (int index = 0; index < count; ++index)
		{
			const auto& neuron = brain->allNeurons[layer][index];
			const char* caption = layer == 0						  ? inputLayerCaptions[index]
								  : layer == (int)NumNeuronLayers - 1 ? outputLayerCaptions[index]
																	  : "";
			const char* typeName = NeuronTypeNames[neuron.type];
			neurons.push_back({layer, index, (int)neuron.type, caption, typeName, brain->allValues[layer][index]});
			for (unsigned c = 0; c < neuron.numConnections; ++c)
			{
				const auto& edge = neuron.allConnections[c];
				connections.push_back(
					{layer, index, edge.dest_layer, edge.dest_neuron, edge.weight * WeightMultiplier});
			}
		}
	}
	view.Draw(neurons, connections, counts, size);
	selectedNeuron = nullptr;
	selectedNeuronCaption = selectedNeuronTypeName = "";
	for (const auto& node : neurons)
		if (node.layer == view.selectedLayer && node.index == view.selectedIndex)
		{
			selectedNeuron = &brain->allNeurons[node.layer][node.index];
			selectedNeuronCaption = node.caption;
			selectedNeuronTypeName = node.typeName;
			selectedNeuronValue = node.value;
			selectedNeuronMemory = (float)brain->allMemory[node.layer][node.index];
			break;
		}
}
} // namespace cb3
