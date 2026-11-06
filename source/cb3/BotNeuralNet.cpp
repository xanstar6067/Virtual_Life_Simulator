#include "BotNeuralNet.h"
#include <array>



namespace cb3
{

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
	
	return 0;
}

static float RadialBasisActivation(float value)
{
	return ((value >=0.45f) and (value <= 0.55f)) ? 1.0f : 0.0f;
}

[[maybe_unused]] static float LinearActivation(float value)
{
	if (value > 1.0f)
		value = 1.0f;
	else if (value < 0)
		value = 0.0f;

	return value;
}

void brain::InitNeurons(NeuronRow* allNeurons)
{
	repeat(NumInputNeurons)
		allNeurons[NeuronInputLayerIndex][i].type = input;

	repeat(NumOutputNeurons)
		allNeurons[NeuronOutputLayerIndex][i].type = output;

	for (uint nx = 0; nx < NumNeuronLayers; ++nx)
	{
		for (uint ny = 0; ny < neuronsInLayer[nx]; ++ny)
		{
			allNeurons[nx][ny].layer = nx;
		}
	}
}

void brain::Process(const NeuronRow* allNeurons, NeuronValueRow* allValues, NeuronMemoryRow* allMemory, const BrainInput& input)
{
	//Clear all neuron values
	memset(allValues, 0, sizeof(float) * NumNeuronLayers * NumNeuronsInLayerMax);

	//Copy input data
	repeat(NumInputNeurons)
	{
		allValues[NeuronInputLayerIndex][i] = input.fields[i];
	}

	int8_t* m;
	const Neuron* n;
	float tmpVal;

	//Parse input layer
	for (uint yi = 0; yi < NumInputNeurons; ++yi)
	{
		n = &allNeurons[NeuronInputLayerIndex][yi];

		tmpVal = allValues[NeuronInputLayerIndex][yi] + (n->bias * BiasMultiplier);

		const NeuronConnection* tmpC;

		for (uint i = 0; i < n->numConnections; ++i)
		{
			tmpC = &n->allConnections[i];

			allValues[tmpC->dest_layer][tmpC->dest_neuron] += tmpVal * (tmpC->weight * WeightMultiplier);
		}
	}

	//Parse hidden layers
	for (uint xi = 1; xi < NumNeuronLayers-1; ++xi)
	{
		for (uint yi = 0; yi < NumHiddenNeurons; ++yi)
		{
			m = &allMemory[xi][yi];
			n = &allNeurons[xi][yi];
			tmpVal = allValues[xi][yi] + (n->bias * BiasMultiplier);

			//Skip calculation if neuron has no further connections
			if (n->numConnections == 0)
				continue;

			switch (n->type)
			{
			case basic:
				tmpVal = ActivationSimple(tmpVal);
				//tmpVal = Linear(tmpVal);
				break;

			case random:
				tmpVal = (RandomVal(1000) <= ((int)(tmpVal * 1000.0f))) ? 1.0f : 0.0f;
				break;

			case radialbasis:
				tmpVal = RadialBasisActivation(tmpVal);
				break;

			case memory:

				if (tmpVal < -1.0f)
				{
					//Wipe memory
					*m = 0;
				}
				else if (tmpVal > 1.0f)
				{
					//Write in memory
					*m = 1;
				}

				tmpVal = *m * 1.0f;

				break;

			}

			//Feed forward
			const NeuronConnection* tmpC;

			for (uint i = 0; i < n->numConnections; ++i)
			{
				tmpC = &n->allConnections[i];

				allValues[tmpC->dest_layer][tmpC->dest_neuron] += tmpVal * (tmpC->weight * WeightMultiplier);
			}
		}
	}


	//Output layer

	//Rotation
	repeat(2)
	{
		n = &allNeurons[NeuronOutputLayerIndex][i];
		tmpVal = allValues[NeuronOutputLayerIndex][i] + (n->bias * BiasMultiplier);

		allValues[NeuronOutputLayerIndex][i] = PlusMinusActivation(tmpVal);
	}

	//All the rest
	for (uint oi = 2; oi < NumOutputNeurons; ++oi)
	{
		n = &allNeurons[NeuronOutputLayerIndex][oi];
		tmpVal = allValues[NeuronOutputLayerIndex][oi] + (n->bias * BiasMultiplier);

		allValues[NeuronOutputLayerIndex][oi] = ActivationSimple(tmpVal);
	}
}



