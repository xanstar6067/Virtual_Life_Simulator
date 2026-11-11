// Ecological regression study. Builds against complete old/current source copies.
// Metadata is added only to those copies; the production models are untouched.
#include "cb3/Field.h"
#include "cb3/SDL.h"
#include "CB3EcologyHooks.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <memory>
#include <numeric>
#include <random>
#include <unordered_map>

SDL_Renderer* renderer = nullptr;
SDL_Window* window = nullptr;
int windowWidth = 1920, windowHeight = 1080;
ImGuiIO* io = nullptr;
MouseState mouseState{};
#undef main
#define CHECK(c) do { if (!(c)) { std::fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c); std::exit(3); } } while (0)

namespace cb3
{
struct EcologyProbe
{
    static int Wrap(int x) { return (x % (FieldCellsWidth) + FieldCellsWidth) % (FieldCellsWidth); }
    static void Relocate(Field& f, int shift, bool cluster, unsigned seed)
    {
        std::vector<Object*> cells(FieldCellsWidth * FieldCellsHeight, nullptr);
        std::vector<unsigned> positions;
        if (cluster)
        {
            positions.resize((FieldCellsWidth / 4) * FieldCellsHeight);
            std::iota(positions.begin(), positions.end(), 0);
            std::mt19937 rng(seed ^ 0x8796231u);
            std::shuffle(positions.begin(), positions.end(), rng);
        }
        size_t index = 0;
        for (int x = 0; x < FieldCellsWidth; ++x)
            for (int y = 0; y < FieldCellsHeight; ++y)
                if (auto* o = f.allCells[x][y])
                {
                    int nx = Wrap(x + shift), ny = y;
                    if (cluster)
                    {
                        CHECK(index < positions.size());
                        nx = positions[index] / FieldCellsHeight;
                        ny = positions[index++] % FieldCellsHeight;
                    }
                    if (o->type() == bot)
                    {
                        auto* b = static_cast<Bot*>(o);
                        auto state = b->GetPersistentState();
                        state.addaptation_birthX = Wrap(state.addaptation_birthX + nx - x);
                        b->SetPersistentState(state);
                    }
                    o->x = nx; o->y = ny;
                    CHECK(cells[size_t(nx) * FieldCellsHeight + ny] == nullptr);
                    cells[size_t(nx) * FieldCellsHeight + ny] = o;
                }
        std::memcpy(f.allCells, cells.data(), sizeof(f.allCells));
    }

    static std::vector<unsigned> Cuts(const Field& f)
    {
#ifdef ECOLOGY_ADAPTIVE
        return {f.workBoundaries, f.workBoundaries + 2 * f.numThreads + 1};
#else
        return {};
#endif
    }

    static void NeutralPopulation(Field& f, unsigned seed)
    {
        f.params.noMutations = true;
        constexpr int left = 528, right = left + 384, top = 4, bottom = 44;
        for (int x = left; x <= right; ++x)
        {
            CHECK(f.AddObject(new Rock(x, top)));
            CHECK(f.AddObject(new Rock(x, bottom)));
        }
        for (int y = top + 1; y < bottom; ++y)
        {
            CHECK(f.AddObject(new Rock(left, y)));
            CHECK(f.AddObject(new Rock(right, y)));
        }
        auto prototype = std::make_unique<Bot>(0, 0, 500);
        NeuronRow neurons[NumNeuronLayers];
        brain::InitNeurons(neurons);
        neurons[NeuronOutputLayerIndex][3].bias = 127; // photosynthesis
        neurons[NeuronOutputLayerIndex][6].bias = 127; // division
        GenomeRef genome(new BrainGenome());
        genome.MakeUnique()->ReplaceInitial(neurons);
        genome.MakeUnique()->RebuildActive();
        prototype->ShareGenome(genome);
        std::mt19937 rng(seed ^ 0x35e179u);
        for (unsigned team = 1; team <= 2; ++team)
        {
            std::vector<Point> positions;
            for (int x = left + 1; x < right; ++x)
                if ((x < (left + right) / 2) == (team == 1))
                    for (int y = top + 1; y < bottom; ++y) positions.emplace_back(x, y);
            std::shuffle(positions.begin(), positions.end(), rng);
            for (unsigned i = 0; i < 200; ++i)
            {
                auto* b = new Bot(positions[i].x, positions[i].y, 500, prototype.get(), false);
                b->ecologyTeam = team; // hidden test label, not color/kinship/input
                b->SetDirection(0);
                CHECK(f.AddObject(b));
            }
        }
    }
};
}
using namespace cb3;

