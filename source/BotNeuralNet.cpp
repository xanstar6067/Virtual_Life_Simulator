
#include "BotNeuralNet.h"



static float ActivationSimple(float value)
{
	return (value >= 0.5f) ? 1.0f : 0.0f;
}

static float PlusMinusActivation(float value)
{
	if (value >= 0.5f)
		return 1.0f;
	else if (value <= -0.5f)
		return -1.0f;
	
	return 0.0f;
}

static float RadialBasisActivation(float value)
{
	return ((value >= 0.45f) && (value <= 0.55f)) ? 1.0f : 0.0f;
}

[[maybe_unused]] static float LinearActivation(float value)
{
	if (value > 1.0f)
		value = 1.0f;
	else if (value < 0.0f)
		value = 0.0f;

	return value;
}


void brain::InitNeurons(NeuronRow* allNeurons)
{
	for (uint yi = 0; yi < NeuronsInLayer; ++yi)
	{
		allNeurons[NeuronOutputLayerIndex][yi].type = output;
		allNeurons[NeuronInputLayerIndex][yi].type = input;
	}

	for (uint nx = 0; nx < NumNeuronLayers; ++nx)
	{
		for (uint ny = 0; ny < NeuronsInLayer; ++ny)
		{
			allNeurons[nx][ny].layer = nx;
		}
	}
}

void brain::Process(const NeuronRow* allNeurons, NeuronValueRow* allValues, NeuronValueRow* allMemory)
{
	float* value;
	float* m;
	const Neuron* n;

	//Parse all neurons, except output layer
	for (uint xi = 0; xi < NumNeuronLayers-1; ++xi)
	{
		for (uint yi = 0; yi < NeuronsInLayer; ++yi)
		{
			value = &allValues[xi][yi];
			m = &allMemory[xi][yi];
			n = &allNeurons[xi][yi];

			//Skip calculation if neuron has no further connections
			if (n->numConnections == 0)
				continue;

			switch (n->type)
			{
			case basic:
				*value = ActivationSimple(*value + n->bias);
				//*value = Linear(*value + n->bias);
				break;

			case random:
				*value = (RandomVal(1000) <= ((int)((*value + n->bias) * 1000.0f))) ? 1.0f : 0.0f;
				break;

			case input:
				*value = *value + n->bias;
				break;

			case radialbasis:
				*value = RadialBasisActivation(*value + n->bias);
				break;

			case memory:

				if ((*value + n->bias) < -1.0f)
				{
					//Wipe memory
					*m = 0.0f;
				}
				else if ((*value + n->bias) > 1.0f)
				{
					//Write in memory
					*m = 1.0f;
				}

				*value = *m;

				break;

			}

			//Feed forward

			const NeuronConnection* tmpC;

			for (uint i = 0; i < n->numConnections; ++i)
			{
				tmpC = &n->allConnections[i];

				allValues[tmpC->dest_layer][tmpC->dest_neuron] += *value * tmpC->weight;
			}
		}
	}


	//Output layer

	//Rotation
	value = &allValues[NeuronOutputLayerIndex][0];
	n = &allNeurons[NeuronOutputLayerIndex][0];

	*value = PlusMinusActivation(*value + n->bias);

	//All the rest
	for (uint oi = 1; oi < NeuronsInLayer; ++oi)
	{
		value = &allValues[NeuronOutputLayerIndex][oi];
		n = &allNeurons[NeuronOutputLayerIndex][oi];

		*value = ActivationSimple(*value + n->bias);
	}
}


void brain::MutateSlightly(NeuronRow* allNeurons)
{
	uint randomNeuronIndex = RandomVal(NumNeuronLayers * NeuronsInLayer);
	uint counter = 0;

	for (uint xi = 0; xi < NumNeuronLayers; ++xi)
	{
		for (uint yi = 0; yi < NeuronsInLayer; ++yi)
		{

			if (counter++ == randomNeuronIndex)
			{
				Neuron* n = &allNeurons[xi][yi];

				n->SlightlyChange();
			}

		}
	}
}


void brain::Mutate(NeuronRow* allNeurons)
{
	uint randomNeuronIndex = RandomVal(NumNeuronLayers * NeuronsInLayer);
	uint counter = 0;
	Neuron* n;
	uint roll;

	for (uint xi = 0; xi < NumNeuronLayers; ++xi)
	{
		for (uint yi = 0; yi < NeuronsInLayer; ++yi)
		{
			if (counter++ == randomNeuronIndex)
			{
				n = &allNeurons[xi][yi];
				roll = RandomVal(101);
								
				if(roll <= 5)
				{
					//5% - set neuron to random
					n->SetRandom();
				}
				else if (roll <= 7)
				{
					//2% - clear neuron
					n->mutate_DeleteNeuron();
				}
				else if (roll <= 12)
				{
					//5% - change neuron type
					n->mutate_ChangeType();
				}
				else if (roll <= 30)
				{
					//18% - change bias
					n->mutate_ChangeBias();
				}
				else if (roll <= 35)
				{
					//5% - 1 connection set to next layer 
					//TODO
					if (n->numConnections > 0)
					{
						uint connection = n->GetRandomConnectionIndex();

						byte lx = n->allConnections[connection].dest_layer;
						byte nx = n->allConnections[connection].dest_neuron;

						Neuron* n2 = &allNeurons[lx][nx];

						if (n2->numConnections > 0)
						{
							uint connection_dest = n2->GetRandomConnectionIndex();

							n->allConnections[connection].dest_layer = n2->allConnections[connection_dest].dest_layer;
							n->allConnections[connection].dest_neuron = n2->allConnections[connection_dest].dest_neuron;
						}
						else
						{
							goto def;
						}
					}
				}
				else
				{
				def:
					//change 1 connection
					n->mutate_ChangeOneConnection();
				}
			}
		}
	}
}