// Shared lookup tables keep the packed plan at two bytes per connection while
// preparing the signed-byte scaling once, with the same float rounding as Process.
static constexpr auto ScaledBias = [] {
	std::array<float, 256> table{};
	for (int i = 0; i < 256; ++i) table[i] = (i < 128 ? i : i - 256) * BiasMultiplier;
	return table;
}();
static constexpr auto ScaledWeight = [] {
	std::array<float, 256> table{};
	for (int i = 0; i < 256; ++i) table[i] = (i < 128 ? i : i - 256) * WeightMultiplier;
	return table;
}();

brain::ExecutionPlan::ExecutionPlan(const ExecutionPlan& other) :
	capacity(other.ready ? other.usedBytes : 0), usedBytes(capacity), inputCount(other.inputCount),
	hiddenCount(other.hiddenCount), ready(other.ready)
{
	memcpy(outputBias, other.outputBias, sizeof(outputBias));
	if (capacity)
	{
		code.reset(new byte[capacity]);
		memcpy(code.get(), other.code.get(), capacity);
	}
}

void brain::ExecutionPlan::Build(const NeuronRow* neurons)
{
	static_assert(NumNeuronLayers * NumNeuronsInLayerMax <= 256);
	ready = false;
	inputCount = hiddenCount = 0;
	size_t bytes = 0;
	for (uint layer = 0; layer < NeuronOutputLayerIndex; ++layer)
		for (uint index = 0; index < (layer == 0 ? NumInputNeurons : NumHiddenNeurons); ++index)
			if (neurons[layer][index].numConnections)
				bytes += (layer == 0 ? 3 : 4) + 2 * neurons[layer][index].numConnections;
	if (bytes > capacity)
	{
		code.reset(new byte[bytes]);
		capacity = static_cast<uint16_t>(bytes);
	}
	usedBytes = static_cast<uint16_t>(bytes);
	byte* next = code.get();
	// Input records: slot/count/bias. Hidden records: slot/type/count/bias.
	// Each record is followed by count target/weight pairs. Slots use the full
	// rectangular value/memory layout, including unused positions for the viewer.
	for (uint layer = 0; layer < NeuronOutputLayerIndex; ++layer)
		for (uint index = 0; index < (layer == 0 ? NumInputNeurons : NumHiddenNeurons); ++index)
		{
			const Neuron& n = neurons[layer][index];
			if (!n.numConnections) continue;
			if (layer == 0) ++inputCount; else ++hiddenCount;
			*next++ = static_cast<byte>(layer * NumNeuronsInLayerMax + index);
			// Input neurons always pass through, regardless of their stored type.
			if (layer != 0) *next++ = static_cast<byte>(n.type);
			*next++ = n.numConnections;
			*next++ = static_cast<byte>(n.bias);
			for (uint edge = 0; edge < n.numConnections; ++edge)
			{
				const auto& c = n.allConnections[edge];
				*next++ = static_cast<byte>(c.dest_layer * NumNeuronsInLayerMax + c.dest_neuron);
				*next++ = static_cast<byte>(c.weight);
			}
		}
	for (uint i = 0; i < NumOutputNeurons; ++i)
		outputBias[i] = static_cast<byte>(neurons[NeuronOutputLayerIndex][i].bias);
	ready = true;
}

static float& PlanValue(NeuronValueRow* values, byte index)
{
	return values[index / NumNeuronsInLayerMax][index % NumNeuronsInLayerMax];
}

static const byte* PlanFeed(NeuronValueRow* values, const byte* next, uint count, float value)
{
	// Plans contain only nonempty nodes, so the first edge always exists.
	do
	{
		const byte dest = *next++;
		const float weight = ScaledWeight[*next++];
		PlanValue(values, dest) += value * weight;
	} while (--count);
	return next;
}

