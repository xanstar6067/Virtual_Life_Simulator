#include "EcosystemChart.h"
#include "UITheme.h"
#include "implot.h"
#include <chrono>
#include <cstdio>
#include <cstring>

namespace vlsui
{
namespace
{
const char* Names[MetricCount] = {"Боты", "Яблоки", "Органика", "Фотосинтетики", "Хищники", "Питаются органикой",
    "Смешанное питание", "Не определено", "Средний запас", "Мало энергии", "Рождения", "Гибель", "Средний возраст живых", "Охотившиеся боты"};
ImVec4 SeriesColor(int metric)
{
    switch (metric)
    {
    case Bots: case Photosynthetic: case Births: return Success();
    case Apples: return ImVec4(.94f, .77f, .34f, 1);
    case Organics: return ImVec4(.64f, .60f, .88f, 1);
    case Predators: case Deaths: case LowEnergy: case AnyPredators: return FeedingColor(FeedingType::Predator);
    case OrganicFeeders: case MeanEnergy: return FeedingColor(FeedingType::Organic);
    case MixedFeeders: return FeedingColor(FeedingType::Mixed);
    default: return Muted();
    }
}
bool Percent(int metric) { return (metric >= Photosynthetic && metric <= UnknownFeeders) || metric == LowEnergy; }
const char* Unit(int metric)
{
    return Percent(metric) ? "%" : metric == MeanEnergy ? "ед." : metric == AverageAge ? "тиков"
        : metric == Births || metric == Deaths ? "/ 100 тиков" : "шт.";
}
double WallSeconds()
{
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
}

EcosystemChart::EcosystemChart()
{
    visible.fill(true);
    visible[AnyPredators] = visible[AverageAge] = false;
}

void EcosystemChart::ClearChart()
{
    history.Clear(); events.clear(); initialized = false;
    pinnedTick = hoverTick = -1; autoFollow = true;
}

void EcosystemChart::AddEvent(double tick, const char* kind, const std::string& label)
{
    if (!events.empty() && events.back().kind == kind && events.back().kind != "Действие" && tick - events.back().tick < 25)
    {
        events.back().tick = tick;
        events.back().label = label;
    }
    else events.push_back({tick, kind, label});
}

void EcosystemChart::Observe(double tick, const TelemetryTotals& totals, const ChartContext& context,
    const std::function<TelemetrySnapshot()>& snapshot, bool force, const char* event)
{
    if (initialized && tick < previous.tick) ClearChart();
    const bool edited = initialized && (totals.added != previous.totals.added || totals.removed != previous.totals.removed);
    const bool changed = initialized && (context.photosynthesis != lastContext.photosynthesis || context.maxEnergy != lastContext.maxEnergy
        || context.season != lastContext.season || context.ocean != lastContext.ocean || context.mud != lastContext.mud);
    const bool due = !initialized || tick - previous.tick >= 25;
    if (!force && !due && ((!edited && !changed) || WallSeconds() < nextManualRefresh)) return;
    auto point = snapshot();
    if (initialized)
    {
        const double elapsed = point.tick - previous.tick;
        point.values[Births] = elapsed > 0 ? (point.totals.births - previous.totals.births) * 100.0 / elapsed : previous.values[Births];
        point.values[Deaths] = elapsed > 0 ? (point.totals.deaths - previous.totals.deaths) * 100.0 / elapsed : previous.values[Deaths];
        if (changed)
        {
            char label[160];
            if (context.photosynthesis != lastContext.photosynthesis)
            {
                snprintf(label, sizeof(label), "Фотосинтез: %d → %d", lastContext.photosynthesis, context.photosynthesis);
                AddEvent(tick, "ФС", label);
            }
            if (context.maxEnergy != lastContext.maxEnergy) AddEvent(tick, "Энергия", "Изменён максимальный запас энергии");
            if (context.season != lastContext.season)
            {
                const char* seasons[] = {"Лето", "Осень", "Зима", "Весна"};
                AddEvent(tick, "Сезон", context.season < 0 ? "Сезоны отключены" : seasons[context.season % 4]);
            }
            if (context.ocean != lastContext.ocean) AddEvent(tick, "Океан", "Изменён уровень океана");
            if (context.mud != lastContext.mud) AddEvent(tick, "Ил", "Изменён уровень ила / грязи");
        }
        if (edited && !event)
        {
            char label[160];
            snprintf(label, sizeof(label), "Ручное изменение: добавлено %llu, удалено %llu ботов",
                (unsigned long long)(point.totals.added - previous.totals.added), (unsigned long long)(point.totals.removed - previous.totals.removed));
            AddEvent(tick, "Правка", label);
        }
    }
    else point.values[Births] = point.values[Deaths] = std::numeric_limits<double>::quiet_NaN();
    history.Add(point);
    if (event) AddEvent(tick, "Действие", event);
    previous = point; lastContext = context; initialized = true;
    nextManualRefresh = WallSeconds() + .25;
}

std::pair<double, double> EcosystemChart::VisibleTicks() const
{
    if (history.samples.empty()) return {0, 1};
    const double last = history.samples.back().lastTick;
    const double span = period == 1 ? 5000 : period == 2 ? 10000 : period == 3 ? 50000 : last - history.samples.front().firstTick;
    const double first = (std::max)(history.samples.front().firstTick, last - span);
    return {first, (std::max)(first + 1, last)};
}

void EcosystemChart::RebuildCache()
{
    if (cachedRevision == history.revision && cachedPeriod == period) return;
    ticks.clear(); aggregated.clear();
    for (int m = 0; m < MetricCount; ++m) { values[m].clear(); minimum[m].clear(); maximum[m].clear(); }
    const auto range = VisibleTicks();
    auto start = std::lower_bound(history.samples.begin(), history.samples.end(), range.first,
        [](const TelemetrySample& point, double tick) { return point.lastTick < tick; });
    if (start != history.samples.begin()) --start;
    for (auto it = start; it != history.samples.end(); ++it)
    {
        ticks.push_back(it->Tick()); aggregated.push_back(it->weight > 1);
        for (int m = 0; m < MetricCount; ++m)
        {
            values[m].push_back(it->value[m]); minimum[m].push_back(it->minimum[m]); maximum[m].push_back(it->maximum[m]);
        }
    }
    cachedRevision = history.revision; cachedPeriod = period;
}

double EcosystemChart::Interpolate(int metric, double tick) const
{
    if (ticks.empty()) return std::numeric_limits<double>::quiet_NaN();
    auto it = std::lower_bound(ticks.begin(), ticks.end(), tick);
    const size_t b = it == ticks.end() ? ticks.size() - 1 : it - ticks.begin();
    if (b == 0 || it == ticks.end() || *it == tick) return values[metric][b];
    const size_t a = b - 1;
    return values[metric][a] + (values[metric][b] - values[metric][a]) * (tick - ticks[a]) / (ticks[b] - ticks[a]);
}

void EcosystemChart::Tooltip(double tick, const std::vector<int>& metrics, bool aggregatedPoint, double eventRadius)
{
    auto bucket = std::lower_bound(ticks.begin(), ticks.end(), tick);
    size_t bucketIndex = bucket == ticks.end() ? ticks.size() - 1 : bucket - ticks.begin();
    if (bucketIndex > 0 && std::abs(ticks[bucketIndex - 1] - tick) < std::abs(ticks[bucketIndex] - tick)) --bucketIndex;
    ImGui::BeginTooltip();
    ImGui::Text("Тик %.0f", tick);
    if (aggregatedPoint) ImGui::TextDisabled("Укрупнённые замеры: средние значения");
    for (int m : metrics)
    {
        const double value = Interpolate(m, tick);
        ImGui::TextColored(SeriesColor(m), "%s", Names[m]); ImGui::SameLine();
        if (!std::isfinite(value)) ImGui::TextUnformatted("—");
        else if (Percent(m))
        {
            ImGui::Text("%.1f %%", value);
            if (m >= Photosynthetic && m <= UnknownFeeders) { ImGui::SameLine(); ImGui::TextDisabled("≈ %.0f ботов", value * Interpolate(Bots, tick) / 100.0); }
        }
        else ImGui::Text("%.*f %s", m == MeanEnergy || m == AverageAge || m == Births || m == Deaths ? 1 : 0, value, Unit(m));
        if (aggregatedPoint && std::isfinite(minimum[m][bucketIndex]))
            ImGui::TextDisabled("Диапазон: %.1f … %.1f %s", minimum[m][bucketIndex], maximum[m][bucketIndex], Unit(m));
    }
    if (showEvents)
        for (const auto& event : events)
            if (std::abs(event.tick - tick) <= eventRadius)
            {
                ImGui::Separator();
                ImGui::PushTextWrapPos(360.0f);
                ImGui::Text("Тик %.0f · %s", event.tick, event.label.c_str());
                ImGui::PopTextWrapPos();
            }
    ImGui::EndTooltip();
}

void EcosystemChart::PlotPanel(const char* id, const char* yLabel, const std::vector<int>& metrics,
    const ImVec2& size, bool percent, bool stacked)
{
    using namespace ImPlot;
    const auto range = VisibleTicks();
    double low = DBL_MAX, high = -DBL_MAX;
    for (int m : metrics)
        for (size_t i = 0; i < ticks.size(); ++i)
            if (ticks[i] >= range.first && std::isfinite(minimum[m][i]))
            { low = (std::min)(low, minimum[m][i]); high = (std::max)(high, maximum[m][i]); }
    if (low == DBL_MAX) { low = 0; high = 1; }
    const double padding = (std::max)((high - low) * .12, 1.0);
    ImVec2 plotSize = size;
    const bool compact = size.y < 190;
    if (compact)
    {
        ImGui::TextDisabled("%s", yLabel);
        plotSize.y -= ImGui::GetTextLineHeight() + ImGui::GetStyle().ItemSpacing.y;
    }
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    if (ImGui::GetIO().MouseWheel != 0 && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)
        && ImGui::IsMouseHoveringRect(origin, ImVec2(origin.x + ImGui::GetContentRegionAvail().x, origin.y + plotSize.y))) autoFollow = false;
    if (BeginPlot(id, plotSize, ImPlotFlags_NoLegend | ImPlotFlags_NoTitle | ImPlotFlags_NoMouseText))
    {
        const bool upperPanel = compact && ((mode == 2 && metrics.size() == 1 && metrics[0] == MeanEnergy && visible[LowEnergy])
            || (mode == 0 && showAge && std::strcmp(id, "##AgeHistory") != 0));
        SetupAxis(ImAxis_X1, upperPanel ? nullptr : "Время, тики", upperPanel ? ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoTickMarks : ImPlotAxisFlags_None);
        SetupAxis(ImAxis_Y1, compact ? nullptr : yLabel);
        SetupAxisFormat(ImAxis_X1, "%.0f");
        SetupAxisLimits(ImAxis_X1, range.first, range.second, autoFollow ? ImPlotCond_Always : ImPlotCond_Once);
        SetupAxisLimits(ImAxis_Y1, percent ? 0 : detailScale ? (std::max)(0.0, low - padding) : 0,
            percent ? 100 : high + padding, autoFollow ? ImPlotCond_Always : ImPlotCond_Once);
        if (compact && autoFollow)
        {
            SetupAxisFormat(ImAxis_Y1, "%.0f");
            SetupAxisTicks(ImAxis_Y1, percent ? 0 : detailScale ? (std::max)(0.0, low - padding) : 0,
                percent ? 100 : high + padding, plotSize.y < 125 ? 2 : 3);
        }
        std::vector<double> lower(ticks.size(), 0), upper(ticks.size(), 0);
        // The bundled shaded renderer writes a NaN intersection vertex for
        // parallel boundaries. These ordered bands only need finite trapezoids.
        auto shade = [&](const std::vector<double>& bottom, const std::vector<double>& top, int metric, float alpha) {
            auto* draw = GetPlotDrawList();
            const ImDrawListFlags flags = draw->Flags;
            draw->Flags &= ~ImDrawListFlags_AntiAliasedFill;
            ImVec4 color = SeriesColor(metric); color.w = alpha;
            PushPlotClipRect();
            for (size_t i = 1; i < ticks.size(); ++i)
                if (std::isfinite(bottom[i - 1]) && std::isfinite(bottom[i]) && std::isfinite(top[i - 1]) && std::isfinite(top[i]))
                    if (top[i - 1] > bottom[i - 1] || top[i] > bottom[i])
                        draw->AddQuadFilled(PlotToPixels(ticks[i - 1], bottom[i - 1]), PlotToPixels(ticks[i], bottom[i]),
                            PlotToPixels(ticks[i], top[i]), PlotToPixels(ticks[i - 1], top[i - 1]), ImGui::ColorConvertFloat4ToU32(color));
            PopPlotClipRect();
            draw->Flags = flags;
        };
        // Submit finite line runs so extinction is a gap, not zero energy.
        auto finiteRuns = [&](const std::vector<double>& data, const auto& render) {
            size_t begin = 0;
            while (begin < data.size())
            {
                while (begin < data.size() && !std::isfinite(data[begin])) ++begin;
                size_t end = begin;
                while (end < data.size() && std::isfinite(data[end])) ++end;
                if (end > begin) render((int)begin, (int)(end - begin));
                begin = end;
            }
        };
        for (int m : metrics)
        {
            if (stacked)
            {
                for (size_t i = 0; i < ticks.size(); ++i) upper[i] = lower[i] + values[m][i];
                shade(lower, upper, m, .42f);
                lower = upper;
            }
            else
            {
                if (std::any_of(aggregated.begin(), aggregated.end(), [](bool old) { return old; }))
                {
                    shade(minimum[m], maximum[m], m, .12f);
                }
                finiteRuns(values[m], [&](int begin, int count) {
                    SetNextLineStyle(SeriesColor(m), 2.0f);
                    SetNextMarkerStyle(count == 1 ? ImPlotMarker_Circle : ImPlotMarker_None, 3.0f);
                    PlotLine(Names[m], ticks.data() + begin, values[m].data() + begin, count);
                });
            }
        }
        const ImVec2 pos = GetPlotPos(), extent = GetPlotSize();
        ImDrawList* draw = GetPlotDrawList();
        PushPlotClipRect();
        if (showEvents)
        {
            float lastLabel = -1000;
            for (const auto& event : events)
            {
                if (event.tick < GetPlotLimits().X.Min || event.tick > GetPlotLimits().X.Max) continue;
                const float x = PlotToPixels(event.tick, 0).x;
                const ImU32 color = ImGui::GetColorU32(ImVec4(.70f, .77f, .77f, .55f));
                for (float y = pos.y; y < pos.y + extent.y; y += 8) draw->AddLine(ImVec2(x, y), ImVec2(x, (std::min)(y + 4, pos.y + extent.y)), color);
                draw->AddCircleFilled(ImVec2(x, pos.y + 5), 3, color);
                if (extent.x > 500 && x - lastLabel > 100 && x + ImGui::CalcTextSize(event.kind.c_str()).x < pos.x + extent.x)
                { draw->AddText(ImVec2(x + 5, pos.y + 6), ImGui::GetColorU32(Muted()), event.kind.c_str()); lastLabel = x; }
            }
        }
        const bool hovered = IsPlotHovered();
        if (hovered && (ImGui::GetIO().MouseWheel != 0 || ImGui::IsMouseDragging(0) || ImGui::IsMouseDragging(1))) autoFollow = false;
        if (hovered && ImGui::IsMouseClicked(0)) pinnedTick = pinnedTick < 0 ? GetPlotMousePos().x : -1;
        if (hovered || pinnedTick >= 0)
        {
            const double tick = pinnedTick >= 0 ? pinnedTick : GetPlotMousePos().x;
            const float x = PlotToPixels(tick, 0).x;
            draw->AddLine(ImVec2(x, pos.y), ImVec2(x, pos.y + extent.y), ImGui::GetColorU32(Muted()));
            double cumulative = 0;
            for (int m : metrics)
            {
                const double value = Interpolate(m, tick); cumulative += value;
                if (std::isfinite(value)) draw->AddCircleFilled(PlotToPixels(tick, stacked ? cumulative : value), 3.5f, ImGui::ColorConvertFloat4ToU32(SeriesColor(m)));
            }
            PopPlotClipRect();
            if (hovered || (pinnedTick >= 0 && !ImGui::IsAnyItemHovered()))
            {
                auto it = std::lower_bound(ticks.begin(), ticks.end(), tick);
                const size_t index = it == ticks.end() ? ticks.size() - 1 : it - ticks.begin();
                hoverTick = tick;
                hoverAggregated = !aggregated.empty() && aggregated[index];
                eventTolerance = (std::max)(25.0, GetPlotLimits().X.Size() * 7.0 / extent.x);
            }
        }
        else PopPlotClipRect();
        EndPlot();
    }
}

void EcosystemChart::Plot()
{
    RebuildCache();
    hoverTick = -1;
    const float width = ImGui::GetContentRegionAvail().x;
    const int columns = width >= 680 ? 3 : 2;
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4, 3));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 3));
    if (ImGui::BeginTable("##ChartControls", columns, ImGuiTableFlags_SizingStretchSame))
    {
        ImGui::TableNextColumn(); ImGui::TextDisabled("Режим"); ImGui::SetNextItemWidth(-1);
        if (ImGui::Combo("##ChartMode", &mode, ChartModeNames, 4)) { autoFollow = true; pinnedTick = -1; }
        ImGui::TableNextColumn(); ImGui::TextDisabled("Период"); ImGui::SetNextItemWidth(-1);
        const char* periods[] = {"Вся история", "Последние 5 000 тиков", "Последние 10 000 тиков", "Последние 50 000 тиков"};
        if (ImGui::Combo("##ChartPeriod", &period, periods, 4)) { autoFollow = true; pinnedTick = -1; RebuildCache(); }
        if (columns == 3 && mode != 1)
        {
            ImGui::TableNextColumn(); ImGui::TextDisabled("Шкала");
            if (ImGui::Checkbox("Показать колебания", &detailScale)) autoFollow = true;
            ItemTooltip("Приблизить шкалу к значениям. От нуля — для общего размера; колебания — для небольших изменений.");
        }
        ImGui::EndTable();
    }
    ImGui::PopStyleVar(2);
    std::vector<int> metrics;
    if (mode == 0) metrics = {Bots, Apples, Organics, AnyPredators};
    if (mode == 1) metrics = {Photosynthetic, Predators, OrganicFeeders, MixedFeeders, UnknownFeeders};
    if (mode == 2) metrics = {MeanEnergy, LowEnergy};
    if (mode == 3) metrics = {Births, Deaths};
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(9, 4));
    float used = 0;
    for (int m : metrics)
    {
        const float itemWidth = ImGui::CalcTextSize(Names[m]).x + 32;
        if (used > 0 && used + itemWidth < width) ImGui::SameLine(); else used = 0;
        ImGui::PushStyleColor(ImGuiCol_CheckMark, SeriesColor(m));
        if (ImGui::Checkbox(Names[m], &visible[m])) autoFollow = true;
        ImGui::PopStyleColor(); used += itemWidth + 9;
        if (m == AnyPredators) ItemTooltip("Как в прежнем графике CB3: все боты, уже получавшие энергию от охоты, включая смешанное питание.");
        if (m == LowEnergy) ItemTooltip("Доля живых ботов с запасом энергии ниже 20% установленного максимума.");
    }
    if (mode == 0)
    {
        if (used + 165 < width) ImGui::SameLine();
        ImGui::Checkbox("Возраст живых", &showAge);
        ItemTooltip("Отдельная шкала в тиках. Средний возраст живых не равен продолжительности жизни погибших.");
    }
    ImGui::Checkbox("События", &showEvents); ImGui::SameLine();
    if (ImGui::Button("Вписать")) { autoFollow = true; pinnedTick = -1; }
    ImGui::SameLine();
    if (ImGui::Button("Очистить историю")) { ClearChart(); RebuildCache(); }
    ItemTooltip("Очистить только наблюдения. Мир и время симуляции сохраняются.");
    if (columns == 2 && mode != 1)
    {
        const float right = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
        const float nextWidth = ImGui::CalcTextSize("Показать колебания").x + ImGui::GetFrameHeight() + ImGui::GetStyle().ItemInnerSpacing.x;
        if (ImGui::GetItemRectMax().x + ImGui::GetStyle().ItemSpacing.x + nextWidth < right) ImGui::SameLine();
        if (ImGui::Checkbox("Показать колебания", &detailScale)) autoFollow = true;
    }
    ImGui::PopStyleVar(2);
    if (history.samples.empty()) { ImGui::TextWrapped("История появится при следующем замере."); return; }
    if (!autoFollow) ImGui::TextDisabled("Ручной масштаб · «Вписать» возвращает слежение за последними тиками");
    else if (std::any_of(aggregated.begin(), aggregated.end(), [](bool old) { return old; })) ImGui::TextWrapped("Старые замеры укрупнены · диапазон сохраняет минимумы и максимумы");
    else if (history.samples.size() == 1) ImGui::TextDisabled("Первый замер · новые точки каждые 25 тиков симуляции");
    else ImGui::TextDisabled("Наведение — значения · щелчок — закрепить · колесо / перетаскивание — масштаб");
    if ((mode == 1 || mode == 2) && history.samples.back().value[Bots] == 0) ImGui::TextDisabled("Живых ботов нет — показатель не определяется");
    std::vector<int> active;
    for (int m : metrics) if (visible[m]) active.push_back(m);
    const float available = (std::max)(130.0f, ImGui::GetContentRegionAvail().y);
    if (mode == 2)
    {
        const int panels = (int)visible[MeanEnergy] + (int)visible[LowEnergy];
        const float height = (std::max)(115.0f, (available - (panels - 1) * ImGui::GetStyle().ItemSpacing.y) / (std::max)(1, panels));
        if (visible[MeanEnergy]) PlotPanel("##EnergyHistory", "Энергия, ед.", {MeanEnergy}, ImVec2(-1, height));
        if (visible[LowEnergy]) PlotPanel("##LowEnergyHistory", "Мало энергии, %", {LowEnergy}, ImVec2(-1, height), true);
    }
    else
    {
        const bool age = mode == 0 && showAge;
        const float height = age ? (available - ImGui::GetStyle().ItemSpacing.y) * .5f : available;
        PlotPanel("##EcosystemHistory", mode == 1 ? "Доля живых ботов, %" : mode == 3 ? "Событий / 100 тиков" : "Количество",
            active, ImVec2(-1, height), mode == 1, mode == 1);
        if (age) PlotPanel("##AgeHistory", "Возраст живых, тики", {AverageAge}, ImVec2(-1, height));
    }
    if (mode == 0 && showAge) active.push_back(AverageAge);
    if (hoverTick >= 0) Tooltip(hoverTick, active, hoverAggregated, eventTolerance);
}

