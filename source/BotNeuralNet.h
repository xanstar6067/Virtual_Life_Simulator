#pragma once


#include "Neuron.h"

#include <atomic>
#include <cstdint>


struct BrainInput
{
	float
	energy = 0.0f,
	vision = 0.0f,
	age = 0.0f,
	rotation = 0.0f,
	height = 0.0f;
};



union BrainOutput
{
	struct
	{
		int desired_rotation;
		int move;
		int photosynthesis;
		int divide;
		int attack;
	};

	int fields[NeuronsInLayer];

	static BrainOutput GetEmptyBrain();
};




// Neuron layout of one network: allNeurons[layer][index].
using NeuronRow = Neuron[NeuronsInLayer];
using NeuronValueRow = float[NeuronsInLayer];

// Network algorithms shared by the standalone BotNeuralNet and by bots, whose
// neurons live in a shared BrainGenome while memory stays per bot.
namespace brain
{
	// Default layout: input/output neuron types and layer numbers.
	void InitNeurons(NeuronRow* allNeurons);
	// allValues must hold the input layer values, other layers zero.
	void Process(const NeuronRow* allNeurons, NeuronValueRow* allValues, NeuronValueRow* allMemory);
	BrainOutput GetOutput(const NeuronValueRow* allValues);
	void MutateSlightly(NeuronRow* allNeurons);
	void Mutate(NeuronRow* allNeurons);
	void MutateHarsh(NeuronRow* allNeurons);
	void Randomize(NeuronRow* allNeurons);
	void Optimize(NeuronRow* allNeurons);
	// Equal networks: compares only fields the network uses (not padding or
	// connection slots past numConnections, which may hold stale bytes).
	bool SameNeurons(const NeuronRow* a, const NeuronRow* b);
	uint64_t HashNeurons(const NeuronRow* allNeurons, uint64_t hash);
}


// Standalone network with its own neurons, values and memory (tests, tools).
// Bots do not embed it: see BrainGenome.
class BotNeuralNet
{
	void ClearMemory();

public:

	Neuron allNeurons[NumNeuronLayers][NeuronsInLayer];

	//Neuron values and memory
	float allValues[NumNeuronLayers][NeuronsInLayer];
	float allMemory[NumNeuronLayers][NeuronsInLayer];

	void Clone(BotNeuralNet* prototype);

	BotNeuralNet();
	BotNeuralNet(BotNeuralNet* prototype);

	//Clear all values
	void Clear();

	void Process();

	/*Very little change, no new connections,
	  experimental*/
	void MutateSlightly();

	void Mutate();
	void MutateHarsh();

	void Randomize();

	void Optimize();

	BrainOutput GetOutput();

	void SetDummy();

};


// Heritable part of a bot brain, shared by relatives (copy-on-write).
// initial - genes received from the parent; active - the network used by the
// bot, initial after Optimize() unless changed later. While a genome is shared
// (refs > 1) it must not be changed: GenomeRef::MakeUnique() gives the owner
// its own copy. The counter is atomic: bots are born and die in worker threads.
class BrainGenome final
{
public:
	Neuron initial[NumNeuronLayers][NeuronsInLayer];
	Neuron active[NumNeuronLayers][NeuronsInLayer];

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
	// Same neurons (both versions) - used to merge genomes on load.
	bool SameNeurons(const BrainGenome& other) const;

	uint32_t RefCount() const { return refs.load(std::memory_order_acquire); }

private:
	friend class GenomeRef;
	mutable std::atomic<uint32_t> refs{ 0 };
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
