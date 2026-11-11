// Headless performance research; no application Main, window, UI, or user writes.
#include "../source/cb3/Field.h"
#include "../source/cb3/SDL.h"
#include "CB3PerformanceHooks.h"
#include <psapi.h>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <numeric>
#include <random>
#include <unordered_set>

SDL_Renderer* renderer = nullptr;
SDL_Window* window = nullptr;
int windowWidth = 1920, windowHeight = 1080;
ImGuiIO* io = nullptr;
MouseState mouseState{};
#undef main

using namespace cb3;
static double frequency;
static volatile uint64_t scanSink = 0;
static double Milliseconds(uint64_t ticks) { return ticks * 1000.0 / frequency; }
static uint64_t CpuTime()
{
    FILETIME created, exited, kernel, user;
    GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user);
    ULARGE_INTEGER k{}, u{};
    k.LowPart = kernel.dwLowDateTime; k.HighPart = kernel.dwHighDateTime;
    u.LowPart = user.dwLowDateTime; u.HighPart = user.dwHighDateTime;
    return k.QuadPart + u.QuadPart;
}
struct Snapshot
{
    size_t bots = 0, genomes = 0, apples = 0, organics = 0, rocks = 0;
    size_t connections = 0, activeNeurons = 0;
    size_t planBytes = 0;
    size_t neuronBytes = 0, editableBytes = 0;
    double privateMiB = 0, workingMiB = 0;
};
static Snapshot Inspect(Field& field)
{
    Snapshot s;
    PROCESS_MEMORY_COUNTERS_EX mem{};
    mem.cb = sizeof(mem);
    GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&mem), sizeof(mem));
    s.privateMiB = mem.PrivateUsage / 1048576.0;
    s.workingMiB = mem.WorkingSetSize / 1048576.0;
    std::unordered_set<const BrainGenome*> genomes;
    for (int x = 0; x < FieldCellsWidth; ++x)
        for (int y = 0; y < FieldCellsHeight; ++y)
            if (auto* o = field.GetObjectLocalCoords(x, y))
            {
                switch (o->type())
                {
                case bot: ++s.bots; genomes.insert(&static_cast<Bot*>(o)->GetGenome()); break;
                case apple: ++s.apples; break;
                case organic_waste: ++s.organics; break;
                case rock: ++s.rocks; break;
                }
            }
    s.genomes = genomes.size();
    for (auto* g : genomes)
    {
        s.planBytes += g->ExecutionStorageBytes();
        s.neuronBytes += g->NeuronStorageBytes();
        s.editableBytes += g->EditableStorageBytes();
        NeuronRow active[NumNeuronLayers];
        g->ReadActive(active);
        for (unsigned l = 0; l < NumNeuronLayers; ++l)
            for (unsigned n = 0; n < neuronsInLayer[l]; ++n)
            {
                s.connections += active[l][n].numConnections;
                s.activeNeurons += active[l][n].numConnections != 0;
            }
    }
    return s;
}
static void PrintSnapshot(const char* label, const Snapshot& s)
{
    std::printf("%s bots=%zu genomes=%zu apples=%zu organics=%zu rocks=%zu private_mib=%.3f working_mib=%.3f mean_connections=%.2f mean_active_neurons=%.2f plan_mib=%.3f neuron_mib=%.3f editable_mib=%.3f\n",
        label, s.bots, s.genomes, s.apples, s.organics, s.rocks, s.privateMiB, s.workingMiB,
        s.genomes ? double(s.connections) / s.genomes : 0,
        s.genomes ? double(s.activeNeurons) / s.genomes : 0, s.planBytes / 1048576.0,
        s.neuronBytes / 1048576.0, s.editableBytes / 1048576.0);
    std::fflush(stdout);
}
static void Fill(Field& field, size_t count, unsigned poolSize, bool duplicate, bool clustered, bool stunned)
{
    if (!count) return;
    poolSize = (std::max)(1u, (std::min)(poolSize, unsigned(count)));
    std::vector<std::unique_ptr<Bot>> pool;
    for (unsigned i = 0; i < poolSize; ++i) pool.push_back(std::make_unique<Bot>(0, 0, 500));
    std::vector<unsigned> cells(FieldCellsWidth * FieldCellsHeight);
    std::iota(cells.begin(), cells.end(), 0);
    if (!clustered) { std::mt19937 rng(12345); std::shuffle(cells.begin(), cells.end(), rng); }
    for (size_t i = 0; i < count; ++i)
    {
        const auto cell = cells[i];
        auto* b = new Bot(cell / FieldCellsHeight, cell % FieldCellsHeight, 500, pool[i % poolSize].get(), false);
        if (duplicate) b->ShareGenome(GenomeRef(new BrainGenome(b->GetGenome())));
        if (stunned) { auto state = b->GetPersistentState(); state.stunned = 1000000000; b->SetPersistentState(state); }
        if (!field.AddObject(b)) std::abort();
    }
}
static void PrintWorkers(unsigned threads, unsigned ticks)
{
    uint64_t stages[audit::StageCount]{};
    uint64_t samples = 0, calls = 0;
    for (unsigned i = 0; i < 17; ++i)
    {
        const auto& w = audit::workers[i];
        samples += w.samples; calls += w.calls;
        for (int s = 0; s < audit::StageCount; ++s) stages[s] += w.stage[s];
        if (i < threads && threads > 1)
            std::printf("WORKER index=%u phase0_ms=%.5f phase1_ms=%.5f objects0=%.2f objects1=%.2f\n",
                i, Milliseconds(w.phase[0]) / ticks, Milliseconds(w.phase[1]) / ticks,
                double(w.objects[0]) / ticks, double(w.objects[1]) / ticks);
    }
    if (samples)
        std::printf("PROFILE samples=%llu calls=%llu bot_ms=%.5f sense_ms=%.5f think_ms=%.5f multiply_ms=%.5f mutate_ms=%.5f rebuild_ms=%.5f\n",
            samples, calls, Milliseconds(stages[0]), Milliseconds(stages[1]), Milliseconds(stages[2]),
            Milliseconds(stages[3]), Milliseconds(stages[4]), Milliseconds(stages[5]));
}
int wmain(int argc, wchar_t** argv)
{
    std::wstring mode = L"fixed", world, out;
    size_t count = 100000;
    unsigned pool = 1024, warmup = 10, minTicks = 0, waves = 5, seed = 12345;
    double targetMs = 600;
    bool duplicate = false, clustered = false;
    for (int i = 1; i < argc; ++i)
    {
        std::wstring a = argv[i];
        auto value = [&]() -> const wchar_t* { if (++i >= argc) std::abort(); return argv[i]; };
        if (a == L"--mode") mode = value();
        else if (a == L"--world") world = value();
        else if (a == L"--out") out = value();
        else if (a == L"--bots") count = _wtoi64(value());
        else if (a == L"--pool") pool = _wtoi(value());
        else if (a == L"--threads") audit::requestedThreads = _wtoi(value());
        else if (a == L"--warmup") warmup = _wtoi(value());
        else if (a == L"--ticks") minTicks = _wtoi(value());
        else if (a == L"--ms") targetMs = _wtof(value());
        else if (a == L"--seed") seed = _wtoi(value());
        else if (a == L"--waves") waves = _wtoi(value());
        else if (a == L"--duplicate-genomes") duplicate = true;
        else if (a == L"--clustered") clustered = true;
        else if (a == L"--fixed-partitions") audit::fixedPartitions = true;
        else { std::fprintf(stderr, "Unknown option\n"); return 2; }
    }
    if (count > FieldCellsWidth * FieldCellsHeight || audit::requestedThreads < 1 || audit::requestedThreads > NumThreads) return 2;
    LARGE_INTEGER f; QueryPerformanceFrequency(&f); frequency = double(f.QuadPart);
    MEMORYSTATUSEX mem{}; mem.dwLength = sizeof(mem); GlobalMemoryStatusEx(&mem);
    std::printf("CONFIG threads=%d cpu_count=%d ram_mib=%.0f Bot=%zu Genome=%zu Neuron=%zu grid_bytes=%zu seed=%u profile=%d\n",
        audit::requestedThreads, SDL_GetCPUCount(), mem.ullTotalPhys / 1048576.0, sizeof(Bot), sizeof(BrainGenome), sizeof(Neuron),
        sizeof(void*) * FieldCellsWidth * FieldCellsHeight, seed,
#ifdef CB3_AUDIT_PROFILE
        1
#else
        0
#endif
    );
    audit::freeze = mode == L"fixed" || mode == L"skipbrain" || mode == L"stunned";
    audit::skipBrain = mode == L"skipbrain";
    srand(seed); Field::seed = seed;
    auto field = std::make_unique<Field>();
    PrintSnapshot("EMPTY", Inspect(*field));
    unsigned frame = 1;
    const uint64_t setupStart = audit::Now();
    if (!world.empty())
    {
        ObjectSaver saver;
        const auto loaded = saver.LoadWorld(field.get(), std::filesystem::path(world));
        if (loaded.width < 0) { std::fprintf(stderr, "Load failed\n"); return 3; }
        frame = unsigned(loaded.tick + 1);
        Field::seed = loaded.seed;
        if (duplicate)
            for (int x = 0; x < FieldCellsWidth; ++x)
                for (int y = 0; y < FieldCellsHeight; ++y)
                    if (auto* o = field->GetObjectLocalCoords(x, y); o && o->type() == bot)
                    {
                        auto* b = static_cast<Bot*>(o);
                        b->ShareGenome(GenomeRef(new BrainGenome(b->GetGenome())));
                    }
    }
    else if (mode == L"growth")
        for (unsigned i = 0; i < waves; ++i) field->SpawnControlGroup();
    else Fill(*field, count, pool, duplicate, clustered, mode == L"stunned");
    if (audit::freeze) { field->params.botMaxLifetime = INT_MAX; field->params.spawnApples = false; field->params.useSeasons = false; }
    std::printf("SETUP ms=%.3f frame=%u\n", Milliseconds(audit::Now() - setupStart), frame);
    PrintSnapshot("START", Inspect(*field));
    if (mode == L"growth")
    {
        unsigned checkpoint = 0;
        const unsigned thresholds[] = {50000, 200000, 500000};
        const unsigned end = frame + (minTicks ? minTicks : 1500);
        while (frame < end)
        {
            const auto before = field->GetTelemetryTotals();
            const auto start = audit::Now(), cpu = CpuTime();
            unsigned steps = (std::min)(25u, end - frame);
            for (unsigned i = 0; i < steps; ++i) field->tick(frame++);
            const auto elapsed = audit::Now() - start;
            const auto cpuElapsed = CpuTime() - cpu;
            const auto after = field->GetTelemetryTotals();
            auto s = Inspect(*field);
            std::printf("GROWTH frame=%u ms_tick=%.5f cpu_cores=%.3f births=%llu deaths=%llu ", frame - 1,
                Milliseconds(elapsed) / steps, cpuElapsed / 10000.0 / Milliseconds(elapsed),
                after.births - before.births, after.deaths - before.deaths);
            PrintSnapshot("STATE", s);
            std::printf("SCHEDULER rebalances=%u\n", audit::rebalances);
            if (checkpoint < std::size(thresholds) && s.bots >= thresholds[checkpoint] && !out.empty())
            {
                std::filesystem::create_directories(out);
                const auto path = std::filesystem::path(out) / ("growth-" + std::to_string(thresholds[checkpoint]) + ".world");
                ObjectSaver saver;
                if (!saver.SaveWorld(field.get(), path, 1, int(frame - 1))) return 4;
                std::printf("CHECKPOINT threshold=%u\n", thresholds[checkpoint++]);
            }
            if (!s.bots) break;
        }
        return 0;
    }
    for (unsigned i = 0; i < warmup; ++i) field->tick(frame++);
    PrintSnapshot("MEASURE_START", Inspect(*field));
    for (auto& w : audit::workers) w = {};
    std::vector<double> times;
    const auto before = field->GetTelemetryTotals();
    const auto start = audit::Now(), cpu = CpuTime();
    do
    {
        const auto tickStart = audit::Now();
        field->tick(frame++);
        times.push_back(Milliseconds(audit::Now() - tickStart));
    } while ((minTicks ? times.size() < minTicks : Milliseconds(audit::Now() - start) < targetMs));
    const double wallMs = Milliseconds(audit::Now() - start);
    const double cpuMs = (CpuTime() - cpu) / 10000.0;
    const auto after = field->GetTelemetryTotals();
    std::sort(times.begin(), times.end());
    std::printf("RESULT ticks=%zu wall_ms=%.4f ms_tick=%.6f p50=%.6f p95=%.6f cpu_cores=%.3f births=%llu deaths=%llu\n",
        times.size(), wallMs, wallMs / times.size(), times[times.size()/2], times[(times.size()-1)*95/100], cpuMs / wallMs,
        after.births - before.births, after.deaths - before.deaths);
    PrintWorkers(field->GetNumThreads(), unsigned(times.size()));
    std::printf("SCHEDULER rebalances=%u\n", audit::rebalances);
    PrintSnapshot("END", Inspect(*field));
    // Pure scan and telemetry costs are measured outside the timed tick section.
    auto s0 = audit::Now();
    for (int repeatIndex = 0; repeatIndex < 20; ++repeatIndex)
    {
        uint64_t occupied = 0;
        for (int x = 0; x < FieldCellsWidth; ++x)
            for (int y = 0; y < FieldCellsHeight; ++y) occupied += field->GetObjectLocalCoords(x, y) != nullptr;
        scanSink = occupied;
    }
    const double scanMs = Milliseconds(audit::Now() - s0) / 20;
    s0 = audit::Now();
    for (int i = 0; i < 5; ++i) scanSink = uint64_t(field->ReadTelemetry(frame).values[vlsui::Bots]);
    std::printf("EXTRA scan_ms=%.6f telemetry_ms=%.6f\n", scanMs, Milliseconds(audit::Now() - s0)/5);
    std::fflush(stdout);
}