static std::string InitialKey(const BrainGenome& genome)
{
    NeuronRow neurons[NumNeuronLayers];
    genome.ReadInitial(neurons);
    std::string key;
    for (unsigned layer = 0; layer < NumNeuronLayers; ++layer)
        for (unsigned i = 0; i < neuronsInLayer[layer]; ++i)
        {
            const auto& n = neurons[layer][i];
            key.push_back(char(n.type)); key.push_back(char(n.bias)); key.push_back(char(n.numConnections));
            for (unsigned j = 0; j < n.numConnections; ++j)
            {
                const auto& c = n.allConnections[j];
                key.push_back(char(c.dest_layer)); key.push_back(char(c.dest_neuron)); key.push_back(char(c.weight));
            }
        }
    return key; // unordered_map checks full strings even when their hashes collide
}
static double Effective(const std::vector<uint64_t>& counts, uint64_t total)
{
    if (!total) return NAN;
    double squared = 0;
    for (auto c : counts) { const double p = double(c) / total; squared += p * p; }
    return 1.0 / squared;
}
static void Sample(Field& f, const std::string& scenario, unsigned seed, unsigned shift,
    unsigned tick, unsigned rebalances, unsigned removedByCrisis, uint64_t minimum,
    int extinction, uint64_t preCrisis, int recovery)
{
    uint64_t bots = 0, apples = 0, organics = 0, left = 0, energy = 0, ages = 0, low = 0;
    uint64_t diets[5] = {};
    std::vector<uint64_t> founders(ecology::nextFounder + 1, 0);
    std::unordered_map<std::string, uint64_t> brainCounts;
    std::unordered_map<const BrainGenome*, uint64_t*> cache;
    for (int x = 0; x < FieldCellsWidth; ++x)
        for (int y = 0; y < FieldCellsHeight; ++y)
            if (auto* o = f.GetObjectLocalCoords(x, y))
            {
                CHECK(o->x == x && o->y == y);
                if (o->type() == apple) ++apples;
                else if (o->type() == organic_waste) ++organics;
                else if (o->type() == bot)
                {
                    auto* b = static_cast<Bot*>(o);
                    ++bots; energy += (std::max)(0, b->energy); ages += b->GetLifetime();
                    low += b->energy < f.params.botMaxEnergy * .2;
                    CHECK(b->ecologyFounder > 0 && b->ecologyFounder < founders.size());
                    ++founders[b->ecologyFounder]; left += b->ecologyTeam == 1;
                    ++diets[int(vlsui::ClassifyFeeding(b->GetEnergyFromPS(), b->GetEnergyFromKills(), b->GetEnergyFromOrganics()))];
                    const auto* g = &b->GetGenome();
                    auto it = cache.find(g);
                    if (it == cache.end()) it = cache.emplace(g, &brainCounts[InitialKey(*g)]).first;
                    ++*it->second;
                }
            }
    const auto totals = f.GetTelemetryTotals();
    CHECK(totals.added + totals.births == bots + totals.deaths + totals.removed);
    std::vector<uint64_t> frequencies;
    for (const auto& entry : brainCounts) frequencies.push_back(entry.second);
    unsigned survivingFounders = 0;
    for (auto c : founders) survivingFounders += c != 0;
    const double n = bots ? double(bots) : NAN;
    std::printf("%u,%s,%u,%u,%llu,%llu,%llu,%.8f,%.8f,%.8f,%zu,%.8f,%u,%.8f,%.8f,%.8f,%.8f,%.8f,%.8f,%llu,%llu,%u,%u,%llu,%d,%llu,%d\n",
        seed, scenario.c_str(), shift, tick, bots, apples, organics, energy / n, ages / n, 100.0 * low / n,
        brainCounts.size(), Effective(frequencies, bots), survivingFounders, Effective(founders, bots),
        100.0 * diets[1] / n, 100.0 * diets[2] / n, 100.0 * diets[3] / n, 100.0 * diets[4] / n,
        100.0 * diets[0] / n, totals.births, totals.deaths, rebalances, removedByCrisis,
        minimum, extinction, preCrisis, recovery);
    // Left/right lineage frequency is separate from the brain-visible state.
    std::printf("LABEL,%u,%s,%u,%u,%.8f\n", seed, scenario.c_str(), shift, tick,
        scenario == "neutral" ? 100.0 * left / n : NAN);
    std::fflush(stdout);
}

static unsigned Crisis(Field& f, unsigned seed)
{
    unsigned removed = 0;
    std::mt19937 rng(seed ^ 0x72131u);
    for (int x = 0; x < FieldCellsWidth; ++x)
        for (int y = 0; y < FieldCellsHeight; ++y)
        {
            const bool remove = rng() % 4 != 0; // same spatial mask in both versions
            if (auto* o = f.GetObjectLocalCoords(x, y); o && o->type() == bot && remove)
            {
                f.RemoveObject(x, y);
                ++removed;
            }
        }
    return removed;
}

