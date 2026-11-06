#include "../source/cb3/BotNeuralNet.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>

using namespace cb3;
#undef main

static size_t comparisons = 0;
static void Require(bool condition, const char* message)
{
    if (!condition) { std::fprintf(stderr, "FAIL: %s at comparison %zu\n", message, comparisons); std::exit(1); }
}
struct Step
{
    NeuronValueRow values[NumNeuronLayers];
    NeuronMemoryRow memory[NumNeuronLayers];
    BrainOutput output;
    int nextRandom;
};
static void Compare(const NeuronRow* neurons, const std::vector<BrainInput>& inputs, unsigned seed)
{
    brain::ExecutionPlan plan;
    plan.Build(neurons);
    Require(plan.IsReady(), "plan is ready");
    NeuronMemoryRow initial[NumNeuronLayers];
    std::mt19937 rng(seed);
    for (auto& layer : initial) for (auto& m : layer) m = static_cast<int8_t>(rng());
    NeuronMemoryRow memory[NumNeuronLayers];
    memcpy(memory, initial, sizeof(memory));
    std::vector<Step> expected(inputs.size());
    srand(seed);
    for (size_t i = 0; i < inputs.size(); ++i)
    {
        brain::Process(neurons, expected[i].values, memory, inputs[i]);
        memcpy(expected[i].memory, memory, sizeof(memory));
        expected[i].output = brain::GetOutput(expected[i].values);
        expected[i].nextRandom = rand();
    }
    memcpy(memory, initial, sizeof(memory));
    srand(seed);
    for (size_t i = 0; i < inputs.size(); ++i)
    {
        NeuronValueRow values[NumNeuronLayers];
        memset(values, 0xCD, sizeof(values));
        plan.Process(values, memory, inputs[i]);
        const auto output = brain::GetOutput(values);
        const int nextRandom = rand();
        ++comparisons;
        Require(memcmp(values, expected[i].values, sizeof(values)) == 0, "all neuron values match bit for bit");
        Require(memcmp(memory, expected[i].memory, sizeof(memory)) == 0, "memory, including unused slots, matches");
        Require(memcmp(&output, &expected[i].output, sizeof(output)) == 0, "all outputs match");
        Require(nextRandom == expected[i].nextRandom, "random stream matches after each step");
    }
}
static void CompareGenome(BrainGenome& g, const BrainInput& input, unsigned seed)
{
    NeuronValueRow expected[NumNeuronLayers], actual[NumNeuronLayers];
    NeuronMemoryRow expectedMemory[NumNeuronLayers]{}, actualMemory[NumNeuronLayers]{};
    srand(seed);
    brain::Process(g.active, expected, expectedMemory, input);
    const int expectedRandom = rand();
    srand(seed);
    g.Process(actual, actualMemory, input);
    const int actualRandom = rand();
    ++comparisons;
    Require(!memcmp(actual, expected, sizeof(actual)), "genome plan/fallback values");
    Require(!memcmp(actualMemory, expectedMemory, sizeof(actualMemory)), "genome plan/fallback memory");
    Require(actualRandom == expectedRandom, "genome plan/fallback random stream");
}
static void CheckMutationSharing()
{
    size_t unchanged = 0, changed = 0;
    for (unsigned network = 0; network < 32; ++network)
    {
        auto parent = GenomeRef::Create();
        auto* own = parent.MakeUnique();
        srand(800 + network);
        if (network % 2) brain::Randomize(own->initial);
        own->RebuildActive();
        if (network % 4 >= 2)
        {
            own->active[NeuronOutputLayerIndex][0].bias = 77;
            own->RecheckActive();
            Require(!own->activeIsOptimizedInitial, "fixture has an edited active brain");
        }
        const BrainGenome original(*parent);
        for (unsigned seed = 0; seed < 256; ++seed)
        {
            // The old implementation always detached and dirtied the active brain.
            GenomeRef expected(new BrainGenome(*parent));
            auto* old = expected.MakeUnique();
            srand(seed);
            for (uint i = 0; i < (1 + RandomVal(MutateNeuronsMaximum + 1)); ++i)
                brain::Mutate(old->initial);
            old->activeIsOptimizedInitial = false;
            const int nextRandom = rand();
            const bool same = brain::SameNeurons(old->initial, parent->initial);

            GenomeRef actual = parent;
            srand(seed);
            actual.MutateInitial();
            ++comparisons;
            Require(rand() == nextRandom, "mutation batch preserves every random draw");
            Require(brain::SameNeurons(old->initial, actual->initial), "mutated initial brain matches legacy");
            Require((actual.get() == parent.get()) == same, "only an effective mutation detaches a shared genome");
            Require(parent->SameNeurons(original), "mutation leaves relatives unchanged");
            Require(actual->activeIsOptimizedInitial == (same && parent->activeIsOptimizedInitial),
                "no-op preserves the existing active-brain flag");
            if (same) ++unchanged; else ++changed;
            old->RebuildActive();
            if (!actual->activeIsOptimizedInitial) actual.MakeUnique()->RebuildActive();
            Require(actual->SameNeurons(*expected), "rebuilt active brain matches even after an edited parent");

            // A sole owner also avoids needless rebuilding, without allocating a new genome.
            GenomeRef unique(new BrainGenome(*parent));
            const auto* before = unique.get();
            srand(seed);
            unique.MutateInitial();
            Require(rand() == nextRandom && unique.get() == before, "sole-owner mutation preserves RNG and allocation");
            if (!unique->activeIsOptimizedInitial) unique.MakeUnique()->RebuildActive();
            Require(unique->SameNeurons(*expected), "sole-owner mutation matches legacy");
        }
    }
    Require(unchanged > 0 && changed > 0, "effective and ineffective mutations are both covered");
    std::printf("Mutation batches: %zu unchanged, %zu changed; shared/unique and edited active brains match\n", unchanged, changed);
}

