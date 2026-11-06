// Headless integration checks: renders the real UI into RAM using SDL software
// rendering. It never creates a desktop window, captures the screen, or sends input.
#ifdef TEST_CB3
#include "../source/cb3/Main.h"
#else
#include "../source/Main.h"
#endif
#include "../source/UITheme.h"
#include "../source/BrainWindow.h"
#include "../source/EcosystemChart.h"
#include "imgui_internal.h"
#include "implot_internal.h"
#include <SDL_image.h>

#include <cassert>
#include <crtdbg.h>
#include <cstdlib>
#include <cstdio>
#include <filesystem>

#undef main

#ifdef TEST_CB3
namespace cb3
{
#endif

struct InterfaceSmokeTest
{
    // Exercise AddObject on a real worker tick without depending on random brains.
    struct BirthProbe : Object
    {
        BirthProbe(int x, int y) : Object(x, y)
        {
#ifndef TEST_CB3
            type = abstract;
#endif
        }
        int tick() override
        {
            auto* child = new Bot(x + 1, y, 500);
            auto state = child->GetPersistentState();
            state.stunned = 1000;
            child->SetPersistentState(state);
            assert(pField->AddObject(child));
            return 1;
        }
    };

    static void CreateTextures()
    {
#ifdef TEST_CB3
        ImageFactory::CreateImages();
#else
        Apple::CreateImage();
        Bot::CreateImage();
        Organics::CreateImage();
#endif
    }

    static void Frame(Main& app, SDL_Surface* surface, float x = -100, float y = -100, bool down = false, int button = 0)
    {
        windowWidth = surface->w;
        windowHeight = surface->h;
        io->DisplaySize = ImVec2((float)windowWidth, (float)windowHeight);
        io->DeltaTime = 1.0f / 60.0f;
        io->MousePos = ImVec2(x, y);
        for (int i = 0; i < 3; ++i) io->MouseDown[i] = down && i == button;
        ImGui_ImplSDLRenderer_NewFrame();
        ImGui::NewFrame();
        SDL_SetRenderDrawColor(renderer, 19, 26, 29, 255);
        SDL_RenderClear(renderer);
        app.field->draw(app.renderType);
        app.DrawWindows();
        ImGui::Render();
        ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(renderer);
        const auto header = ImGui::FindWindowByName("##WorkspaceHeader");
        assert(header && !header->ScrollbarX && !header->ScrollbarY);
        assert(header->Size.y == 64 && header->ContentSize.y <= 36.0f);
        assert(ImGui::FindWindowByName("##WorkspaceInspector"));
        assert(ImGui::FindWindowByName("##WorkspaceTools"));
        const auto layout = vlsui::CalculateLayout(windowWidth, windowHeight);
        assert(layout.world.x == 10 && layout.tools.x > layout.inspector.x);
        const auto footer = ImGui::FindWindowByName("##WorkspaceFooter");
        if (footer && footer->ScrollbarY)
        {
            std::fprintf(stderr, "Footer overflow at %dx%d: content %.1f, available %.1f\n",
                windowWidth, windowHeight, footer->ContentSize.y, footer->InnerRect.GetHeight());
            IMG_SavePNG(surface, "footer-overflow.png");
        }
        assert(footer && !footer->ScrollbarX && !footer->ScrollbarY);
    }

    static void Click(Main& app, SDL_Surface* surface, float x, float y, int button = 0)
    {
        Frame(app, surface, x, y);
        Frame(app, surface, x, y, true, button);
        Frame(app, surface, x, y, false, button);
    }

    static unsigned CountBots(Main& app)
    {
        unsigned count = 0;
        for (int x = 0; x < FieldCellsWidth; ++x)
            for (int y = 0; y < FieldCellsHeight; ++y)
                if (auto obj = app.field->GetObjectLocalCoords(x, y))
                {
#ifdef TEST_CB3
                    count += obj->type() == bot;
#else
                    count += obj->type == bot;
#endif
                }
        return count;
    }

    static void CheckDisplayModes(Main& app, SDL_Surface* surface)
    {
        const auto footer = ImGui::FindWindowByName("##WorkspaceFooter");
        const auto layout = vlsui::CalculateLayout(windowWidth, windowHeight);
        const auto table = ImGui::TableFindByID(footer->GetID("##WorkspaceStats"));
        assert(table);
        float x = layout.compact
            ? footer->Pos.x + footer->WindowPadding.x + ImGui::CalcTextSize("Режим просмотра").x + ImGui::GetStyle().ItemSpacing.x
            : table->Columns[1].WorkMinX;
        const float y = layout.compact ? footer->Pos.y + footer->WindowPadding.y + 15.0f
            : table->OuterRect.Min.y + ImGui::GetTextLineHeight() + ImGui::GetStyle().ItemSpacing.y + 15.0f;
        const auto originalCount = CountBots(app);
        for (int mode = 0; mode < 4; ++mode)
        {
            const float width = ImGui::CalcTextSize(vlsui::DisplayModeNames[mode]).x + ImGui::GetStyle().FramePadding.x * 2.0f;
            assert(x + width <= footer->Pos.x + footer->Size.x - footer->WindowPadding.x);
            Click(app, surface, x + width * 0.5f, y);
            assert((int)app.renderType == mode);
            assert(CountBots(app) == originalCount);
            x += width + ImGui::GetStyle().ItemSpacing.x;
        }
        app.renderType = (RenderTypes)0;
        Frame(app, surface);
    }