void brain::MutateHarsh(NeuronRow* allNeurons)
{
	for (uint xi = 0; xi < NumNeuronLayers; ++xi)
	{
		for (uint yi = 0; yi < NeuronsInLayer; ++yi)
		{
			if (RandomPercent(50))
				continue;

			Neuron* n = &allNeurons[xi][yi];

			n->SetRandom();
		}
	}
}


void brain::Optimize(NeuronRow* allNeurons)
{
	Neuron* n;
	
	//TODO
	for (int x = NeuronOutputLayerIndex - 1; x > 0; --x)
	{
		for (int y = 0; y < NeuronsInLayer; ++y)
		{
			n = &allNeurons[x][y];

			//If neuron is a dead-end
			if (n->numConnections == 0)
			{
				//Clear bias
				n->SetZero();

				//Clear all connections to this neuron in prev. layers
				for (int cx = 0; cx < x; ++cx)
				{
					for (int cy = 0; cy < NeuronsInLayer; ++cy)
					{
						int ind = allNeurons[cx][cy].IsConnected(x, y);

						if (ind != -1)
						{
							allNeurons[cx][cy].RemoveConnection(ind);
						}
					}
				}
			}
		}
	}
}


void brain::Randomize(NeuronRow* allNeurons)
{
	//Input layer
	for (uint y = 0; y < NeuronsInLayer; ++y)
	{
		allNeurons[NeuronInputLayerIndex][y].SetRandomConnections();
		allNeurons[NeuronInputLayerIndex][y].SetRandomBias();
	}

	//Hidden layers
	for (uint xi = 1; xi < NumNeuronLayers - 1; ++xi)
	{
		for (uint yi = 0; yi < NeuronsInLayer; ++yi)
		{
			allNeurons[xi][yi].SetRandom();
		}
	}

	//Output layer
	for (uint y = 0; y < NeuronsInLayer; ++y)
	{
		allNeurons[NeuronOutputLayerIndex][y].SetRandomBias();
	}
}


BrainOutput brain::GetOutput(const NeuronValueRow* allValues)
{
	BrainOutput toRet;

	repeat(NeuronsInLayer)
		toRet.fields[i] = (int)allValues[NeuronOutputLayerIndex][i];

	return toRet;
}


//Standalone network

void BotNeuralNet::ClearMemory()
{
	memset(allMemory, 0, sizeof(allMemory));
}

BotNeuralNet::BotNeuralNet()
{
	brain::InitNeurons(allNeurons);
	ClearMemory();
}

void BotNeuralNet::Clone(BotNeuralNet* prototype)
{
	ClearMemory();
	memcpy(allNeurons, prototype->allNeurons, sizeof(allNeurons));
}

BotNeuralNet::BotNeuralNet(BotNeuralNet* prototype)
{
	Clone(prototype);
}

void BotNeuralNet::Clear()
{
	memset(allValues, 0, sizeof(allValues));
}

void BotNeuralNet::Process()
{
	brain::Process(allNeurons, allValues, allMemory);
}

void BotNeuralNet::MutateSlightly()
{
	brain::MutateSlightly(allNeurons);
}

void BotNeuralNet::Mutate()
{
	brain::Mutate(allNeurons);
}

void BotNeuralNet::MutateHarsh()
{
	brain::MutateHarsh(allNeurons);
}

void BotNeuralNet::Randomize()
{
	brain::Randomize(allNeurons);
}

void BotNeuralNet::Optimize()
{
	brain::Optimize(allNeurons);
}

BrainOutput BotNeuralNet::GetOutput()
{
	return brain::GetOutput(allValues);
}

void BotNeuralNet::SetDummy()
{
	//Preset brain

	/*
	Example:
	
	allNeurons[1][3].SetTunnel(3);
	...
	allNeurons[4][3].bias = 0.0f;
	allNeurons[4][3].AddConnection(3, 0.5f);
	*/
}

