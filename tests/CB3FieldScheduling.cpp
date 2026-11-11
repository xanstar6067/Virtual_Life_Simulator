// Real engine workers, no UI. The reference wakes exactly one worker at a time
// in each spatial pass, preserving that worker's actual CRT random stream.
#include "../source/cb3/Field.h"
#include "../source/cb3/SDL.h"
#include "CB3PerformanceHooks.h"
#include <cstdio>
#include <fstream>
#include <iterator>
#include <memory>
#include <random>

SDL_Renderer* renderer = nullptr;
SDL_Window* window = nullptr;
int windowWidth = 1920, windowHeight = 1080;
ImGuiIO* io = nullptr;
MouseState mouseState{};
#undef main

#define CHECK(condition) do { if (!(condition)) { \
    std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); std::abort(); } } while (0)

namespace cb3
{
struct FieldSchedulingTest
{
    static void CheckLayout(const Field& f)
    {
        const unsigned parts = 2 * f.numThreads;
        CHECK(f.workBoundaries[0] == 0 && f.workBoundaries[parts] == FieldCellsWidth);
        for (unsigned p = 0; p < parts; ++p)
            CHECK(f.workBoundaries[p + 1] - f.workBoundaries[p] >= 2);
        // Mark the entire one-column halo, including the cylindrical seam.
        // No concurrently running workers may even read the same halo column.
        for (unsigned pass = 0; pass < 2; ++pass)
        {
            bool touched[FieldCellsWidth] = {};
            for (unsigned p = pass; p < parts; p += 2)
                for (int x = int(f.workBoundaries[p]) - 1; x <= int(f.workBoundaries[p + 1]); ++x)
                {
                    const unsigned column = (x + FieldCellsWidth) % (FieldCellsWidth);
                    CHECK(!touched[column]);
                    touched[column] = true;
                }
        }
    }

    static uint64_t Cost(const Field& f)
    {
        uint64_t maximum[2] = {};
        for (unsigned p = 0; p < 2u * f.numThreads; ++p)
        {
            uint64_t sum = 0;
            for (unsigned x = f.workBoundaries[p]; x < f.workBoundaries[p + 1]; ++x)
                sum += f.columnWork[x];
            maximum[p % 2] = (std::max)(maximum[p % 2], sum);
        }
        return maximum[0] + maximum[1];
    }

    static void PartitionTests()
    {
        std::mt19937 rng(654321);
        for (unsigned workers = 2; workers <= NumThreads; ++workers)
        {
            audit::requestedThreads = workers;
            auto f = std::make_unique<Field>();
            for (unsigned trial = 0; trial < 128; ++trial)
            {
                for (unsigned x = 0; x < FieldCellsWidth; ++x)
                {
                    switch (trial % 4)
                    {
                    case 0: f->columnWork[x] = 133; break;
                    case 1: f->columnWork[x] = 133 + (x < 752 ? 34048 : 0); break;
                    case 2: f->columnWork[x] = 133 + (x == trial ? 10000000 : 0); break;
                    default: f->columnWork[x] = 133 + (rng() % 34049); break;
                    }
                }
                f->columnWorkReady = true;
                f->rebalanceCountdown = 0;
                const auto before = Cost(*f);
                f->RebalanceWork();
                CHECK(Cost(*f) <= before);
                CheckLayout(*f);
            }
        }
        std::puts("PASS 1920 layouts: coverage, minimum widths, cyclic halos, predicted cost");
    }

    static void SerialTick(Field& f, unsigned frame)
    {
        f.telemetryInTick = true;
        if (f.params.useSeasons) f.SeasonTick();
        Object::currentFrame = frame;
        if (f.params.spawnApples && f.spawnApplesCounter++ == AppleSpawnInterval)
        {
            f.SpawnApples();
            f.spawnApplesCounter = 0;
        }
        f.RebalanceWork();
        f.objectsTotal = f.botsTotal = f.applesTotal = f.organicsTotal = 0;
        f.predatorsTotal = f.averageLifetime = 0;
        for (unsigned pass = 0; pass < 2; ++pass)
        {
            for (int i = 0; i < f.numThreads; ++i) f.objectCounters[i].Clear();
            {
                std::lock_guard<std::mutex> lock(f.threadMutex);
                f.threadsReady = 0;
            }
            for (int i = 0; i < f.numThreads; ++i)
            {
                {
                    std::lock_guard<std::mutex> lock(f.threadMutex);
                    f.threadGoMarker[i] = true;
                }
                f.threadStartCondition.notify_all();
                std::unique_lock<std::mutex> lock(f.threadMutex);
                f.threadDoneCondition.wait(lock, [&] { return f.threadsReady == i + 1; });
            }
            for (int i = 0; i < f.numThreads; ++i)
            {
                const auto& c = f.objectCounters[i];
                f.objectsTotal += c.objects; f.botsTotal += c.bots;
                f.applesTotal += c.apples; f.organicsTotal += c.organics;
                f.predatorsTotal += c.predators; f.averageLifetime += c.lifetime;
            }
        }
        if (f.botsTotal) f.averageLifetime /= f.botsTotal;
        f.columnWorkReady = true;
        f.telemetryInTick = false;
    }