    static void CheckFeeding(Main& app, SDL_Surface* surface)
    {
        const auto bot = (Bot*)app.selectedObject;
        const auto originalState = bot->GetPersistentState();
        struct Example { int ps, prey, organic; vlsui::FeedingType type; };
        using enum vlsui::FeedingType;
        for (const auto& example : {Example{0, 0, 0, Unknown}, {19, 0, 0, Unknown},
            {20, 0, 0, Photosynthetic}, {0, 50, 0, Predator}, {0, 0, 50, Organic},
            {100, 100, 100, Mixed}, {40, 20, 0, Photosynthetic}, {39, 21, 0, Mixed}})
        {
            auto state = originalState;
            state.energyFromPS = example.ps;
            state.energyFromPredation = example.prey;
            state.energyFromOrganics = example.organic;
            bot->SetPersistentState(state);
            Frame(app, surface);
            assert(bot->GetEnergyFromOrganics() == example.organic);
            assert(vlsui::ClassifyFeeding(bot->GetEnergyFromPS(), bot->GetEnergyFromKills(), bot->GetEnergyFromOrganics()) == example.type);
            if (example.type == Predator) assert(IMG_SavePNG(surface, "predator-inspector.png") == 0);
            if (example.type == Photosynthetic) assert(IMG_SavePNG(surface, "photosynthetic-inspector.png") == 0);
        }
        bot->SetPersistentState(originalState);
    }

#ifdef TEST_CB3
    static void CheckCb3Brain(Main& app)
    {
        BotNeuralNet brain;
        // A unit-weight input feeds every output, including sums outside -1..1.
        for (uint i = 0; i < NumOutputNeurons; ++i)
            brain.allNeurons[0][0].AddConnection(NeuronOutputLayerIndex, i, 50);
        for (int bias : {-50, 0, 50})
        {
            for (uint i = 0; i < NumOutputNeurons; ++i)
                brain.allNeurons[NeuronOutputLayerIndex][i].bias = bias;
            for (float sum : {-5.0f, -0.51f, -0.5f, -0.49f, 0.0f, 0.49f, 0.5f, 0.51f, 5.0f})
            {
                BrainInput input{};
                input.energy = sum;
                brain.Process(input);
                const auto output = brain.GetOutput();
                const float value = sum + bias * BiasMultiplier;
                for (uint i = 0; i < NumOutputNeurons; ++i)
                {
                    const int expected = i < 2
                        ? (value >= 0.5f ? 1 : value <= -0.5f ? -1 : 0)
                        : (value >= 0.5f ? 1 : 0);
                    assert(output.fields[i] == expected);
                    assert(brain.allValues[NeuronOutputLayerIndex][i] == expected);
                }
            }
        }

        // Exercise the actual lookup and turn for all eight headings and no turn.
        uint tick = 2000;
        for (int x = -1; x <= 1; ++x)
            for (int y = -1; y <= 1; ++y)
            {
                app.ClearWorld();
                auto* organism = new Bot(20, 20, 500);
                const auto active = organism->GetActiveBrain();
                for (uint layer = 0; layer < NumNeuronLayers; ++layer)
                    for (auto& neuron : active.allNeurons[layer])
                        neuron.SetZero();
                active.allNeurons[NeuronOutputLayerIndex][0].bias = x * 100;
                active.allNeurons[NeuronOutputLayerIndex][1].bias = y * 100;
                const int target = RotationsReverse[x + 1][y + 1];
                auto state = organism->GetPersistentState();
                state.direction = target < 0 ? 3 : (target + 1) % 8;
                state.stunned = 0;
                organism->SetPersistentState(state);
                assert(app.field->AddObject(organism));
                app.field->tick(tick++);
                assert(app.field->GetObjectLocalCoords(20, 20) == organism);
                assert(organism->GetDirection() == (target < 0 ? 3 : target));
                assert(organism->energy == 500 - EveryTickEnergyPenalty - (target < 0 ? 0 : RotateCost));
            }

        // The real Bot path uses the shared plan, exposes the same observed
        // values, and falls back while a direct editable view is outstanding.
        app.ClearWorld();
        auto* planned = new Bot(20, 20, 500);
        assert(app.field->AddObject(planned));
        Bot::SetObservedBot(planned);
        auto view = planned->GetActiveBrain();
        for (uint layer = 0; layer < NumNeuronLayers; ++layer)
            for (auto& neuron : view.allNeurons[layer]) neuron.SetZero();
        view.allNeurons[0][0].AddConnection(1, 0, 50);
        view.allNeurons[1][0].type = NeuronType::memory;
        view.allNeurons[1][0].AddConnection(NeuronOutputLayerIndex, 0, 50);
        view.allNeurons[0][1].AddConnection(1, 1, 50);
        view.allNeurons[1][1].type = NeuronType::random;
        view.allNeurons[1][1].AddConnection(NeuronOutputLayerIndex, 1, 50);
        planned->RecheckBrain();
        assert(planned->GetGenome().ExecutionStorageBytes() > 0);
        auto compareThink = [&](float energy) {
            NeuronValueRow values[NumNeuronLayers];
            NeuronMemoryRow memory[NumNeuronLayers];
            memcpy(memory, planned->GetBrainMemory(), sizeof(memory));
            BrainInput input{}; input.energy = energy; input.age = .5f;
            srand(1729);
            brain::Process(planned->GetGenome().active, values, memory, input);
            auto expected = brain::GetOutput(values);
            int expectedRandom = rand();
            srand(1729);
            auto actual = planned->think(input);
            assert(rand() == expectedRandom);
            assert(!memcmp(&expected, &actual, sizeof(expected)));
            assert(!memcmp(values, view.allValues, sizeof(values)));
            assert(!memcmp(memory, planned->GetBrainMemory(), sizeof(memory)));
        };
        for (float value : {1.01f, 0.0f, -1.01f, 1.0f}) compareThink(value);
        view = planned->GetActiveBrain();
        view.allNeurons[NeuronOutputLayerIndex][0].bias = -100;
        compareThink(0.0f); // Before RecheckBrain: stale plan must not be used.
        planned->RecheckBrain();
        compareThink(0.0f);
        Bot::SetObservedBot(nullptr);
        app.ClearWorld();
    }
#endif

    // Relatives share one brain genome until one of them changes (copy-on-write).
    static void CheckGenomeSharing(Main& app)
    {
        app.ClearWorld();
#ifdef TEST_CB3
        const bool noMutations = app.field->params.noMutations;
        app.field->params.noMutations = false;
#endif
        auto* parent = new Bot(20, 20, 500);
        const auto* genome = parent->GetGenomeRef().get();
        assert(genome->RefCount() == 1 && genome->activeIsOptimizedInitial);

        // A child without mutation only adds a reference to the parent's genome.
        auto* twin = new Bot(21, 20, 500, parent, false);
        assert(twin->GetGenomeRef().get() == genome && genome->RefCount() == 2);
        assert(!memcmp(twin->GetGenome().active, parent->GetGenome().active, sizeof(genome->active)));

        // A mutated child gets its own copy, the parent is not changed.
        std::remove_const_t<decltype(genome->initial)> parentInitial;
        memcpy(parentInitial, genome->initial, sizeof(parentInitial));
        auto* mutant = new Bot(22, 20, 500, parent, true);
        assert(mutant->GetGenomeRef().get() != genome && genome->RefCount() == 2);
        assert(!memcmp(parentInitial, genome->initial, sizeof(parentInitial)));
        assert(mutant->GetGenome().activeIsOptimizedInitial);

        // Editing one relative (brain window view) copies, the other keeps the brain.
        const Neuron parentNeuron = genome->active[1][0];
        const auto edited = twin->GetActiveBrain();
        assert(twin->GetGenomeRef().get() != genome && genome->RefCount() == 1);
        edited.allNeurons[1][0].SetZero();
        edited.allNeurons[1][0].bias = 77;
        assert(!memcmp(&parentNeuron, &genome->active[1][0], sizeof(Neuron)));
        twin->RecheckBrain();
        assert(!twin->GetGenome().activeIsOptimizedInitial);
        // Its child must not inherit the edited active brain, only the initial one.
        auto* grandchild = new Bot(23, 20, 500, twin, false);
        assert(grandchild->GetGenomeRef().get() != twin->GetGenomeRef().get());
        assert(grandchild->GetGenome().activeIsOptimizedInitial);

        // Global mutation of a sharing bot copies too.
        auto* sibling = new Bot(24, 20, 500, parent, false);
        assert(sibling->GetGenomeRef().get() == genome && genome->RefCount() == 2);
#ifdef TEST_CB3
        sibling->Mutagen();
#else
        // Classic Mutagen mutates the genes with a 10% chance.
        for (int i = 0; i < 1000 && sibling->GetGenomeRef().get() == genome; ++i)
            sibling->Mutagen();
#endif
        assert(sibling->GetGenomeRef().get() != genome && genome->RefCount() == 1);
        assert(!memcmp(parentInitial, genome->initial, sizeof(parentInitial)));

        // Memory stays per bot and is cleared for a newborn.
        const auto parentBrain = parent->GetActiveBrain();
        parentBrain.allMemory[1][0] = 1;
        auto* newborn = new Bot(25, 20, 500, parent, false);
        assert(newborn->GetBrainMemory()[1][0] == 0 && parent->GetBrainMemory()[1][0] == 1);

        for (Bot* b : {parent, twin, mutant, grandchild, sibling, newborn})
            delete b;
#ifdef TEST_CB3
        app.field->params.noMutations = noMutations;
#endif
    }