// Fields that define a neuron's behaviour; floats are compared bit for bit.
static bool SameNeuron(const Neuron& a, const Neuron& b)
{
	if (a.type != b.type || a.layer != b.layer || a.numConnections != b.numConnections ||
		memcmp(&a.bias, &b.bias, sizeof(a.bias)) != 0)
		return false;

	for (uint i = 0; i < a.numConnections; ++i)
	{
		const NeuronConnection& ca = a.allConnections[i];
		const NeuronConnection& cb = b.allConnections[i];

		if (ca.dest_layer != cb.dest_layer || ca.dest_neuron != cb.dest_neuron ||
			memcmp(&ca.weight, &cb.weight, sizeof(ca.weight)) != 0)
			return false;
	}

	return true;
}

bool brain::SameNeurons(const NeuronRow* a, const NeuronRow* b)
{
	for (uint x = 0; x < NumNeuronLayers; ++x)
		for (uint y = 0; y < NeuronsInLayer; ++y)
			if (!SameNeuron(a[x][y], b[x][y]))
				return false;

	return true;
}

uint64_t brain::HashNeurons(const NeuronRow* allNeurons, uint64_t hash)
{
	//FNV-1a over the fields compared by SameNeurons
	auto add = [&hash](const void* data, size_t size)
	{
		const unsigned char* bytes = (const unsigned char*)data;
		for (size_t i = 0; i < size; ++i)
			hash = (hash ^ bytes[i]) * 1099511628211ull;
	};

	for (uint x = 0; x < NumNeuronLayers; ++x)
		for (uint y = 0; y < NeuronsInLayer; ++y)
		{
			const Neuron& n = allNeurons[x][y];
			add(&n.type, sizeof(n.type));
			add(&n.layer, sizeof(n.layer));
			add(&n.bias, sizeof(n.bias));
			add(&n.numConnections, sizeof(n.numConnections));

			for (uint i = 0; i < n.numConnections; ++i)
			{
				const NeuronConnection& c = n.allConnections[i];
				add(&c.dest_layer, sizeof(c.dest_layer));
				add(&c.dest_neuron, sizeof(c.dest_neuron));
				add(&c.weight, sizeof(c.weight));
			}
		}

	return hash;
}


//Shared genome

BrainGenome::BrainGenome()
{
	brain::InitNeurons(initial);
	brain::InitNeurons(active);
}

BrainGenome::BrainGenome(const BrainGenome& other) :
	activeIsOptimizedInitial(other.activeIsOptimizedInitial)
{
	memcpy(initial, other.initial, sizeof(initial));
	memcpy(active, other.active, sizeof(active));
}

void BrainGenome::RebuildActive()
{
	memcpy(active, initial, sizeof(active));
	brain::Optimize(active);
	activeIsOptimizedInitial = true;
}

void BrainGenome::RecheckActive()
{
	Neuron optimized[NumNeuronLayers][NeuronsInLayer];
	memcpy(optimized, initial, sizeof(optimized));
	brain::Optimize(optimized);
	activeIsOptimizedInitial = brain::SameNeurons(optimized, active);
}

bool BrainGenome::SameNeurons(const BrainGenome& other) const
{
	return brain::SameNeurons(initial, other.initial) && brain::SameNeurons(active, other.active);
}


void GenomeRef::Acquire()
{
	if (genome)
		genome->refs.fetch_add(1, std::memory_order_relaxed);
}

void GenomeRef::ReleaseRef()
{
	if (genome && genome->refs.fetch_sub(1, std::memory_order_acq_rel) == 1)
		delete genome;
	genome = nullptr;
}

GenomeRef::GenomeRef(BrainGenome* g) : genome(g)
{
	Acquire();
}

GenomeRef::GenomeRef(const GenomeRef& other) : genome(other.genome)
{
	Acquire();
}

GenomeRef::GenomeRef(GenomeRef&& other) noexcept : genome(other.genome)
{
	other.genome = nullptr;
}

GenomeRef& GenomeRef::operator=(const GenomeRef& other)
{
	if (genome != other.genome)
	{
		BrainGenome* g = other.genome;
		if (g)
			g->refs.fetch_add(1, std::memory_order_relaxed);
		ReleaseRef();
		genome = g;
	}
	return *this;
}

GenomeRef& GenomeRef::operator=(GenomeRef&& other) noexcept
{
	if (this != &other)
	{
		ReleaseRef();
		genome = other.genome;
		other.genome = nullptr;
	}
	return *this;
}

GenomeRef::~GenomeRef()
{
	ReleaseRef();
}

GenomeRef GenomeRef::Create()
{
	return GenomeRef(new BrainGenome());
}

BrainGenome* GenomeRef::MakeUnique()
{
	// refs == 1 means this reference is the only owner: nobody else can share
	// the genome concurrently, so it may be changed in place.
	if (genome && genome->refs.load(std::memory_order_acquire) != 1)
		*this = GenomeRef(new BrainGenome(*genome));
	else if (!genome)
		*this = Create();
	return genome;
}


BrainOutput BrainOutput::GetEmptyBrain()
{
	return { 0, 0, 0, 0, 0 };
};
