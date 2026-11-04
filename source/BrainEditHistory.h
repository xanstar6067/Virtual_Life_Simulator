#pragma once

#include <array>
#include <cstddef>
#include <map>
#include <vector>

namespace vlsui
{
// Each neuron and brain version has its own window-local timeline.
// Values and live memory are not rewound. Empty histories allocate no storage.
template <class Neuron, std::size_t Layers, std::size_t Width>
class BrainEditHistory
{
    struct Edit { Neuron before, after; };
    struct Timeline
    {
        std::vector<Edit> edits;
        std::size_t position = 0;
    };
    std::map<std::size_t, Timeline> histories;
    const void* owner = nullptr;

    static bool Valid(int version, int layer, int index)
    {
        return version >= 0 && version < 2 && layer >= 0 && layer < (int)Layers && index >= 0 && index < (int)Width;
    }

    static std::size_t Key(int version, int layer, int index)
    {
        return (version * Layers + layer) * Width + index;
    }

    const Timeline* Find(int version, int layer, int index) const
    {
        if (!Valid(version, layer, index)) return nullptr;
        const auto it = histories.find(Key(version, layer, index));
        return it == histories.end() ? nullptr : &it->second;
    }

    static bool Same(const Neuron& x, const Neuron& y)
    {
        if (x.type != y.type || x.layer != y.layer || x.bias != y.bias || x.numConnections != y.numConnections)
            return false;
        for (unsigned c = 0; c < x.numConnections; ++c)
        {
            const auto& u = x.allConnections[c];
            const auto& v = y.allConnections[c];
            if (u.dest_layer != v.dest_layer || u.dest_neuron != v.dest_neuron || u.weight != v.weight)
                return false;
        }
        return true;
    }

    void RecordState(int version, int layer, int index, const Neuron& before, const Neuron& after)
    {
        if (!Valid(version, layer, index) || Same(before, after)) return;
        auto& timeline = histories[Key(version, layer, index)];
        timeline.edits.erase(timeline.edits.begin() + timeline.position, timeline.edits.end());
        timeline.edits.push_back({before, after});
        timeline.position = timeline.edits.size();
    }

public:
    void Clear()
    {
        histories.clear();
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

    bool Empty() const { return histories.empty(); }
    bool CanUndo(int version, int layer, int index) const { return Position(version, layer, index) > 0; }
    bool CanRedo(int version, int layer, int index) const { return Position(version, layer, index) < Size(version, layer, index); }
    std::size_t Position(int version, int layer, int index) const
    {
        const auto* timeline = Find(version, layer, index);
        return timeline ? timeline->position : 0;
    }
    std::size_t Size(int version, int layer, int index) const
    {
        const auto* timeline = Find(version, layer, index);
        return timeline ? timeline->edits.size() : 0;
    }

    template <class Operation>
    void Record(int version, int layer, int index, Neuron& neuron, const Operation& operation)
    {
        const Neuron before = neuron;
        operation();
        RecordState(version, layer, index, before, neuron);
    }

    template <class Brain, class Operation>
    void RecordBrain(int version, Brain& brain, const Operation& operation)
    {
        // Whole-brain mutation adds one step to each neuron that actually changes.
        std::array<std::array<Neuron, Width>, Layers> before;
        for (std::size_t layer = 0; layer < Layers; ++layer)
            for (std::size_t index = 0; index < Width; ++index)
                before[layer][index] = brain.allNeurons[layer][index];
        operation();
        for (std::size_t layer = 0; layer < Layers; ++layer)
            for (std::size_t index = 0; index < Width; ++index)
                RecordState(version, (int)layer, (int)index, before[layer][index], brain.allNeurons[layer][index]);
    }

    void Undo(int version, int layer, int index, Neuron& neuron)
    {
        if (!CanUndo(version, layer, index)) return;
        auto& timeline = histories.at(Key(version, layer, index));
        neuron = timeline.edits[--timeline.position].before;
    }

    void Redo(int version, int layer, int index, Neuron& neuron)
    {
        if (!CanRedo(version, layer, index)) return;
        auto& timeline = histories.at(Key(version, layer, index));
        neuron = timeline.edits[timeline.position++].after;
    }
};
} // namespace vlsui