void brain::ExecutionPlan::Process(NeuronValueRow* values, NeuronMemoryRow* memory, const BrainInput& input) const
{
	memset(values, 0, sizeof(float) * NumNeuronLayers * NumNeuronsInLayerMax);
	memcpy(values[NeuronInputLayerIndex], input.fields, sizeof(input.fields));
	const byte* next = code.get();
	for (uint i = 0; i < inputCount; ++i)
	{
		const byte index = *next++;
		const byte count = *next++;
		const float value = PlanValue(values, index) + ScaledBias[*next++];
		next = PlanFeed(values, next, count, value);
	}
	for (uint i = 0; i < hiddenCount; ++i)
	{
		const byte index = *next++;
		const byte type = *next++;
		const byte count = *next++;
		float value = PlanValue(values, index) + ScaledBias[*next++];
		switch (type)
		{
		case basic: value = ActivationSimple(value); break;
		case random: value = (RandomVal(1000) <= static_cast<int>(value * 1000.0f)) ? 1.0f : 0.0f; break;
		case radialbasis: value = RadialBasisActivation(value); break;
		case NeuronType::memory:
		{
			auto& state = memory[index / NumNeuronsInLayerMax][index % NumNeuronsInLayerMax];
			if (value < -1.0f) state = 0;
			else if (value > 1.0f) state = 1;
			value = state * 1.0f;
			break;
		}
		}
		next = PlanFeed(values, next, count, value);
	}
	for (uint i = 0; i < 2; ++i)
	{
		const float value = values[NeuronOutputLayerIndex][i] + ScaledBias[outputBias[i]];
		values[NeuronOutputLayerIndex][i] = PlusMinusActivation(value);
	}
	for (uint i = 2; i < NumOutputNeurons; ++i)
	{
		const float value = values[NeuronOutputLayerIndex][i] + ScaledBias[outputBias[i]];
		values[NeuronOutputLayerIndex][i] = ActivationSimple(value);
	}
}

void brain::Mutate(NeuronRow* allNeurons)
{
	uint randomNeuronIndex = RandomVal(NumNeuronLayers * NumNeuronsInLayerMax);
	uint counter = 0;
	Neuron* n;
	uint roll;

	for (uint xi = 0; xi < NumNeuronLayers; ++xi)
	{
		for (uint yi = 0; yi < neuronsInLayer[xi]; ++yi)
		{
			if (counter++ == randomNeuronIndex)
			{
				n = &allNeurons[xi][yi];
				roll = RandomVal(101);
								
				if(roll <= 10)
				{
					//10% - set neuron to random
					n->SetRandom();
				}
				else if (roll <= 12)
				{
					//2% - clear neuron
					n->mutate_DeleteNeuron();
				}
				else if (roll <= 22)
				{
					//10% - change neuron type
					n->mutate_ChangeType();
				}
				else if (roll <= 40)
				{
					//18% - change bias
					n->mutate_ChangeBias();
				}
				else if (roll <= 50)
				{
					//10% - 1 connection jump to next layer 
					//TODO
					if (n->numConnections > 0)
					{
						NeuronConnection& connection = n->allConnections[n->GetRandomConnectionIndex()];

						byte lx = connection.dest_layer;
						byte nx = connection.dest_neuron;

						Neuron* n2 = &allNeurons[lx][nx];

						if (n2->numConnections > 0)
						{
							NeuronConnection& connection_dest = n2->allConnections[n2->GetRandomConnectionIndex()];

							connection.dest_layer = connection_dest.dest_layer;
							connection.dest_neuron = connection_dest.dest_neuron;
						}
						else
						{
							goto def;
						}
					}
					else
					{
						goto def;
					}
				}
				else
				{
				def:
					//change 1 connection
					if (n->numConnections == 0)
						return;

					uint connection = n->GetRandomConnectionIndex();
					NeuronConnection* c = &n->allConnections[connection];
					byte d_layer = c->dest_layer;

					if ((roll <= 85) and ((d_layer - xi) > 1))
					{
						//Split connection	
						for (int i = xi + 1; i < d_layer; ++i)
						{
							//Find unused neuron
							for (int b = 0; b < NumHiddenNeurons; ++b)
							{
								Neuron& n2 = allNeurons[i][b];

								if (n2.IsInactive())
								{
									//Copy neuron connection
									n2.bias = 0;
									n2.AddConnection(c);

									//Retarget old connection
									n->RemoveConnection(connection);

									c->weight = WeightValueCorrespondingTo_1;
									c->dest_layer = i;
									c->dest_neuron = b;

									//We are done
									return;
								}
							}
						}
					}
					else
					{
						//Other mutations
						n->mutate_ChangeConnection(connection);
					}
				}
			}
		}
	}
}


