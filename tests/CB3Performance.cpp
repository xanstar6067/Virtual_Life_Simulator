// Headless performance research; no application Main, window, UI, or user writes.
#include "../source/cb3/Field.h"
#include "../source/cb3/SDL.h"
#include "CB3PerformanceHooks.h"
#include <psapi.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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
// Optional arena for Bot objects (--bot-layout sequential|shuffled), active only while a world is loaded.
// It isolates the cost of memory layout: the same bots in iteration order or in random memory order.
namespace layout
{
    constexpr size_t SlotSize = 256, SlotCount = size_t(1) << 20;
    static char* arena = nullptr;
    static std::vector<uint32_t> order;
    static size_t next = 0;
    static bool active = false;
    static bool Owns(void* p) { return arena && p >= arena && p < arena + SlotSize * SlotCount; }
    static void Init(bool shuffled)
    {
        arena = static_cast<char*>(VirtualAlloc(nullptr, SlotSize * SlotCount, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
        if (!arena) std::abort();
        order.resize(SlotCount);
        std::iota(order.begin(), order.end(), 0u);
        if (shuffled) { std::mt19937 rng(777); std::shuffle(order.begin(), order.end(), rng); }
        active = true;
    }
}
void* operator new(size_t n)
{
    if (layout::active && n == sizeof(cb3::Bot) && layout::next < layout::SlotCount)
        return layout::arena + size_t(layout::order[layout::next++]) * layout::SlotSize;
    if (void* p = std::malloc(n ? n : 1)) return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept { if (!layout::Owns(p)) std::free(p); }
void operator delete(void* p, size_t) noexcept { operator delete(p); }

// Dependent random loads over a buffer: average memory latency for a given working-set size.
static void MeasureLatency()
{
    for (size_t kib : {16, 128, 512, 2048, 8192, 32768, 262144, 1048576})
    {
        const size_t n = kib * 1024 / 64;
        std::vector<uint32_t> perm(n);
        std::iota(perm.begin(), perm.end(), 0u);
        std::mt19937 rng(1);
        std::shuffle(perm.begin() + 1, perm.end(), rng);
        auto* buf = static_cast<uint32_t*>(_aligned_malloc(n * 64, 64));
        if (!buf) std::abort();
        for (size_t i = 0; i < n; ++i) buf[size_t(perm[i]) * 16] = perm[(i + 1) % n];
        uint32_t at = 0;
        for (size_t i = 0; i < n; ++i) at = buf[size_t(at) * 16];
        const size_t loads = 20000000;
        const auto t0 = audit::Now();
        for (size_t i = 0; i < loads; ++i) at = buf[size_t(at) * 16];
        const double ns = Milliseconds(audit::Now() - t0) * 1e6 / loads;
        scanSink += at;
        std::printf("LATENCY kib=%zu ns=%.2f\n", kib, ns);
        _aligned_free(buf);
    }
}
// --mode brain: records every bot's genome, memory and input during one frozen tick, then evaluates only
// BrainGenome::Process + GetOutput for these records in processing order. The checksum covers all values,
// memory and outputs, so it must match between implementations that are bitwise equivalent.
namespace brainbench
{
    struct Record { const BrainGenome* genome; int8_t memory[NumNeuronLayers * NumNeuronsInLayerMax]; float input[NumInputNeurons]; };
    static std::vector<Record> records;
    static void Store(const void* genome, const int8_t* memory, const float* input)
    {
        Record r;
        r.genome = static_cast<const BrainGenome*>(genome);
        std::memcpy(r.memory, memory, sizeof(r.memory));
        std::memcpy(r.input, input, sizeof(r.input));
        records.push_back(r);
    }
}
static void MeasureBrain(Field& field, unsigned& frame, double targetMs, unsigned seed, bool byGenome)
{
    using namespace brainbench;
    records.reserve(1u << 20);
    audit::recordBrain = &Store;
    field.tick(frame++);
    audit::recordBrain = nullptr;
    // --brain-by-genome: same records grouped by genome, so the plan stays in cache (compute-bound reference).
    if (byGenome) std::stable_sort(records.begin(), records.end(), [](const Record& a, const Record& b) { return a.genome < b.genome; });
    // One pass evaluates every record in processing order; `full` also mixes all values, memory and outputs.
    const auto pass = [&](bool full)
    {
        srand(seed);
        uint64_t sum = 1469598103934665603ull;
        for (const auto& r : records)
        {
            int8_t memory[NumNeuronLayers][NumNeuronsInLayerMax];
            std::memcpy(memory, r.memory, sizeof(memory));
            BrainInput input;
            std::memcpy(input.fields, r.input, sizeof(input.fields));
            float values[NumNeuronLayers][NumNeuronsInLayerMax];
            r.genome->Process(values, memory, input);
            const BrainOutput out = brain::GetOutput(values);
            const auto mix = [&](const void* data, size_t bytes)
            {
                const auto* b = static_cast<const uint8_t*>(data);
                for (size_t i = 0; i < bytes; i += 8)
                {
                    uint64_t v = 0;
                    std::memcpy(&v, b + i, (std::min)(size_t(8), bytes - i));
                    sum = (sum ^ v) * 1099511628211ull;
                }
            };
            if (full) { mix(values, sizeof(values)); mix(memory, sizeof(memory)); }
            mix(out.fields, sizeof(out.fields));
        }
        return sum;
    };
    const uint64_t checksum = pass(true);
    std::vector<double> passes;
    uint64_t outputs = 0;
    const auto start = audit::Now();
    do
    {
        const auto t0 = audit::Now();
        const uint64_t sum = pass(false);
        passes.push_back(Milliseconds(audit::Now() - t0));
        if (passes.size() > 1 && sum != outputs) { std::fprintf(stderr, "Brain outputs differ between passes\n"); std::exit(5); }
        outputs = sum;
    } while (passes.size() < 3 || Milliseconds(audit::Now() - start) < targetMs);
    if (pass(true) != checksum) { std::fprintf(stderr, "Brain checksum differs between passes\n"); std::exit(5); }
    std::sort(passes.begin(), passes.end());
    std::printf("BRAIN records=%zu passes=%zu ns_per_call_p50=%.3f ns_per_call_min=%.3f checksum=%016llx\n",
        records.size(), passes.size(), passes[passes.size() / 2] * 1e6 / records.size(), passes[0] * 1e6 / records.size(),
        static_cast<unsigned long long>(checksum));
    std::fflush(stdout);
}
static void PrintSnapshot(const char* label, const Snapshot& s)
{
    std::printf("%s bots=%zu genomes=%zu apples=%zu organics=%zu rocks=%zu private_mib=%.3f working_mib=%.3f mean_connections=%.2f mean_active_neurons=%.2f plan_mib=%.3f neuron_mib=%.3f editable_mib=%.3f bot_pool_mib=%.3f\n",
        label, s.bots, s.genomes, s.apples, s.organics, s.rocks, s.privateMiB, s.workingMiB,
        s.genomes ? double(s.connections) / s.genomes : 0,
        s.genomes ? double(s.activeNeurons) / s.genomes : 0, s.planBytes / 1048576.0,
        s.neuronBytes / 1048576.0, s.editableBytes / 1048576.0, botpool::AllocatedBytes() / 1048576.0);
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
    std::wstring botLayout;
    unsigned age = 0;
    bool brainByGenome = false;
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
        else if (a == L"--bot-layout") botLayout = value();
        else if (a == L"--age") age = _wtoi(value());
        else if (a == L"--brain-by-genome") brainByGenome = true;
        else if (a == L"--fixed-partitions") audit::fixedPartitions = true;
        else { std::fprintf(stderr, "Unknown option\n"); return 2; }
    }
    if (count > FieldCellsWidth * FieldCellsHeight || audit::requestedThreads < 1 || audit::requestedThreads > NumThreads) return 2;
    LARGE_INTEGER f; QueryPerformanceFrequency(&f); frequency = double(f.QuadPart);
    if (mode == L"latency") { MeasureLatency(); return 0; }
    if (!botLayout.empty() && botLayout != L"sequential" && botLayout != L"shuffled") return 2;
    if (!botLayout.empty() && world.empty()) return 2;
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
    const bool freezeMode = mode == L"fixed" || mode == L"skipbrain" || mode == L"stunned" || mode == L"brain";
    if (mode == L"brain" && (world.empty() || audit::requestedThreads != 1)) return 2;
    // With --age a loaded world first runs real ticks, so births and deaths fragment memory naturally.
    audit::freeze = freezeMode && !age;
    audit::skipBrain = mode == L"skipbrain" && !age;
    srand(seed); Field::seed = seed;
    auto field = std::make_unique<Field>();
    PrintSnapshot("EMPTY", Inspect(*field));
    unsigned frame = 1;
    const uint64_t setupStart = audit::Now();
    if (!world.empty())
    {
        ObjectSaver saver;
        if (!botLayout.empty()) layout::Init(botLayout == L"shuffled");
        const auto loaded = saver.LoadWorld(field.get(), std::filesystem::path(world));
        layout::active = false;
        // Bot has its own pooled operator new, which the global replacement above cannot intercept.
        if (!botLayout.empty() && !layout::next)
        {
            std::fprintf(stderr, "--bot-layout needs sources without the bot pool (an archived harness)\n");
            return 4;
        }
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
    for (unsigned i = 0; i < age; ++i) field->tick(frame++);
    audit::freeze = freezeMode;
    audit::skipBrain = mode == L"skipbrain";
    if (audit::freeze) { field->params.botMaxLifetime = INT_MAX; field->params.spawnApples = false; field->params.useSeasons = false; }
    std::printf("SETUP ms=%.3f frame=%u\n", Milliseconds(audit::Now() - setupStart), frame);
    PrintSnapshot("START", Inspect(*field));
    if (mode == L"brain") { MeasureBrain(*field, frame, targetMs, seed, brainByGenome); return 0; }
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