    static void CheckTelemetry(Main& app)
    {
        app.ClearWorld();
        assert(app.field->AddObject(new BirthProbe(0, 10)));
        const auto before = app.field->GetTelemetryTotals();
        app.field->tick(1000);
        const auto born = app.field->GetTelemetryTotals();
        assert(born.births == before.births + 1);
        assert(born.added == before.added);
        auto* child = app.field->GetObjectLocalCoords(1, 10);
        assert(child);
        app.field->RemoveObject(1, 10);
        const auto removed = app.field->GetTelemetryTotals();
        assert(removed.deaths == born.deaths && removed.removed == born.removed + 1);

        auto* dying = new Bot(10, 10, 500);
        auto dyingState = dying->GetPersistentState();
        dyingState.stunned = 0;
        dying->SetPersistentState(dyingState);
        dying->energy = 0;
        assert(app.field->AddObject(dying));
        app.field->tick(1001);
        assert(!app.field->GetObjectLocalCoords(10, 10));
        assert(app.field->GetTelemetryTotals().deaths == removed.deaths + 1);

        app.ClearWorld();
        for (int i = 0; i < 5; ++i)
        {
            auto* organism = new Bot(20 + i, 20, (i + 1) * 50);
            auto state = organism->GetPersistentState();
            state.energyFromPS = i == 0 || i == 3 ? 50 : 0;
            state.energyFromPredation = i == 1 || i == 3 ? 50 : 0;
            state.energyFromOrganics = i == 2 || i == 3 ? 50 : 0;
            organism->SetPersistentState(state);
            organism->SetLifetime(i * 10);
            assert(app.field->AddObject(organism));
        }
        const auto snapshot = app.field->ReadTelemetry(25);
        assert(snapshot.values[vlsui::Bots] == 5);
        for (int metric = vlsui::Photosynthetic; metric <= vlsui::UnknownFeeders; ++metric)
            assert(std::abs(snapshot.values[metric] - 20.0) < .001);
        assert(snapshot.values[vlsui::MeanEnergy] == 150);
        assert(snapshot.values[vlsui::AverageAge] == 20);
        assert(snapshot.values[vlsui::LowEnergy] == 20);
        app.ClearWorld();
        const auto empty = app.field->ReadTelemetry(0);
        assert(!std::isfinite(empty.values[vlsui::MeanEnergy]));
        assert(!std::isfinite(empty.values[vlsui::LowEnergy]));
        assert(!std::isfinite(empty.values[vlsui::Photosynthetic]));
        app.QuickLoadWorld();
    }

    static void CheckHistory()
    {
        vlsui::TelemetryHistory history;
        double expectedSum = 0, min = 1e30, max = -1e30;
        for (int i = 0; i < 25000; ++i)
        {
            vlsui::TelemetrySnapshot point;
            point.tick = i * 25;
            point.values[vlsui::Bots] = i == 7 ? 1000000 : 1000 + i % 300;
            expectedSum += point.values[vlsui::Bots];
            min = (std::min)(min, point.values[vlsui::Bots]); max = (std::max)(max, point.values[vlsui::Bots]);
            history.Add(point);
        }
        assert(history.samples.size() <= vlsui::TelemetryHistory::Capacity);
        assert(history.samples.front().firstTick == 0 && history.samples.back().lastTick == 24999 * 25);
        assert(history.samples.back().weight == 1);
        double actualSum = 0, actualMin = 1e30, actualMax = -1e30;
        uint64_t observations = 0;
        for (const auto& sample : history.samples)
        {
            actualSum += sample.value[vlsui::Bots] * sample.weight;
            actualMin = (std::min)(actualMin, sample.minimum[vlsui::Bots]); actualMax = (std::max)(actualMax, sample.maximum[vlsui::Bots]);
            observations += sample.weight;
        }
        assert(observations == 25000 && std::abs(actualSum - expectedSum) < .001);
        assert(actualMin == min && actualMax == max);

        vlsui::EcosystemChart chart;
        vlsui::TelemetrySnapshot point;
        point.tick = 100; point.totals = {10, 20, 50, 30};
        chart.Observe(point.tick, point.totals, {}, [&] { return point; }, true);
        point.tick = 125; point.totals = {15, 23, 57, 39};
        chart.Observe(point.tick, point.totals, {}, [&] { return point; }, true);
        const auto& latest = chart.history.samples.back();
        assert(latest.value[vlsui::Births] == 20 && latest.value[vlsui::Deaths] == 12);
        assert(!chart.events.empty());
        const size_t size = chart.history.samples.size();
        point.values[vlsui::Bots] = 500;
        chart.Observe(point.tick, point.totals, {}, [&] { return point; }, true);
        assert(chart.history.samples.size() == size && chart.history.samples.back().value[vlsui::Bots] == 500);
    }

    static void ChartFixture(Main& app)
    {
        app.chart.ClearChart();
        app.ticknum = 20000;
        app.RecordChartSample(true);
        app.chart.history.Clear();
        for (int i = 0; i <= 200; ++i)
        {
            vlsui::TelemetrySnapshot point;
            point.tick = i * 100;
            const double drop = std::exp(-std::pow((i - 125) / 18.0, 2));
            point.values[vlsui::Bots] = 80000 + i * 150 - 30000 * drop;
            point.values[vlsui::Apples] = 5000 + 1500 * std::sin(i / 10.0);
            point.values[vlsui::Organics] = 2000 + 15000 * drop;
            point.values[vlsui::Photosynthetic] = 65 - i * .12;
            point.values[vlsui::Predators] = 10 + i * .08;
            point.values[vlsui::OrganicFeeders] = 8;
            point.values[vlsui::MixedFeeders] = 15 + i * .04;
            point.values[vlsui::UnknownFeeders] = 2;
            point.values[vlsui::MeanEnergy] = 350 - 220 * drop;
            point.values[vlsui::LowEnergy] = 2 + 45 * drop;
            point.values[vlsui::AverageAge] = 60 + 10 * std::sin(i / 15.0);
            point.values[vlsui::Births] = 200 + 60 * std::sin(i / 12.0);
            point.values[vlsui::Deaths] = 180 + 160 * drop;
            point.values[vlsui::AnyPredators] = point.values[vlsui::Bots] * .3;
            app.chart.history.Add(point);
        }
        app.chart.AddEvent(10000, "ФС", "Фотосинтез: 8 → 3");
    }