    static void Populate(Field& f, bool clustered)
    {
        f.params.Reset();
        f.params.fertility_delay = 0;
        f.params.botMaxLifetime = 10000;
        f.params.useSeasons = true;
        f.params.seasonInterval = 7;
        f.params.spawnApples = true;
        if (!clustered)
        {
            for (unsigned i = 0; i < 5; ++i) f.SpawnControlGroup();
            return;
        }
        std::vector<std::unique_ptr<Bot>> pool;
        for (unsigned i = 0; i < 16; ++i)
        {
            auto b = std::make_unique<Bot>(0, 0, 500);
            if (i < 8)
            {
                NeuronRow neurons[NumNeuronLayers];
                brain::InitNeurons(neurons);
                // Movement in both horizontal directions, photosynthesis,
                // attacks, organic feeding, births, and combinations of them.
                neurons[(NumNeuronLayers - 1)][i % 2 ? 0 : 1].bias = i % 2 ? 127 : -127;
                neurons[(NumNeuronLayers - 1)][2 + i % 6].bias = 127;
                if (i == 7) neurons[(NumNeuronLayers - 1)][6].bias = 127;
                auto g = GenomeRef(new BrainGenome());
                g.MakeUnique()->ReplaceInitial(neurons);
                g.MakeUnique()->RebuildActive();
                b->ShareGenome(g);
            }
            pool.push_back(std::move(b));
        }
        unsigned index = 0;
        for (int x = 0; x < FieldCellsWidth; ++x)
        {
            // Dense population on both sides of the cylindrical seam.
            if (x >= 360 && x < FieldCellsWidth - 90) continue;
            for (int y = 1; y < FieldCellsHeight - 1; y += 2)
            {
                auto* b = new Bot(x, y, 500, pool[index++ % pool.size()].get(), false);
                auto state = b->GetPersistentState();
                state.fertilityDelay = 0;
                b->SetPersistentState(state);
                CHECK(f.AddObject(b));
                if (x % 3 == 0) CHECK(f.AddObject(new Apple(x, y + 1, 100)));
                else if (x % 3 == 1) CHECK(f.AddObject(new Organics(x, y + 1, 100)));
            }
        }
    }

    struct Observation
    {
        uint64_t births, deaths;
        unsigned objects, bots, apples, organics, predators, age;
        std::vector<unsigned> cuts, work;
        bool operator==(const Observation&) const = default;
    };
    struct RunResult
    {
        std::vector<Observation> observations;
        std::vector<std::vector<char>> saves;
        uint64_t moves = 0, attacks = 0;
        unsigned rebalances = 0;
    };
    static RunResult Run(unsigned workers, bool clustered, bool serial, const std::filesystem::path& out)
    {
        constexpr unsigned seed = 12345;
        srand(seed); Field::seed = seed;
        audit::requestedThreads = workers;
        auto f = std::make_unique<Field>();
        Populate(*f, clustered);
        RunResult result;
        for (unsigned frame = 1; frame <= 64; ++frame)
        {
            std::vector<unsigned> previous(f->workBoundaries, f->workBoundaries + 2 * workers + 1);
            if (serial) SerialTick(*f, frame); else f->tick(frame);
            CHECK(f->collectColumnWork == ((frame - 1) % 16 == 0));
            CheckLayout(*f);
            const auto t = f->GetTelemetryTotals();
            result.observations.push_back({t.births, t.deaths, f->objectsTotal, f->botsTotal,
                f->applesTotal, f->organicsTotal, f->predatorsTotal, f->averageLifetime,
                {f->workBoundaries, f->workBoundaries + 2 * workers + 1},
                {f->columnWork, f->columnWork + FieldCellsWidth}});
            if (previous != result.observations.back().cuts) ++result.rebalances;
            if (frame % 16 == 0)
            {
                const auto path = out / (std::to_string(workers) + (clustered ? "-cluster-" : "-growth-") +
                    (serial ? "serial-" : "parallel-") + std::to_string(frame) + ".world");
                ObjectSaver saver;
                CHECK(saver.SaveWorld(f.get(), path, 1, frame));
                std::ifstream file(path, std::ios::binary);
                result.saves.emplace_back(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
            }
        }
        for (int x = 0; x < FieldCellsWidth; ++x)
            for (int y = 0; y < FieldCellsHeight; ++y)
                if (auto* o = f->GetObjectLocalCoords(x, y); o && o->type() == bot)
                {
                    const auto s = static_cast<Bot*>(o)->GetPersistentState();
                    // New random bots start these movement counters at 1000.
                    if (s.numMovesX < 1000) result.moves += s.numMovesX;
                    if (s.numMovesY < 1000) result.moves += s.numMovesY;
                    result.attacks += s.numAttacks;
                }
        return result;
    }

    static void IntegrationTests(const std::filesystem::path& out)
    {
        for (unsigned workers : {2u, 3u, 4u, 8u, 16u})
            for (bool clustered : {false, true})
            {
                const auto reference = Run(workers, clustered, true, out);
                const auto parallel = Run(workers, clustered, false, out);
                CHECK(reference.observations == parallel.observations);
                CHECK(reference.saves == parallel.saves);
                CHECK(parallel.observations.back().births > 0 && parallel.observations.back().deaths > 0);
                if (clustered) CHECK(parallel.rebalances > 0 && parallel.moves > 0 && parallel.attacks > 0);
                std::printf("PASS workers=%u clustered=%d frames=64 rebalances=%u births=%llu deaths=%llu moves=%llu attacks=%llu saves=4\n",
                    workers, clustered, parallel.rebalances, parallel.observations.back().births,
                    parallel.observations.back().deaths, parallel.moves, parallel.attacks);
                std::fflush(stdout);
            }
    }
};
}

int wmain()
{
    const auto out = std::filesystem::path(L"x64/perf-audit/scheduling-tests/worlds");
    std::filesystem::create_directories(out);
    cb3::FieldSchedulingTest::PartitionTests();
    cb3::FieldSchedulingTest::IntegrationTests(out);
    std::puts("PASS field scheduling: parallel execution equals serialized spatial passes");
}
