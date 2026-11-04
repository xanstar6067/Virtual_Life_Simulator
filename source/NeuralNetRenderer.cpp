#include "NeuralNetRenderer.h"

void NeuralNetRenderer::ResetSelection()
{
	view.ClearSelection();
	selectedNeuron = nullptr;
	selectedBrain = nullptr;
	selectedNeuronCaption = selectedNeuronTypeName = "";
	selectedNeuronValue = selectedNeuronMemory = 0.0f;
}

void NeuralNetRenderer::Render(BotNeuralNet* brain, const ImVec2& size)
{
	if (selectedBrain != brain)
		ResetSelection();
	selectedBrain = brain;
	static const char* inputs[] = {"энергия", "зрение", "возраст", "направление", "высота"};
	static const char* outputs[] = {"поворот", "движение", "фотосинтез", "размножение", "атака"};
	std::vector<int> counts;
	std::vector<vlsui::GraphNeuron> neurons;
	std::vector<vlsui::GraphConnection> connections;
	for (int layer = 0; layer < (int)NumNeuronLayers; ++layer)
	{
		const int count = NeuronsInLayer;
		counts.push_back(count);
		for (int index = 0; index < count; ++index)
		{
			const auto& neuron = brain->allNeurons[layer][index];
			const char* caption = layer == 0 ? inputs[index] : layer == (int)NumNeuronLayers - 1 ? outputs[index] : "";
			const char* typeName = Neuron::GetTextFromType(neuron.type);
			neurons.push_back({layer, index, (int)neuron.type, caption, typeName, brain->allValues[layer][index]});
			for (unsigned c = 0; c < neuron.numConnections; ++c)
			{
				const auto& edge = neuron.allConnections[c];
				connections.push_back({layer, index, edge.dest_layer, edge.dest_neuron, edge.weight * 1.0f});
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