    static void CheckCharts(Main& app, SDL_Surface* surface, const char* prefix)
    {
        const auto originalTick = app.ticknum;
        ChartFixture(app);
        app.showChart = true;
        Frame(app, surface); Frame(app, surface);
        // Click the real mode and period selectors in the chart window.
        auto select = [&](int column, int choice) {
            auto* window = ImGui::FindWindowByName(vlsui::ChartWindowTitle);
            const auto* table = ImGui::TableFindByID(window->GetID("##ChartControls"));
            const float x = (table->Columns[column].WorkMinX + table->Columns[column].WorkMaxX) * .5f;
            const float y = table->OuterRect.Min.y + table->CellPaddingY + ImGui::GetTextLineHeight() + ImGui::GetStyle().ItemSpacing.y + 11;
            Click(app, surface, x, y);
            Frame(app, surface, x, y); // Let ImGui finish measuring the popup before reading its position.
            const auto* popup = ImGui::FindWindowByName("##Combo_00");
            assert(popup && popup->Active);
            const float row = ImGui::GetTextLineHeightWithSpacing();
            Click(app, surface, popup->Pos.x + 80, popup->Pos.y + popup->WindowPadding.y + row * choice + ImGui::GetTextLineHeight() * .5f);
        };
        select(0, 2);
        assert(app.chart.mode == 2);
        select(1, 1);
        assert(app.chart.period == 1);
        app.chart.period = 0;
        for (int mode = 0; mode < 4; ++mode)
        {
            app.chart.mode = mode;
            app.chart.autoFollow = true;
            Frame(app, surface); Frame(app, surface);
            auto* window = ImGui::FindWindowByName(vlsui::ChartWindowTitle);
            assert(window && window->Active && !(window->Flags & ImGuiWindowFlags_NoMove));
            if (window->ScrollbarY) std::fprintf(stderr, "Chart overflow %s mode %d content %.1f available %.1f\n", prefix, mode, window->ContentSize.y, window->InnerRect.GetHeight());
            char imageName[100]; snprintf(imageName, sizeof(imageName), "%s-chart-%d.png", prefix, mode);
            assert(IMG_SavePNG(surface, imageName) == 0);
            assert(!window->ScrollbarX && !window->ScrollbarY);
            const char* plotId = mode == 2 ? "##EnergyHistory" : "##EcosystemHistory";
            auto* plot = ImPlot::GetCurrentContext()->Plots.GetByKey(window->GetID(plotId));
            assert(plot);
            const ImVec2 hover = plot->PlotRect.GetCenter();
            Frame(app, surface, hover.x, hover.y);
            bool tooltip = false;
            for (const auto* candidate : GImGui->Windows)
                tooltip |= candidate->Active && (candidate->Flags & ImGuiWindowFlags_Tooltip);
            assert(tooltip);
            assert(plot->Axes[ImAxis_X1].Range.Max == 20000);
            app.chart.period = 1;
            Frame(app, surface);
            assert(app.chart.VisibleTicks().first == 15000);
            app.chart.period = 0;
        }
        app.chart.mode = 0;
        app.chart.showAge = true;
        Frame(app, surface); Frame(app, surface);
        assert(!ImGui::FindWindowByName(vlsui::ChartWindowTitle)->ScrollbarY);
        app.chart.showAge = false;
        // Undefined energy and composition after extinction must never reach
        // the SDL renderer as NaN geometry, and must not become zero readings.
        auto emptyPoint = app.field->ReadTelemetry(app.ticknum);
        emptyPoint.tick = 20025;
        emptyPoint.values[vlsui::Bots] = 0;
        for (int metric = vlsui::Photosynthetic; metric <= vlsui::LowEnergy; ++metric)
            emptyPoint.values[metric] = std::numeric_limits<double>::quiet_NaN();
        app.chart.history.Add(emptyPoint);
        for (int mode : {1, 2})
        {
            app.chart.mode = mode;
            Frame(app, surface);
            for (int list = 0; list < ImGui::GetDrawData()->CmdListsCount; ++list)
                for (const auto& vertex : ImGui::GetDrawData()->CmdLists[list]->VtxBuffer)
                    assert(std::isfinite(vertex.pos.x) && std::isfinite(vertex.pos.y));
        }
        app.showChart = false;
        app.chart.mode = 0;
        app.ticknum = originalTick;
        app.chart.ClearChart();
        app.RecordChartSample(true);
        Frame(app, surface);
    }

    static void ClickBrainEdit(Main& app, SDL_Surface* surface, bool randomize, bool mutate = false)
    {
        Frame(app, surface); // Refresh table geometry after a connection count changes.
        const auto parent = ImGui::FindWindowByName(vlsui::BrainWindowTitle);
        ImGuiWindow* details = nullptr;
        for (auto child : parent->DC.ChildWindows)
            if (strstr(child->Name, "##NeuronDetails")) details = child;
        assert(details);
        const auto table = ImGui::TableFindByID(details->GetID("##NeuronConnections"));
        assert(table);
        const float x = details->WorkRect.Min.x + details->WorkRect.GetWidth() * (mutate ? .5f : randomize ? .25f : .75f);
        const float y = table->OuterRect.Max.y + ImGui::GetStyle().ItemSpacing.y + 15.0f
            + (mutate ? 30.0f + ImGui::GetStyle().ItemSpacing.y : 0.0f);
        Click(app, surface, x, y);
    }