void EcosystemChart::DrawWindow(bool& open)
{
    if (!open) return;
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    ImGui::SetNextWindowSizeConstraints(ImVec2((std::min)(480.0f, display.x - 24), (std::min)(360.0f, display.y - 24)), ImVec2(display.x - 24, display.y - 24));
    ImGui::SetNextWindowSize(ImVec2((std::min)(960.0f, display.x - 24), (std::min)(700.0f, display.y - 24)), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(display.x * .5f, display.y * .5f), ImGuiCond_FirstUseEver, ImVec2(.5f, .5f));
    if (lastDisplay.x > 0 && (lastDisplay.x != display.x || lastDisplay.y != display.y))
    {
        const ImVec2 size((std::min)(windowSize.x, display.x - 24), (std::min)(windowSize.y, display.y - 24));
        ImGui::SetNextWindowSize(size);
        ImGui::SetNextWindowPos(ImVec2(std::clamp(windowPosition.x, 0.0f, display.x - size.x), std::clamp(windowPosition.y, 0.0f, display.y - size.y)));
    }
    const bool expanded = ImGui::Begin(ChartWindowTitle, &open, ImGuiWindowFlags_NoCollapse);
    windowPosition = ImGui::GetWindowPos(); windowSize = ImGui::GetWindowSize(); lastDisplay = display;
    if (expanded) Plot();
    ImGui::End();
}

