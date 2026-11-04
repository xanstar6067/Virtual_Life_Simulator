#pragma once

#include <array>
#include <cstddef>
#include <utility>
#include <vector>

namespace vlsui
{
// Window-local edits only: neuron values and the bot's live memory are not rewound.
template <class Neuron, std::size_t Layers, std::size_t Width>
class BrainEditHistory
{
	using Snapshot = std::array<std::array<Neuron, Width>, Layers>;
	struct Edit
	{
		int version;
		Snapshot before, after;
	};
	std::vector<Edit> edits;
	std::size_t position = 0;
	const void* owner = nullptr;

	template <class Brain>
	static Snapshot Capture(const Brain& brain)
	{
		Snapshot result;
		for (std::size_t layer = 0; layer < Layers; ++layer)
			for (std::size_t index = 0; index < Width; ++index)
				result[layer][index] = brain.allNeurons[layer][index];
		return result;
	}

	static bool Same(const Snapshot& a, const Snapshot& b)
	{
		for (std::size_t layer = 0; layer < Layers; ++layer)
			for (std::size_t index = 0; index < Width; ++index)
			{
				const auto& x = a[layer][index];
				const auto& y = b[layer][index];
				if (x.type != y.type || x.layer != y.layer || x.bias != y.bias || x.numConnections != y.numConnections)
					return false;
				for (unsigned c = 0; c < x.numConnections; ++c)
				{
					const auto& u = x.allConnections[c];
					const auto& v = y.allConnections[c];
					if (u.dest_layer != v.dest_layer || u.dest_neuron != v.dest_neuron || u.weight != v.weight)
						return false;
				}
			}
		return true;
	}

	template <class Brain>
	static void Restore(const Snapshot& snapshot, Brain& brain)
	{
		for (std::size_t layer = 0; layer < Layers; ++layer)
			for (std::size_t index = 0; index < Width; ++index)
				brain.allNeurons[layer][index] = snapshot[layer][index];
	}

public:
	void Clear()
	{
		std::vector<Edit>().swap(edits);
		position = 0;
		owner = nullptr;
	}

	void SetOwner(const void* bot)
	{
		if (owner != bot)
		{
			Clear();
			owner = bot;
		}
	}

	bool CanUndo() const { return position > 0; }
	bool CanRedo() const { return position < edits.size(); }
	std::size_t Position() const { return position; }
	std::size_t Size() const { return edits.size(); }

	template <class Brain, class Operation>
	void Record(int version, Brain& brain, const Operation& operation)
	{
		auto before = Capture(brain);
		operation();
		auto after = Capture(brain);
		if (Same(before, after))
			return;
		edits.erase(edits.begin() + position, edits.end());
		edits.push_back({version, std::move(before), std::move(after)});
		position = edits.size();
	}

	template <class Brain>
	void Undo(Brain& current, Brain& initial, int& version)
	{
		if (!CanUndo())
			return;
		const auto& edit = edits[--position];
		version = edit.version;
		Restore(edit.before, version == 0 ? current : initial);
	}

	template <class Brain>
	void Redo(Brain& current, Brain& initial, int& version)
	{
		if (!CanRedo())
			return;
		const auto& edit = edits[position++];
		version = edit.version;
		Restore(edit.after, version == 0 ? current : initial);
	}
};
} // namespace vlsui