    static void CheckBrainEdits(Main& app, SDL_Surface* surface)
    {
        const auto bot = (Bot*)app.selectedObject;
        auto& history = app.nn_renderer.history;
        assert(history.Empty());
        const auto window = ImGui::FindWindowByName(vlsui::BrainWindowTitle);
        const float x = window->Pos.x + window->WindowPadding.x;
        const float y = window->Pos.y + window->TitleBarHeight() + window->WindowPadding.y
            + 28.0f + ImGui::GetStyle().ItemSpacing.y + 14.0f;
        auto undo = [&] { Click(app, surface, x + 46, y); };
        auto redo = [&] { Click(app, surface, x + 92 + ImGui::GetStyle().ItemSpacing.x + 46, y); };
        auto select = [&](int index) {
            const auto point = app.nn_renderer.view.NeuronPosition(1, index);
            Click(app, surface, point.x, point.y);
            assert(app.nn_renderer.selectedNeuron);
        };
        auto roll = [&] {
            const int index = app.nn_renderer.view.selectedIndex;
            const auto count = history.Position(app.brainToShow, 1, index);
            for (int attempt = 0; attempt < 20 && history.Position(app.brainToShow, 1, index) == count; ++attempt)
                ClickBrainEdit(app, surface, true);
            assert(history.Position(app.brainToShow, 1, index) == count + 1);
        };

        // Three edits of A, followed by two edits of B, remain independent.
        roll();
        const Neuron a1 = bot->GetGenome().active[1][0];
        roll();
        const Neuron a2 = bot->GetGenome().active[1][0];
        roll();
        const Neuron a3 = bot->GetGenome().active[1][0];
        assert(history.Size(0, 1, 0) == 3);
        select(3);
        assert(!history.CanUndo(0, 1, 3));
        roll();
        const Neuron b1 = bot->GetGenome().active[1][3];
        roll();
        BrainGenome bothEdited(bot->GetGenome());
        assert(history.Size(0, 1, 3) == 2);
        select(0);
        auto active = bot->GetActiveBrain();
        active.allMemory[1][0] = 1;
        undo();
        undo();
        BrainGenome expected(bothEdited);
        expected.active[1][0] = a1;
        assert(expected.SameNeurons(bot->GetGenome()));
        assert(bot->GetBrainMemory()[1][0] == 1);
        redo();
        redo();
        assert(bothEdited.SameNeurons(bot->GetGenome()));

        // Branching B's history must preserve A's redo branch.
        undo();
        select(3);
        undo();
        expected.active[1][0] = a2;
        expected.active[1][3] = b1;
        assert(expected.SameNeurons(bot->GetGenome()));
        roll();
        assert(!history.CanRedo(0, 1, 3) && history.CanRedo(0, 1, 0));
        BrainGenome branched(bot->GetGenome());
        select(0);
        redo();
        branched.active[1][0] = a3;
        assert(branched.SameNeurons(bot->GetGenome()));

        // Clear and repeated no-op clear affect only A's timeline.
        const auto beforeClear = history.Size(0, 1, 0);
        ClickBrainEdit(app, surface, false);
        const auto afterClear = history.Size(0, 1, 0);
        BrainGenome cleared(bot->GetGenome());
        ClickBrainEdit(app, surface, false);
        assert(history.Size(0, 1, 0) == afterClear);
        if (afterClear > beforeClear)
        {
            undo();
            assert(branched.SameNeurons(bot->GetGenome()));
            redo();
            assert(cleared.SameNeurons(bot->GetGenome()));
        }

        // The same neuron in the initial brain has a separate timeline.
        const float switchY = window->Pos.y + window->TitleBarHeight() + window->WindowPadding.y + 14;
        Click(app, surface, x + 146, switchY);
        assert(!app.nn_renderer.selectedNeuron);
        // Disabled controls cannot apply the previous version's selection.
        undo();
        assert(cleared.SameNeurons(bot->GetGenome()));
        select(0);
        assert(!history.CanUndo(1, 1, 0));
        roll();
        BrainGenome initialEdited(bot->GetGenome());
        undo();
        assert(app.brainToShow == 1 && cleared.SameNeurons(bot->GetGenome()));
        redo();
        assert(initialEdited.SameNeurons(bot->GetGenome()));
        Click(app, surface, x + 46, switchY);
        select(0);
        assert(history.Size(0, 1, 0) == afterClear);
        assert(history.Size(0, 1, 3) == 2 && history.Size(1, 1, 0) == 1);

#ifdef TEST_CB3
        // Mutation records each changed neuron, without erasing older histories.
        BrainGenome beforeMutation(bot->GetGenome());
        for (int attempt = 0; attempt < 20 && beforeMutation.SameNeurons(bot->GetGenome()); ++attempt)
            ClickBrainEdit(app, surface, false, true);
        BrainGenome mutated(bot->GetGenome());
        int changed = 0;
        for (int layer = 0; layer < (int)NumNeuronLayers; ++layer)
            for (int index = 0; index < (int)neuronsInLayer[layer]; ++index)
            {
                BrainGenome expectedUndo(mutated);
                expectedUndo.active[layer][index] = beforeMutation.active[layer][index];
                if (expectedUndo.SameNeurons(mutated)) continue;
                assert(history.CanUndo(0, layer, index));
                auto brainView = bot->GetActiveBrain();
                history.Undo(0, layer, index, brainView.allNeurons[layer][index]);
                assert(expectedUndo.SameNeurons(bot->GetGenome()));
                history.Redo(0, layer, index, brainView.allNeurons[layer][index]);
                assert(mutated.SameNeurons(bot->GetGenome()));
                ++changed;
            }
        assert(changed > 0);
        assert(history.Size(0, 1, 0) >= afterClear && history.Size(1, 1, 0) == 1);
#endif

        // Owner changes, deselection, and closing release all per-neuron histories.
        history.SetOwner(&expected);
        assert(history.Empty());
        Frame(app, surface);
        roll();
        app.Deselect();
        assert(history.Empty());
        app.selectedObject = bot;
        app.field->TrackObject(bot);
        app.showBrain = true;
        app.brainToShow = 0;
        Frame(app, surface);
        select(0);
        roll();
        assert(!history.Empty());
    }

    static ImGuiWindow* BrainPopup()
    {
        const auto& popups = ImGui::GetCurrentContext()->OpenPopupStack;
        assert(!popups.empty() && popups.back().Window && popups.back().Window->Active);
        return popups.back().Window;
    }

    static void ClickBrainMenu(Main& app, SDL_Surface* surface, int row, int separators = 0)
    {
        const auto popup = BrainPopup();
        const float line = ImGui::GetTextLineHeight(), spacing = ImGui::GetStyle().ItemSpacing.y;
        const float y = popup->Pos.y + popup->WindowPadding.y + line + 2*spacing + row*(line+spacing) + separators*spacing + line*.5f;
        Click(app, surface, popup->Pos.x + popup->Size.x*.5f, y);
        Frame(app, surface);
    }

    static void BrainEscape(Main& app, SDL_Surface* surface)
    {
        io->AddKeyEvent(ImGuiKey_Escape, true);
        Frame(app, surface);
        io->AddKeyEvent(ImGuiKey_Escape, false);
        Frame(app, surface);
    }

    static void ClickWeightFooter(Main& app, SDL_Surface* surface, bool apply)
    {
        Frame(app, surface);
        const auto popup = BrainPopup();
        Click(app, surface, popup->WorkRect.Min.x + 60 + (apply ? 0 : 120+ImGui::GetStyle().ItemSpacing.x),
            popup->DC.CursorMaxPos.y-14);
        Frame(app, surface);
    }