int main()
{
    std::mt19937 rng(61793);
    std::vector<BrainInput> inputs(128);
    for (auto& input : inputs)
        for (auto& f : input.fields) f = static_cast<int>(rng() % 12001) / 1000.0f - 6.0f;
    for (unsigned trial = 0; trial < 1024; ++trial)
    {
        srand(trial + 71);
        BotNeuralNet network;
        network.Randomize();
        for (unsigned m = 0; m < trial % 13; ++m) network.Mutate();
        if (trial % 2) network.Optimize();
        Compare(network.allNeurons, inputs, trial + 907);
    }

    BotNeuralNet special;
    Compare(special.allNeurons, inputs, 1); // Empty plan, scratch must still be cleared.
    // Multiple random nodes, zero weights, max connection slots, and unusual
    // hidden input/output types: preserve stored execution and RNG order exactly.
    for (uint layer = 0; layer < NeuronOutputLayerIndex; ++layer)
        for (uint index = 0; index < neuronsInLayer[layer]; ++index)
        {
            auto& n = special.allNeurons[layer][index];
            n.type = static_cast<NeuronType>((index + layer) % 6);
            n.bias = static_cast<int8_t>((index * 43 + layer * 29) % 256);
            for (uint edge = 0; edge < NumNeuronsInLayerMax; ++edge)
                n.AddConnection(NeuronOutputLayerIndex, edge, static_cast<int8_t>(edge * 23 - 80));
            n.allConnections[0].weight = 0;
        }
    Compare(special.allNeurons, inputs, 1927);
    brain::ExecutionPlan largePlan;
    largePlan.Build(special.allNeurons);
    Require(largePlan.StorageBytes() > 0, "full network allocates code");
    BotNeuralNet empty;
    largePlan.Build(empty.allNeurons);
    brain::ExecutionPlan emptyCopy(largePlan);
    Require(emptyCopy.IsReady() && emptyCopy.StorageBytes() == 0,
        "genome copies retain used instructions, not an ancestor's allocation reserve");
    // All signed-byte biases and weights, including extrema outside mutation limits.
    for (int v = -128; v < 128; ++v)
    {
        special.allNeurons[0][0].bias = static_cast<int8_t>(v);
        special.allNeurons[0][0].allConnections[0].weight = static_cast<int8_t>(v);
        special.allNeurons[NeuronOutputLayerIndex][0].bias = static_cast<int8_t>(v);
        Compare(special.allNeurons, inputs, unsigned(v + 128));
    }

    BotNeuralNet thresholds;
    thresholds.allNeurons[0][0].AddConnection(1, 0, 50);
    thresholds.allNeurons[1][0].type = NeuronType::memory;
    thresholds.allNeurons[1][0].AddConnection(NeuronOutputLayerIndex, 0, 50);
    thresholds.allNeurons[0][1].AddConnection(1, 1, 50);
    thresholds.allNeurons[1][1].type = radialbasis;
    thresholds.allNeurons[1][1].AddConnection(NeuronOutputLayerIndex, 1, 50);
    std::vector<BrainInput> boundaryInputs;
    for (float x : {-1.001f, -1.0f, -.999f, .449f, .45f, .5f, .55f, .551f, .999f, 1.0f, 1.001f, 0.0f})
    {
        BrainInput input{};
        for (auto& f : input.fields) f = x;
        boundaryInputs.push_back(input);
    }
    Compare(thresholds.allNeurons, boundaryInputs, 999);

    auto shared = GenomeRef::Create();
    brain::Randomize(shared.MakeUnique()->initial);
    shared.MakeUnique()->RebuildActive();
    Require(shared->ExecutionStorageBytes() > 0, "genome owns a compiled plan");
    BrainInput input{}; input.energy = 1;
    CompareGenome(*shared.MakeUnique(), input, 714);
    GenomeRef edited = shared;
    auto* own = edited.MakeUnique();
    Require(edited.get() != shared.get(), "editing copies the shared genome");
    own->InvalidateActivePlan();
    own->active[NeuronOutputLayerIndex][0].bias = 77;
    CompareGenome(*own, input, 714); // Editable view before Recheck uses the reference.
    own->RecheckActive();
    CompareGenome(*own, input, 714);
    CompareGenome(*shared.MakeUnique(), input, 714);
    brain::Mutate(own->initial);
    own->RebuildActive();
    CompareGenome(*own, input, 714);
    CheckMutationSharing();
    std::printf("PASS: %zu comparisons; brain steps and mutation batches preserve state and RNG\n", comparisons);
    return 0;
}
