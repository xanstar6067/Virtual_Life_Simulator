#pragma once

#include "BotDiet.h"
#include <array>
#include <atomic>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace vlsui
{
enum Metric { Bots, Apples, Organics, Photosynthetic, Predators, OrganicFeeders, MixedFeeders, UnknownFeeders,
    MeanEnergy, LowEnergy, Births, Deaths, AverageAge, AnyPredators, MetricCount };

struct TelemetryTotals
{
    uint64_t births = 0, deaths = 0, added = 0, removed = 0;
};

struct TelemetryCounters
{
    std::atomic<uint64_t> births{0}, deaths{0}, added{0}, removed{0};
    TelemetryTotals Read() const
    {
        return {births.load(std::memory_order_relaxed), deaths.load(std::memory_order_relaxed),
            added.load(std::memory_order_relaxed), removed.load(std::memory_order_relaxed)};
    }
};

struct TelemetrySnapshot
{
    double tick = 0;
    std::array<double, MetricCount> values{};
    TelemetryTotals totals;
};

// Called on the simulation thread only, after workers have completed a tick.
// Reads the actual cells rather than the engine's pre-tick display counters.
template<typename BotType, typename FieldType, typename Kind, typename ObjectKind>
TelemetrySnapshot ScanTelemetry(FieldType& field, int width, int height, int maxEnergy, double tick,
    Kind kind, ObjectKind botKind, ObjectKind appleKind, ObjectKind organicKind)
{
    TelemetrySnapshot sample;
    sample.tick = tick;
    double energy = 0, ages = 0, lowEnergy = 0;
    for (int x = 0; x < width; ++x)
        for (int y = 0; y < height; ++y)
            if (auto* obj = field.GetObjectLocalCoords(x, y))
            {
                const auto type = kind(obj);
                if (type == appleKind) ++sample.values[Apples];
                else if (type == organicKind) ++sample.values[Organics];
                else if (type == botKind)
                {
                    auto* organism = static_cast<BotType*>(obj);
                    ++sample.values[Bots];
                    energy += (std::max)(0, obj->energy);
                    ages += obj->GetLifetime();
                    lowEnergy += obj->energy < (std::max)(1, maxEnergy) * .2;
                    sample.values[AnyPredators] += organism->GetEnergyFromKills() > 0;
                    switch (ClassifyFeeding(organism->GetEnergyFromPS(), organism->GetEnergyFromKills(), organism->GetEnergyFromOrganics()))
                    {
                    case FeedingType::Photosynthetic: ++sample.values[Photosynthetic]; break;
                    case FeedingType::Predator: ++sample.values[Predators]; break;
                    case FeedingType::Organic: ++sample.values[OrganicFeeders]; break;
                    case FeedingType::Mixed: ++sample.values[MixedFeeders]; break;
                    default: ++sample.values[UnknownFeeders]; break;
                    }
                }
            }
    const double bots = sample.values[Bots];
    if (bots > 0)
    {
        for (int m = Photosynthetic; m <= UnknownFeeders; ++m) sample.values[m] *= 100.0 / bots;
        sample.values[MeanEnergy] = energy / bots;
        sample.values[LowEnergy] = lowEnergy * 100.0 / bots;
        sample.values[AverageAge] = ages / bots;
    }
    else
    {
        for (int m = Photosynthetic; m <= UnknownFeeders; ++m)
            sample.values[m] = std::numeric_limits<double>::quiet_NaN();
        sample.values[MeanEnergy] = sample.values[LowEnergy] = sample.values[AverageAge] = std::numeric_limits<double>::quiet_NaN();
    }
    return sample;
}

struct TelemetrySample
{
    double firstTick = 0, lastTick = 0;
    uint64_t weight = 1;
    std::array<uint64_t, MetricCount> validWeight{};
    std::array<double, MetricCount> value{}, minimum{}, maximum{};
    double Tick() const { return (firstTick + lastTick) * .5; }
};

class TelemetryHistory
{
public:
    static constexpr size_t Capacity = 8192;
    std::vector<TelemetrySample> samples;
    uint64_t revision = 0;

    void Clear() { samples.clear(); ++revision; }
    void Add(const TelemetrySnapshot& sample)
    {
        TelemetrySample point;
        point.firstTick = point.lastTick = sample.tick;
        point.value = point.minimum = point.maximum = sample.values;
        for (int m = 0; m < MetricCount; ++m) point.validWeight[m] = std::isfinite(point.value[m]) ? 1 : 0;
        if (!samples.empty() && sample.tick < samples.back().lastTick) Clear();
        if (!samples.empty() && sample.tick == samples.back().lastTick)
            samples.back() = point;
        else samples.push_back(point);
        if (samples.size() > Capacity)
        {
            // Compact only old data; the newest 4096+ observations remain exact.
            const size_t oldCount = Capacity / 2;
            // Retain the very first observation as an exact endpoint as well.
            for (size_t i = 0; i < oldCount; i += 2)
                samples[1 + i / 2] = Merge(samples[1 + i], samples[2 + i]);
            samples.erase(samples.begin() + 1 + oldCount / 2, samples.begin() + 1 + oldCount);
        }
        ++revision;
    }

private:
    static TelemetrySample Merge(const TelemetrySample& a, const TelemetrySample& b)
    {
        TelemetrySample merged;
        merged.firstTick = a.firstTick;
        merged.lastTick = b.lastTick;
        merged.weight = a.weight + b.weight;
        for (int m = 0; m < MetricCount; ++m)
        {
            const bool av = std::isfinite(a.value[m]), bv = std::isfinite(b.value[m]);
            merged.validWeight[m] = a.validWeight[m] + b.validWeight[m];
            merged.value[m] = av && bv ? (a.value[m] * a.validWeight[m] + b.value[m] * b.validWeight[m]) / merged.validWeight[m]
                : av ? a.value[m] : b.value[m];
            merged.minimum[m] = av && bv ? (std::min)(a.minimum[m], b.minimum[m]) : av ? a.minimum[m] : b.minimum[m];
            merged.maximum[m] = av && bv ? (std::max)(a.maximum[m], b.maximum[m]) : av ? a.maximum[m] : b.maximum[m];
        }
        return merged;
    }
};

struct ChartContext
{
    int photosynthesis = 0, maxEnergy = 0, season = -1, ocean = 0, mud = 0;
};

struct ChartEvent
{
    double tick;
    std::string kind, label;
};
}