    static void CheckBrainGraphEditing(Main& app, SDL_Surface* surface)
    {
        auto bot = (Bot*)app.selectedObject;
        auto current = bot->GetActiveBrain();
        for (int layer = 0; layer < (int)NumNeuronLayers; ++layer)
#ifdef TEST_CB3
            for (int index = 0; index < (int)neuronsInLayer[layer]; ++index)
#else
            for (int index = 0; index < NeuronsInLayer; ++index)
#endif
                current.allNeurons[layer][index].ClearConnections();
        current.allNeurons[1][0].type = basic;
        bot->RecheckBrain();
        BrainGenome original(bot->GetGenome());
        const auto relative = bot->GetGenomeRef();
        auto& r = app.nn_renderer;
        r.history.Clear();
        r.history.SetOwner(bot);
        Frame(app, surface); // A shared genome must be copied before any editor operation.
        auto nodeClick = [&](int layer, int index, int button = 0) {
            const auto p = r.view.NeuronPosition(layer, index);
            Click(app, surface, p.x, p.y, button);
            Frame(app, surface);
        };
        auto start = [&] {
            nodeClick(1, 0, 1);
            assert(r.view.selectedLayer == 1 && r.view.selectedIndex == 0);
            ClickBrainMenu(app, surface, 0);
            assert(r.view.IsConnecting());
        };
        auto lineMenu = [&] {
            Frame(app, surface);
            const auto point = vlsui::NeuralGraphView::CurvePoint(r.view.NeuronPosition(1, 0), r.view.NeuronPosition(3, 1), .42f);
            Click(app, surface, point.x, point.y, 1);
            Frame(app, surface);
            assert(r.editor.sourceLayer == 1 && r.editor.sourceIndex == 0 && r.editor.targetLayer == 3 && r.editor.targetIndex == 1);
        };

        nodeClick(1, 0, 1);
        assert(IMG_SavePNG(surface, "brain-neuron-menu.png") == 0);
        ClickBrainMenu(app, surface, 0);
        assert(r.view.IsConnecting() && r.view.CanLinkTo(3, 1) && !r.view.CanLinkTo(1, 0) && !r.view.CanLinkTo(0, 0));
        nodeClick(0, 0); // Invalid destinations never change selection or the genome.
        assert(r.view.IsConnecting() && r.view.selectedLayer == 1 && original.SameNeurons(bot->GetGenome()));
        BrainEscape(app, surface);
        assert(!r.view.IsConnecting() && r.history.Empty());
        start();
        nodeClick(2, 0, 1); // Right-click cancels without opening another menu.
        assert(!r.view.IsConnecting() && r.history.Empty());

        r.view.zoomPercent = 125;
        r.view.pan = ImVec2(5, -3);
        Frame(app, surface);
        start();
        const auto target = r.view.NeuronPosition(3, 1);
        Frame(app, surface, target.x, target.y);
        assert(IMG_SavePNG(surface, "brain-add-connection.png") == 0);
        nodeClick(3, 1);
        assert(r.editor.weightOpen && !r.view.IsConnecting() && original.SameNeurons(bot->GetGenome()));
        const auto popupId = BrainPopup()->ID;
        ImGui::SetWindowSize(vlsui::BrainWindowTitle, ImVec2(650, 540));
        Frame(app, surface);
        Frame(app, surface);
        assert(r.editor.weightOpen && BrainPopup()->ID == popupId);
        assert(IMG_SavePNG(surface, "compact-brain-connection-weight.png") == 0);
        ImGui::SetWindowSize(vlsui::BrainWindowTitle, ImVec2(800, 540));
        Frame(app, surface);
        ClickWeightFooter(app, surface, false);
        assert(r.history.Empty() && original.SameNeurons(bot->GetGenome()));

        start();
        nodeClick(3, 1);
        assert(IMG_SavePNG(surface, "brain-connection-weight.png") == 0);
        const auto popup = BrainPopup();
        // Click the actual -1 preset, then explicitly commit the new edge.
        const float presetY = popup->DC.CursorMaxPos.y - 28 - 2*ImGui::GetStyle().ItemSpacing.y - ImGui::GetTextLineHeight() - 13;
        Click(app, surface, popup->WorkRect.Min.x+64+ImGui::GetStyle().ItemSpacing.x+32, presetY);
        assert(r.editor.draftWeight == -1);
        ClickWeightFooter(app, surface, true);
        const auto& source = bot->GetGenome().active[1][0];
        assert(source.numConnections == 1 && source.allConnections[0].dest_layer == 3 && source.allConnections[0].dest_neuron == 1);
#ifdef TEST_CB3
        constexpr float weightScale = WeightMultiplier;
#else
        constexpr float weightScale = 1;
#endif
        assert(std::abs(source.allConnections[0].weight*weightScale + 1) < .001f);
        assert(r.history.Size(0, 1, 0) == 1);
        assert(relative->SameNeurons(original));

        lineMenu();
        assert(IMG_SavePNG(surface, "brain-connection-menu.png") == 0);
        ClickBrainMenu(app, surface, 1); // Invert sign.
        assert(std::abs(source.allConnections[0].weight*weightScale - 1) < .001f);
        lineMenu();
        ClickBrainMenu(app, surface, 3, 1); // Undo only the source neuron.
        assert(std::abs(source.allConnections[0].weight*weightScale + 1) < .001f);
        lineMenu();
        ClickBrainMenu(app, surface, 4, 1);
        assert(std::abs(source.allConnections[0].weight*weightScale - 1) < .001f);
        lineMenu();
        ClickBrainMenu(app, surface, 0);
        assert(r.editor.weightOpen);
        r.editor.draftWeight = .73f; // Verify CB3's native weight quantization on commit.
        ClickWeightFooter(app, surface, true);
#ifdef TEST_CB3
        assert(source.allConnections[0].weight == 37);
#else
        assert(std::abs(source.allConnections[0].weight - .73f) < .001f);
#endif
        start();
        assert(!r.view.CanLinkTo(3, 1));
        nodeClick(3, 1); // Duplicate edge is not a selectable target.
        assert(r.view.IsConnecting() && source.numConnections == 1);
        BrainEscape(app, surface);
        lineMenu();
        ClickBrainMenu(app, surface, 2);
        assert(source.numConnections == 0);
        nodeClick(1, 0, 1);
        ClickBrainMenu(app, surface, 4, 2); // Node menu undo restores the exact edge.
        assert(source.numConnections == 1 && source.allConnections[0].dest_layer == 3);

        // Capacity and output-neuron restrictions are enforced in the real menu.
        r.view.Fit();
        Frame(app, surface);
        current = bot->GetActiveBrain();
        current.allNeurons[1][0].ClearConnections();
        for (int index = 0; index < (int)MaxConnectionsPerNeuronType[basic]; ++index)
            current.allNeurons[1][0].AddConnection(NumNeuronLayers-1, index, 1);
        nodeClick(1, 0, 1);
        ClickBrainMenu(app, surface, 0);
        assert(!r.view.IsConnecting());
        BrainEscape(app, surface);
        nodeClick(NumNeuronLayers-1, 0, 1);
        ClickBrainMenu(app, surface, 0);
        assert(!r.view.IsConnecting());
        BrainEscape(app, surface);
        current.allNeurons[1][0].ClearConnections();

        // Input neurons can create outgoing connections too.
        nodeClick(0, 0, 1);
        ClickBrainMenu(app, surface, 0);
        assert(r.view.IsConnecting() && r.view.linkSourceLayer == 0);
        nodeClick(2, 1);
        ClickWeightFooter(app, surface, true);
        assert(bot->GetGenome().active[0][0].numConnections == 1);
        assert(r.history.Size(0, 0, 0) == 1);

        // A brain switch or closing the window must cancel an unfinished operation.
        start();
        const auto window = ImGui::FindWindowByName(vlsui::BrainWindowTitle);
        const float toolbarX = window->Pos.x+window->WindowPadding.x;
        const float toolbarY = window->Pos.y+window->TitleBarHeight()+window->WindowPadding.y+14;
        Click(app, surface, toolbarX+146, toolbarY);
        assert(app.brainToShow == 1 && !r.view.IsConnecting());
        auto initial = bot->GetInitialBrain();
        initial.allNeurons[1][0].type = basic;
        initial.allNeurons[1][0].ClearConnections();
        BrainGenome beforeInitialEdit(bot->GetGenome());
        Frame(app, surface);
        start();
        nodeClick(3, 1);
        ClickWeightFooter(app, surface, true);
        assert(r.history.Size(1, 1, 0) == 1);
        assert(bot->GetGenome().initial[1][0].numConnections == 1 && source.numConnections == 0);
        nodeClick(1, 0, 1);
        ClickBrainMenu(app, surface, 4, 2);
        assert(beforeInitialEdit.SameNeurons(bot->GetGenome()));
        Click(app, surface, toolbarX+46, toolbarY);
        start();
        nodeClick(3, 1);
        assert(r.editor.weightOpen);
        app.showBrain = false;
        Frame(app, surface);
        assert(!r.editor.weightOpen && !r.view.IsConnecting());
        assert(ImGui::GetCurrentContext()->OpenPopupStack.empty());
        app.showBrain = true;
        Frame(app, surface);
        assert(ImGui::GetCurrentContext()->OpenPopupStack.empty());
        nodeClick(1, 0);
        r.view.Fit();
        Frame(app, surface);
        assert(relative->SameNeurons(original));
    }