void EcosystemChart::PlotPreview(const ImVec2& size, bool& open)
{
    RebuildCache();
    const float width = size.x <= 0 ? ImGui::GetContentRegionAvail().x : size.x;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 3));
    const char* shortNames[] = {"Популяция", "Питание", "Энергия", "Рождения / гибель"};
    ImGui::SetNextItemWidth((std::max)(90.0f, (std::min)(180.0f, width - 80)));
    if (ImGui::Combo("##PreviewMetric", &mode, shortNames, 4)) { autoFollow = true; pinnedTick = -1; }
    ImGui::SameLine(); if (ImGui::SmallButton("Графики")) open = true;
    const int primary = mode == 0 ? Bots : mode == 1 ? Predators : mode == 2 ? MeanEnergy : Births;
    const double value = ticks.empty() ? std::numeric_limits<double>::quiet_NaN() : values[primary].back();
    const double delta = ticks.empty() ? 0 : value - Interpolate(primary, VisibleTicks().first);
    if (!std::isfinite(value)) ImGui::TextDisabled("%s · нет данных", Names[primary]);
    else if (mode == 3) ImGui::Text("Рож. %.0f · гиб. %.0f / 100 т.", value, values[Deaths].back());
    else ImGui::Text("%s %.*f %s · %+.*f%s", Names[primary], Percent(primary) ? 1 : 0, value, Unit(primary), Percent(primary) ? 1 : 0, delta, Percent(primary) ? " п.п." : "");
    const ImVec2 start = ImGui::GetCursorScreenPos();
    const ImVec2 extent((std::max)(1.0f, width), 24);
    ImGui::InvisibleButton("##ChartPreview", extent);
    auto* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(start, ImVec2(start.x + extent.x, start.y + extent.y), ImGui::GetColorU32(ImGuiCol_FrameBg), 4);
    const auto range = VisibleTicks();
    std::vector<int> previewMetrics = mode == 0 ? std::vector<int>{Bots} : mode == 1 ? std::vector<int>{Photosynthetic, Predators, OrganicFeeders, MixedFeeders, UnknownFeeders}
        : mode == 2 ? std::vector<int>{MeanEnergy} : std::vector<int>{Births, Deaths};
    double lo = DBL_MAX, hi = -DBL_MAX;
    for (int m : previewMetrics) for (double v : values[m]) if (std::isfinite(v)) { lo = (std::min)(lo, v); hi = (std::max)(hi, v); }
    const double pad = lo == DBL_MAX ? 1 : (std::max)((hi - lo) * .15, 1.0);
    if (lo == DBL_MAX) { lo = 0; hi = 1; }
    if (mode == 1) { lo = 0; hi = 100; } else { lo = (std::max)(0.0, lo - pad); hi += pad; }
    auto point = [&](double tick, double v) { return ImVec2(start.x + 2 + (float)((tick - range.first) / (range.second - range.first)) * (extent.x - 4), start.y + extent.y - 2 - (float)((v - lo) / (hi - lo)) * (extent.y - 4)); };
    draw->PushClipRect(start, ImVec2(start.x + extent.x, start.y + extent.y), true);
    const ImDrawListFlags flags = draw->Flags;
    if (mode == 1) draw->Flags &= ~ImDrawListFlags_AntiAliasedFill;
    std::vector<double> lower(ticks.size(), 0);
    for (int m : previewMetrics)
    {
        for (size_t i = 0; i < ticks.size(); ++i)
        {
            const double v = mode == 1 ? lower[i] + values[m][i] : values[m][i];
            if (mode == 1 && i > 0 && std::isfinite(v) && std::isfinite(lower[i - 1] + values[m][i - 1]))
            {
                const ImVec2 a = point(ticks[i - 1], lower[i - 1]), b = point(ticks[i], lower[i]);
                const ImVec2 c = point(ticks[i], v), d = point(ticks[i - 1], lower[i - 1] + values[m][i - 1]);
                ImVec4 color = SeriesColor(m); color.w = .65f;
                draw->AddQuadFilled(a, b, c, d, ImGui::ColorConvertFloat4ToU32(color));
            }
            else if (std::isfinite(v))
            {
                if (i && std::isfinite(values[m][i - 1])) draw->AddLine(point(ticks[i - 1], values[m][i - 1]), point(ticks[i], v), ImGui::ColorConvertFloat4ToU32(SeriesColor(m)), 1.6f);
                else draw->AddCircleFilled(point(ticks[i], v), 2, ImGui::ColorConvertFloat4ToU32(SeriesColor(m)));
            }
        }
        if (mode == 1) for (size_t i = 0; i < ticks.size(); ++i) lower[i] += values[m][i];
    }
    draw->PopClipRect();
    draw->Flags = flags;
    if (ImGui::IsItemHovered() && !ticks.empty())
    {
        const double tick = range.first + std::clamp((double)(ImGui::GetIO().MousePos.x - start.x) / extent.x, 0.0, 1.0) * (range.second - range.first);
        Tooltip(tick, previewMetrics);
        if (ImGui::IsMouseDoubleClicked(0)) open = true;
    }
    ImGui::PopStyleVar(2);
}
}
