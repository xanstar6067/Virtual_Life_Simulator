#pragma once

#include <Windows.h>
#include <cstdint>

// Used only by generated benchmark copies of engine translation units.
namespace audit
{
inline int requestedThreads = 1;
inline bool freeze = false;
inline bool skipBrain = false;
inline bool fixedPartitions = false;
inline unsigned rebalances = 0;
// Set by --mode brain: receives each bot's genome, neuron memory and brain input right before thinking.
inline void (*recordBrain)(const void* genome, const int8_t* memory, const float* input) = nullptr;
inline thread_local int slot = 16;
inline thread_local bool sampling = false;
enum Stage { BotTick, Sense, Think, Multiply, Mutate, Rebuild, StageCount };
struct alignas(64) Worker
{
    uint64_t phase[2]{}, objects[2]{};
    uint64_t calls = 0, samples = 0;
    uint64_t stage[StageCount]{};
};
inline Worker workers[17]{};
inline uint64_t Now()
{
    LARGE_INTEGER value;
    QueryPerformanceCounter(&value);
    return static_cast<uint64_t>(value.QuadPart);
}
struct Scope
{
    Stage stage;
    uint64_t start;
    explicit Scope(Stage s) : stage(s), start(sampling ? Now() : 0) {}
    ~Scope() { if (start) workers[slot].stage[stage] += Now() - start; }
};
struct SampleBot
{
    uint64_t start = 0;
    SampleBot()
    {
        sampling = (++workers[slot].calls % 128) == 0;
        if (sampling) { ++workers[slot].samples; start = Now(); }
    }
    ~SampleBot()
    {
        if (start) workers[slot].stage[BotTick] += Now() - start;
        sampling = false;
    }
};
}