    static void CheckBrainWindow(Main& app, SDL_Surface* surface)
    {
        app.showBrain = true;
        Frame(app, surface);
        Frame(app, surface);
        auto brainWindow = ImGui::FindWindowByName(vlsui::BrainWindowTitle);
        assert(brainWindow && brainWindow->Active);
        assert(!brainWindow->ScrollbarY);
        assert(!(brainWindow->Flags & (ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar)));
        assert(!ImGui::FindWindowByName("Сводка бота"));
        const auto bot = (Bot*)app.selectedObject;
        assert(app.nn_renderer.selectedBrain == bot->GetActiveBrain().allNeurons);
        auto point = app.nn_renderer.view.NeuronPosition(1, 0);
        Click(app, surface, point.x, point.y);
        assert(app.nn_renderer.selectedNeuron == &bot->GetActiveBrain().allNeurons[1][0]);
        const auto botsBeforeCanvasClick = CountBots(app);
        app.mouseFunc = mouse_remove;
        mouseState.mouseX = (int)point.x;
        mouseState.mouseY = (int)point.y;
        assert(io->WantCaptureMouse);
        app.MouseClick();
        assert(CountBots(app) == botsBeforeCanvasClick);
        app.mouseFunc = mouse_select;

        CheckBrainEdits(app, surface);
        CheckBrainGraphEditing(app, surface);

        // The small version switch lives in this window and clears stale selection.
        const float switchX = brainWindow->Pos.x + brainWindow->WindowPadding.x + 146.0f;
        const float switchY = brainWindow->Pos.y + brainWindow->TitleBarHeight() + brainWindow->WindowPadding.y + 14.0f;
        Click(app, surface, switchX, switchY);
        assert(app.brainToShow == 1);
        assert(app.nn_renderer.selectedBrain == bot->GetInitialBrain().allNeurons);
        assert(!app.nn_renderer.selectedNeuron);
        point = app.nn_renderer.view.NeuronPosition(1, 1);
        Click(app, surface, point.x, point.y);
        assert(app.nn_renderer.selectedNeuron == &bot->GetInitialBrain().allNeurons[1][1]);

        // Inject input into ImGui only: no operating-system mouse events.
        const ImVec2 previous = brainWindow->Pos;
        const ImVec2 nodeBeforeMove = app.nn_renderer.view.NeuronPosition(1, 1);
        const float titleX = previous.x + 220.0f, titleY = previous.y + 10.0f;
        Frame(app, surface, titleX, titleY);
        Frame(app, surface, titleX, titleY, true);
        Frame(app, surface, titleX + 24.0f, titleY + 18.0f, true);
        Frame(app, surface, titleX + 24.0f, titleY + 18.0f, false);
        assert(brainWindow->Pos.x != previous.x || brainWindow->Pos.y != previous.y);
        const ImVec2 nodeAfterMove = app.nn_renderer.view.NeuronPosition(1, 1);
        assert(std::abs((nodeAfterMove.x - nodeBeforeMove.x) - (brainWindow->Pos.x - previous.x)) < 1.0f);
        assert(std::abs((nodeAfterMove.y - nodeBeforeMove.y) - (brainWindow->Pos.y - previous.y)) < 1.0f);
        assert(IMG_SavePNG(surface, "brain-window.png") == 0);

        const float closeX = brainWindow->Pos.x + brainWindow->Size.x - ImGui::GetStyle().FramePadding.x - ImGui::GetFontSize() * .5f;
        const float closeY = brainWindow->Pos.y + brainWindow->TitleBarHeight() * .5f;
        Click(app, surface, closeX, closeY);
        assert(!app.showBrain);
        Frame(app, surface);
        assert(!brainWindow->Active);
        assert(app.nn_renderer.history.Empty());
        app.showBrain = true;
        Frame(app, surface);
        assert(app.nn_renderer.history.Empty());
        app.showBrain = false;
        Frame(app, surface);
        app.brainToShow = 0;
    }

    static void CheckSelection(Main& app, SDL_Surface* surface)
    {
        for (int x = 0; x < 100; ++x)
            for (int y = 0; y < 80; ++y)
                if (auto obj = app.field->GetObjectLocalCoords(x, y))
                {
#ifdef TEST_CB3
                    if (obj->type() != bot) continue;
#else
                    if (obj->type != bot) continue;
#endif
                    const double scale = Field::GetViewScale();
                    mouseState.mouseX = FieldX + (int)std::ceil(x * FieldCellSize * scale + 1e-6) - Field::viewX;
                    mouseState.mouseY = FieldY + (int)std::ceil(y * FieldCellSize * scale + 1e-6) - Field::viewY;
                    const Point cell = app.field->ScreenCoordsToLocal(mouseState.mouseX, mouseState.mouseY);
                    assert(cell.x == x && cell.y == y);
                    app.mouseFunc = mouse_select;
                    Frame(app, surface, (float)mouseState.mouseX, (float)mouseState.mouseY);
                    assert(!io->WantCaptureMouse);
                    app.MouseClick();
                    assert(app.selectedObject == obj);
                    assert(app.revealSelection);
                    return;
                }
        assert(false && "No bot found in the visible test area");
    }