void brain::Optimize(NeuronRow* allNeurons)
{
	Neuron* n;
	
	for (uint x = NeuronOutputLayerIndex - 1; x > 0; --x)
	{
		for (uint y = 0; y < neuronsInLayer[x]; ++y)
		{
			n = &allNeurons[x][y];

			//If neuron is a dead-end
			if (n->numConnections == 0)
			{
				//Clear bias
				n->SetZero();

				//Clear all connections to this neuron in prev. layers
				for (uint cx = 0; cx < x; ++cx)
				{
					for (uint cy = 0; cy < NumNeuronsInLayerMax; ++cy)
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
	for (uint y = 0; y < NumInputNeurons; ++y)
	{
		allNeurons[NeuronInputLayerIndex][y].SetRandomConnections();
		allNeurons[NeuronInputLayerIndex][y].SetRandomBias();
	}

	//Hidden layers
	for (uint xi = 1; xi < NumNeuronLayers - 1; ++xi)
	{
		for (uint yi = 0; yi < NumHiddenNeurons; ++yi)
		{
			allNeurons[xi][yi].SetRandom();
		}
	}

	//Output layer
	for (uint y = 0; y < NumOutputNeurons; ++y)
	{
		allNeurons[NeuronOutputLayerIndex][y].SetRandomBias();
	}
}


BrainOutput brain::GetOutput(const NeuronValueRow* allValues)
{
	BrainOutput toRet;

	repeat(NumNeuronsInLayerMax)
		toRet.fields[i] = static_cast<int>(allValues[NeuronOutputLayerIndex][i]);

	return toRet;
}


//Standalone network

BotNeuralNet::BotNeuralNet()
{
	brain::InitNeurons(allNeurons);
	memset(allMemory, 0, sizeof(allMemory));
}

void BotNeuralNet::Clone(BotNeuralNet* prototype)
{
	memset(allMemory, 0, sizeof(allMemory));
	memcpy(allNeurons, prototype->allNeurons, sizeof(allNeurons));
}

BotNeuralNet::BotNeuralNet(BotNeuralNet* prototype)
{
	Clone(prototype);
}

void BotNeuralNet::Process(BrainInput input)
{
	brain::Process(allNeurons, allValues, allMemory, input);
}

void BotNeuralNet::Mutate()
{
	brain::Mutate(allNeurons);
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


// Fields that define a neuron's behaviour. Rows shorter than the widest layer
// have unused tail neurons: their layer number is not meaningful.
static bool SameNeuron(const Neuron& a, const Neuron& b, bool used)
{
	if (a.type != b.type || a.bias != b.bias || a.numConnections != b.numConnections || (used && a.layer != b.layer))
		return false;

	for (uint i = 0; i < a.numConnections; ++i)
	{
		const NeuronConnection& ca = a.allConnections[i];
		const NeuronConnection& cb = b.allConnections[i];

		if (ca.dest_layer != cb.dest_layer || ca.dest_neuron != cb.dest_neuron || ca.weight != cb.weight)
			return false;
	}

	return true;
}

bool brain::SameNeurons(const NeuronRow* a, const NeuronRow* b)
{
	for (uint x = 0; x < NumNeuronLayers; ++x)
		for (uint y = 0; y < NumNeuronsInLayerMax; ++y)
			if (!SameNeuron(a[x][y], b[x][y], y < neuronsInLayer[x]))
				return false;

	return true;
}

uint64_t brain::HashNeurons(const NeuronRow* allNeurons, uint64_t hash)
{
	//FNV-1a over the fields compared by SameNeurons
	auto add = [&hash](uint value)
	{
		hash = (hash ^ (value & 0xFFu)) * 1099511628211ull;
	};

	for (uint x = 0; x < NumNeuronLayers; ++x)
		for (uint y = 0; y < neuronsInLayer[x]; ++y)
		{
			const Neuron& n = allNeurons[x][y];
			add((uint)n.type);
			add((uint)(uint8_t)n.bias);
			add((uint)n.numConnections);

			for (uint i = 0; i < n.numConnections; ++i)
			{
				const NeuronConnection& c = n.allConnections[i];
				add((uint)c.dest_layer);
				add((uint)c.dest_neuron);
				add((uint)(uint8_t)c.weight);
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
	activeIsOptimizedInitial(other.activeIsOptimizedInitial), executionPlan(other.executionPlan)
{
	memcpy(initial, other.initial, sizeof(initial));
	memcpy(active, other.active, sizeof(active));
}

void BrainGenome::RebuildActive()
{
	memcpy(active, initial, sizeof(active));
	brain::Optimize(active);
	activeIsOptimizedInitial = true;
	executionPlan.Build(active);
}

void BrainGenome::RecheckActive()
{
	Neuron optimized[NumNeuronLayers][NumNeuronsInLayerMax];
	memcpy(optimized, initial, sizeof(optimized));
	brain::Optimize(optimized);
	activeIsOptimizedInitial = brain::SameNeurons(optimized, active);
	executionPlan.Build(active);
}

void BrainGenome::Process(NeuronValueRow* values, NeuronMemoryRow* memory, const BrainInput& input) const
{
	if (executionPlan.IsReady()) executionPlan.Process(values, memory, input);
	else brain::Process(active, values, memory, input);
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


[[maybe_unused]] static void SetDummy()
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


}
