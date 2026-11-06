#pragma once

#include "Neuron.h"
#include <memory>


namespace cb3
{

union BrainInput
{
	struct
	{
		float
		energy,
		age,
		rotation,
		height,	//Y coordinate
		area,	//is in ocean/mud?
		eye1,	//Sight
		eye1_isrelative,
		eye_islookingatme;
		//eye2;	//Second sight cell
	};

	float fields[NumInputNeurons] = {};
};



union BrainOutput
{
	struct
	{
		int desired_rotation_x;
		int desired_rotation_y;

		int move;
		int photosynthesis;
		int attack;
		int digestOrganics;

		int divide_num;
		int divide_energy;		
	};

	int fields[NumOutputNeurons];
};



// Neuron layout of one network: allNeurons[layer][index].
using NeuronRow = Neuron[NumNeuronsInLayerMax];
using NeuronValueRow = float[NumNeuronsInLayerMax];
using NeuronMemoryRow = int8_t[NumNeuronsInLayerMax];

// Network algorithms shared by the standalone BotNeuralNet and by bots, whose
// neurons live in a shared BrainGenome while memory stays per bot.
namespace brain
{
	// Default layout: input/output neuron types and layer numbers.
	void InitNeurons(NeuronRow* allNeurons);
	// values are scratch and fully overwritten; memory is the bot's own state.
	void Process(const NeuronRow* allNeurons, NeuronValueRow* allValues, NeuronMemoryRow* allMemory, const BrainInput& input);
	BrainOutput GetOutput(const NeuronValueRow* allValues);
	void Mutate(NeuronRow* allNeurons);
	void Randomize(NeuronRow* allNeurons);
	void Optimize(NeuronRow* allNeurons);
	// Equal networks: compares only fields the network uses (not padding or
	// connection slots past numConnections, which may hold stale bytes).
	bool SameNeurons(const NeuronRow* a, const NeuronRow* b);
	uint64_t HashNeurons(const NeuronRow* allNeurons, uint64_t hash);

	// Derived, non-serialized execution data. Nodes with no outgoing edges are
	// omitted; the order of all remaining nodes and edges is unchanged.
	class ExecutionPlan final
	{
		std::unique_ptr<byte[]> code;
		uint16_t capacity = 0;
		uint16_t usedBytes = 0;
		byte outputBias[NumOutputNeurons]{};
		byte inputCount = 0, hiddenCount = 0;
		bool ready = false;
	public:
		ExecutionPlan() = default;
		ExecutionPlan(const ExecutionPlan& other);
		ExecutionPlan& operator=(const ExecutionPlan&) = delete;
		void Build(const NeuronRow* neurons);
		void Invalidate() { ready = false; }
		bool IsReady() const { return ready; }
		size_t StorageBytes() const { return capacity; }
		void Process(NeuronValueRow* values, NeuronMemoryRow* memory, const BrainInput& input) const;
	};
}


// Standalone network with its own neurons, values and memory (tests, tools).
// Bots do not embed it: see BrainGenome.
class BotNeuralNet
{
public:

	Neuron allNeurons[NumNeuronLayers][NumNeuronsInLayerMax];

	//Neuron values and memory
	float allValues[NumNeuronLayers][NumNeuronsInLayerMax];
	int8_t allMemory[NumNeuronLayers][NumNeuronsInLayerMax];

	void Clone(BotNeuralNet* prototype);

	BotNeuralNet();
	BotNeuralNet(BotNeuralNet* prototype);

	void Process(BrainInput input);

	void Mutate();
	void Randomize();
	void Optimize();
	
	BrainOutput GetOutput();
};


// Heritable part of a bot brain, shared by relatives (copy-on-write).
// initial - genes received from the parent; active - initial after Optimize(),
// the network actually used by the bot. While a genome is shared (refs > 1)
// it must not be changed: GenomeRef::MakeUnique() gives the owner its own copy.
// The reference counter is atomic: bots are born and die in worker threads.
class BrainGenome final
{
public:
	Neuron initial[NumNeuronLayers][NumNeuronsInLayerMax];
	Neuron active[NumNeuronLayers][NumNeuronsInLayerMax];

	// true when active is exactly Optimize(initial). Only then a child without
	// mutation may share this genome instead of building its own.
	bool activeIsOptimizedInitial = false;

	BrainGenome();
	BrainGenome(const BrainGenome& other);
	BrainGenome& operator=(const BrainGenome&) = delete;

	// active = Optimize(initial)
	void RebuildActive();
	// Recompute activeIsOptimizedInitial after neurons were edited directly.
	void RecheckActive();
	// Editable BrainViews invalidate the derived plan until RecheckActive.
	void InvalidateActivePlan() { executionPlan.Invalidate(); }
	void Process(NeuronValueRow* values, NeuronMemoryRow* memory, const BrainInput& input) const;
	size_t ExecutionStorageBytes() const { return executionPlan.StorageBytes(); }
	// Same neurons (both versions) - used to merge genomes on load.
	bool SameNeurons(const BrainGenome& other) const;

	uint32_t RefCount() const { return refs.load(std::memory_order_acquire); }

private:
	friend class GenomeRef;
	mutable std::atomic<uint32_t> refs{ 0 };
	brain::ExecutionPlan executionPlan;
};


// Intrusive reference to a BrainGenome.
class GenomeRef final
{
	BrainGenome* genome = nullptr;

	void Acquire();
	void ReleaseRef();

public:
	GenomeRef() = default;
	explicit GenomeRef(BrainGenome* g);
	GenomeRef(const GenomeRef& other);
	GenomeRef(GenomeRef&& other) noexcept;
	GenomeRef& operator=(const GenomeRef& other);
	GenomeRef& operator=(GenomeRef&& other) noexcept;
	~GenomeRef();

	static GenomeRef Create();

	const BrainGenome* operator->() const { return genome; }
	const BrainGenome& operator*() const { return *genome; }
	const BrainGenome* get() const { return genome; }
	explicit operator bool() const { return genome != nullptr; }

	// Copy-on-write: returns a genome owned only by this reference.
	BrainGenome* MakeUnique();
};


}