    static void Run(Main& app, SDL_Surface* surface)
    {
        CheckHistory();
        srand(12345);
        assert(!app.simulate);
        Frame(app, surface);
        Frame(app, surface);
        assert(ImGui::FindWindowByName("##FirstPopulation")->Active);
        assert(IMG_SavePNG(surface, "empty-world.png") == 0);

        // Exercise the actual header button, rather than calling the seed handler.
        Click(app, surface, 95, 42);
        assert(CountBots(app) > 0);
        assert(app.chart.history.samples.back().value[vlsui::Bots] == CountBots(app));
        assert(app.field->GetTelemetryTotals().births == 0);
        assert(!app.showFirstPopulationHint);
        assert(!app.simulate);
        assert(!ImGui::FindWindowByName("##FirstPopulation")->Active);

        Click(app, surface, 240, 42);
        assert(app.simulate);
        Click(app, surface, 240, 42);
        assert(!app.simulate);
        SDL_Delay(30);
        const auto previousTick = app.ticknum;
        Click(app, surface, 340, 42);
        assert(app.ticknum == previousTick + 1);
        CheckDisplayModes(app, surface);

        // Test SDL hit testing at the new origin, also after zooming and resizing.
        CheckSelection(app, surface);
        Field::zoom = 2.0;
        app.field->ClampViewOffset();
        CheckSelection(app, surface);
        Field::zoom = 1.0;
        Frame(app, surface);
        assert(IMG_SavePNG(surface, "selected-organism.png") == 0);
        CheckFeeding(app, surface);
        CheckBrainWindow(app, surface);

        // Brush tools remain reachable independently of the inspector tab.
        const auto layout = vlsui::CalculateLayout(windowWidth, windowHeight);
        Click(app, surface, (float)layout.tools.x + 55, (float)layout.tools.y + 82);
        assert(app.mouseFunc == mouse_place_rock);
        app.mouseFunc = mouse_select;

        // Round-trip a real world through the existing, mode-specific save slot.
        const auto originalCount = CountBots(app);
        app.QuickSaveWorld();
        assert(std::filesystem::exists(QuicksaveFilename));
        app.ClearWorld();
        assert(CountBots(app) == 0);
        app.QuickLoadWorld();
        assert(CountBots(app) == originalCount);
        assert(!app.showFirstPopulationHint);
#ifdef TEST_CB3
        CheckCb3Brain(app);
#endif
        CheckGenomeSharing(app);
        CheckTelemetry(app);
        CheckCharts(app, surface, "wide");

        app.showSaveLoad = true;
        app.showAdaptation = true;
        app.showChart = true;
#ifdef TEST_CB3
        app.showAutomaticAdaptation = true;
#else
        app.showInfo = true;
#endif
        Frame(app, surface);
        app.showSaveLoad = app.showAdaptation = app.showChart = false;
#ifdef TEST_CB3
        app.showAutomaticAdaptation = false;
#else
        app.showInfo = false;
#endif
        SDL_Surface* previousSurface = nullptr;
        for (auto size : {ImVec2(640, 480), ImVec2(1024, 768), ImVec2(1920, 1080)})
        {
            SDL_Surface* resized = SDL_CreateRGBSurfaceWithFormat(0, (int)size.x, (int)size.y, 32, SDL_PIXELFORMAT_RGBA32);
            assert(resized);
            ImGui_ImplSDLRenderer_Shutdown();
            SDL_DestroyRenderer(renderer);
            if (previousSurface) SDL_FreeSurface(previousSurface);
            renderer = SDL_CreateSoftwareRenderer(resized);
            assert(renderer);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            CreateTextures();
            ImGui_ImplSDLRenderer_Init(renderer);
            Frame(app, resized);
            Frame(app, resized);
            CheckDisplayModes(app, resized);
            CheckSelection(app, resized);
            if (size.x == 1920)
            {
                Frame(app, resized);
                assert(IMG_SavePNG(resized, "wide-workspace.png") == 0);
            }
            if (size.x == 640)
            {
                CheckCharts(app, resized, "compact");
                Frame(app, resized);
                assert(IMG_SavePNG(resized, "compact-workspace.png") == 0);
                app.showBrain = true;
                Frame(app, resized);
                Frame(app, resized);
                const auto brainWindow = ImGui::FindWindowByName(vlsui::BrainWindowTitle);
                assert(brainWindow->Pos.x >= 0 && brainWindow->Pos.y >= 0);
                assert(brainWindow->Pos.x + brainWindow->Size.x <= size.x);
                assert(brainWindow->Pos.y + brainWindow->Size.y <= size.y);
                assert(IMG_SavePNG(resized, "compact-brain-window.png") == 0);
                app.showBrain = false;
                Frame(app, resized);
            }
            previousSurface = resized;
        }
        ImGui_ImplSDLRenderer_Shutdown();
        SDL_DestroyRenderer(renderer);
        SDL_FreeSurface(previousSurface);
        renderer = SDL_CreateSoftwareRenderer(surface);
        assert(renderer);
        ImGui_ImplSDLRenderer_Init(renderer);
        std::puts("PASS: workspace, brain viewer, all four chart modes/periods/tooltips, chart selectors, age panel, exact telemetry, lifecycle/manual edits, bounded history with extrema");
    }
};

#ifdef TEST_CB3
}
#endif

int main(int argc, char** argv)
{
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    assert(argc == 2);
    std::filesystem::create_directories(argv[1]);
    std::filesystem::current_path(argv[1]);
    SDL_Init(SDL_INIT_TIMER);
    auto surface = SDL_CreateRGBSurfaceWithFormat(0, 1280, 800, 32, SDL_PIXELFORMAT_RGBA32);
    assert(surface);
    renderer = SDL_CreateSoftwareRenderer(surface);
    assert(renderer);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    ImGui::CreateContext();
    io = &ImGui::GetIO();
    io->IniFilename = nullptr;
    io->LogFilename = nullptr;
    vlsui::AddInterfaceFont(*io);
    assert(io->Fonts->Fonts.Size > 0);
    ImPlot::CreateContext();
    vlsui::ApplyModernTheme();
    ImGui_ImplSDLRenderer_Init(renderer);
#ifdef TEST_CB3
    cb3::ImageFactory::CreateImages();
    {
        cb3::Main app;
        cb3::InterfaceSmokeTest::Run(app, surface);
    }
#else
    Apple::CreateImage();
    Bot::CreateImage();
    Organics::CreateImage();
    InterfaceSmokeTest::Run(simulation, surface);
    simulation.Shutdown();
#endif
    ImGui_ImplSDLRenderer_Shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_FreeSurface(surface);
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    return 0;
}