int wmain(int argc, wchar_t** argv)
{
    unsigned seed = 12345, ticks = 600, shift = 0, interval = 50;
    std::string scenario = "normal";
    std::filesystem::path initialOut, finalOut;
    for (int i = 1; i < argc; ++i)
    {
        const std::wstring a = argv[i];
        auto value = [&]() { CHECK(++i < argc); return argv[i]; };
        if (a == L"--seed") seed = _wtoi(value());
        else if (a == L"--ticks") ticks = _wtoi(value());
        else if (a == L"--shift") shift = _wtoi(value());
        else if (a == L"--threads") ecology::threads = _wtoi(value());
        else if (a == L"--interval") interval = _wtoi(value());
        else if (a == L"--scenario") { std::wstring v = value(); scenario.clear(); for (auto c : v) scenario.push_back(static_cast<char>(c)); }
        else if (a == L"--initial-out") initialOut = value();
        else if (a == L"--final-out") finalOut = value();
        else CHECK(false);
    }
    CHECK(interval > 0 && ticks > 0 && shift < (FieldCellsWidth));
    CHECK(ecology::threads >= 1 && ecology::threads <= NumThreads);
    CHECK(scenario == "normal" || scenario == "cluster" || scenario == "scarce" || scenario == "crisis" || scenario == "neutral");
    srand(seed); Field::seed = seed; ecology::nextFounder = 1;
    auto field = std::make_unique<Field>();
    if (scenario == "scarce") field->params.PSreward = 2;
    if (scenario == "neutral") EcologyProbe::NeutralPopulation(*field, seed);
    else for (unsigned i = 0; i < 5; ++i) field->SpawnControlGroup();
    if (scenario == "cluster") EcologyProbe::Relocate(*field, 0, true, seed);
    if (!initialOut.empty()) { ObjectSaver saver; CHECK(saver.SaveWorld(field.get(), initialOut, 1, 0)); }
    if (shift)
    {
        // Verify both the forward geometry and its inverse, including birth X.
        struct Position { Object* object; int x, y, birth; };
        std::vector<Position> original;
        for (int x = 0; x < FieldCellsWidth; ++x)
            for (int y = 0; y < FieldCellsHeight; ++y)
                if (auto* o = field->GetObjectLocalCoords(x, y))
                    original.push_back({o, x, y, o->type() == bot ? static_cast<Bot*>(o)->GetPersistentState().addaptation_birthX : 0});
        EcologyProbe::Relocate(*field, shift, false, seed);
        for (const auto& p : original)
        {
            CHECK(p.object->x == EcologyProbe::Wrap(p.x + shift) && p.object->y == p.y);
            if (p.object->type() == bot)
                CHECK(static_cast<Bot*>(p.object)->GetPersistentState().addaptation_birthX == EcologyProbe::Wrap(p.birth + shift));
        }
        EcologyProbe::Relocate(*field, FieldCellsWidth - shift, false, seed);
        for (const auto& p : original)
        {
            CHECK(field->GetObjectLocalCoords(p.x, p.y) == p.object);
            if (p.object->type() == bot) CHECK(static_cast<Bot*>(p.object)->GetPersistentState().addaptation_birthX == p.birth);
        }
        EcologyProbe::Relocate(*field, shift, false, seed);
    }
    std::puts("seed,scenario,shift,tick,bots,apples,organics,mean_energy,mean_age,low_energy_pct,brain_richness,brain_effective,founder_richness,founder_effective,ps_pct,predator_pct,organic_pct,mixed_pct,unknown_pct,births,deaths,rebalances,crisis_removed,min_bots,extinction_tick,pre_crisis_bots,recovery_ticks");
    unsigned rebalances = 0, removed = 0;
    auto live = [&]() { const auto t = field->GetTelemetryTotals(); return t.added + t.births - t.deaths - t.removed; };
    uint64_t minimum = live(), preCrisis = 0;
    int extinction = -1, recovery = -1;
    Sample(*field, scenario, seed, shift, 0, rebalances, removed, minimum, extinction, preCrisis, recovery);
    for (unsigned tick = 1; tick <= ticks; ++tick)
    {
        if (scenario == "crisis")
        {
            if (tick == 251) { preCrisis = live(); removed = Crisis(*field, seed); field->params.PSreward = 0; }
            if (tick == 351) field->params.PSreward = PSRewardInitial;
        }
        const auto before = EcologyProbe::Cuts(*field);
        field->tick(tick);
        const auto population = live();
        minimum = (std::min)(minimum, population);
        if (!population && extinction < 0) extinction = int(tick);
        if (scenario == "crisis" && tick >= 351 && preCrisis > 0 && population * 10 >= preCrisis * 9 && recovery < 0)
            recovery = int(tick) - 350;
        rebalances += before != EcologyProbe::Cuts(*field);
        if (tick % interval == 0 || tick == ticks)
            Sample(*field, scenario, seed, shift, tick, rebalances, removed, minimum, extinction, preCrisis, recovery);
    }
    if (!finalOut.empty()) { ObjectSaver saver; CHECK(saver.SaveWorld(field.get(), finalOut, 1, ticks)); }
}
