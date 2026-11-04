#pragma once

#include "SimulationTelemetry.h"
#include "imgui.h"
#include <functional>

namespace vlsui
{
inline constexpr const char* ChartWindowTitle = "Графики экосистемы###EcosystemCharts";
inline constexpr const char* ChartModeNames[] = {"Популяция и ресурсы", "Состав по питанию", "Энергия", "Рождения / гибель"};

class EcosystemChart
{
public:
    TelemetryHistory history;
    std::vector<ChartEvent> events;
    int mode = 0;
    int period = 0;
    bool detailScale = false, showEvents = true, showAge = false, autoFollow = true;
    std::array<bool, MetricCount> visible;

    EcosystemChart();
    void ClearChart();
    void Observe(double tick, const TelemetryTotals& totals, const ChartContext& context,
        const std::function<TelemetrySnapshot()>& snapshot, bool force = false, const char* event = nullptr);
    void AddEvent(double tick, const char* kind, const std::string& label);
    void DrawWindow(bool& open);
    void PlotPreview(const ImVec2& size, bool& open);
    void Plot();
    std::pair<double, double> VisibleTicks() const;

private:
    TelemetrySnapshot previous;
    ChartContext lastContext;
    bool initialized = false;
    double nextManualRefresh = 0;
    uint64_t cachedRevision = UINT64_MAX;
    int cachedPeriod = -1;
    std::vector<double> ticks;
    std::array<std::vector<double>, MetricCount> values, minimum, maximum;
    std::vector<bool> aggregated;
    double pinnedTick = -1, hoverTick = -1, eventTolerance = 25;
    bool hoverAggregated = false;
    ImVec2 lastDisplay{}, windowPosition{}, windowSize{};
    void RebuildCache();
    void PlotPanel(const char* id, const char* yLabel, const std::vector<int>& metrics,
        const ImVec2& size, bool percent = false, bool stacked = false);
    void Tooltip(double tick, const std::vector<int>& metrics, bool aggregatedPoint = false, double eventRadius = 25);
    double Interpolate(int metric, double tick) const;
};
}
