#pragma once

#include "NeuralGraphView.h"
#include "imgui_internal.h"
#include <iterator>
#include <type_traits>

namespace vlsui
{
struct BrainGraphRules
{
    float weightScale, minWeight, maxWeight;
    int maxConnections[6];
};

// Shared editor for both engines. Only neuron coordinates survive between frames.
struct BrainGraphEditor
{
    int sourceLayer = -1, sourceIndex = -1, targetLayer = -1, targetIndex = -1;
    bool contextOpen = false, weightOpen = false, adding = false;
    float draftWeight = 1.0f;
    ImGuiID contextPopupId = 0, weightPopupId = 0;

    void Reset()
    {
        // Closing/switching the brain must also release popup input capture.
        // Only our own popup stack entries and their children are closed.
        if (auto* context = ImGui::GetCurrentContext())
            for (int i = 0; i < context->OpenPopupStack.Size; ++i)
                if ((contextPopupId && context->OpenPopupStack[i].PopupId == contextPopupId) ||
                    (weightPopupId && context->OpenPopupStack[i].PopupId == weightPopupId))
                {
                    ImGui::ClosePopupToLevel(i, true);
                    break;
                }

        sourceLayer = sourceIndex = targetLayer = targetIndex = -1;
        contextOpen = weightOpen = adding = false;
    }

    static bool Valid(int layer, int index, const std::vector<int>& counts)
    {
        return layer >= 0 && layer < (int)counts.size() && index >= 0 && index < counts[layer];
    }

    template <class Neuron>
    static int Connection(const Neuron& neuron, int layer, int index)
    {
        for (unsigned c = 0; c < neuron.numConnections; ++c)
            if (neuron.allConnections[c].dest_layer == layer && neuron.allConnections[c].dest_neuron == index)
                return (int)c;
        return -1;
    }

    template <class Neuron>
    static bool HasRoom(const Neuron& neuron, int layer, const std::vector<int>& counts, const BrainGraphRules& rules)
    {
        const int type = (int)neuron.type;
        return type >= 0 && type < 6 && layer + 1 < (int)counts.size() &&
            (int)neuron.numConnections < rules.maxConnections[type] && neuron.numConnections < std::size(neuron.allConnections);
    }

    template <class Neuron>
    static bool CanConnect(const Neuron& neuron, int source, int target, int index,
        const std::vector<int>& counts, const BrainGraphRules& rules)
    {
        return Valid(target, index, counts) && target > source && HasRoom(neuron, source, counts, rules) &&
            Connection(neuron, target, index) < 0;
    }

    template <class Brain>
    void Prepare(NeuralGraphView& view, const Brain& brain, const std::vector<int>& counts, const BrainGraphRules& rules)
    {
        view.linkTargets.clear();
        if (!view.IsConnecting()) return;
        if (!Valid(view.linkSourceLayer, view.linkSourceIndex, counts)) { view.CancelLink(); return; }
        const auto& source = brain.allNeurons[view.linkSourceLayer][view.linkSourceIndex];
        for (int layer = view.linkSourceLayer + 1; layer < (int)counts.size(); ++layer)
            for (int index = 0; index < counts[layer]; ++index)
                if (CanConnect(source, view.linkSourceLayer, layer, index, counts, rules))
                    view.linkTargets.emplace_back(layer, index);
        if (view.linkTargets.empty()) view.CancelLink();
    }

    template <class Weight>
    static Weight NativeWeight(float value, const BrainGraphRules& rules)
    {
        if (!std::isfinite(value)) value = 0;
        float native = std::clamp(value, rules.minWeight, rules.maxWeight) / rules.weightScale;
        if constexpr (std::is_integral_v<Weight>) native = std::round(native);
        return static_cast<Weight>(native);
    }

    template <class Brain, class History>
    void Draw(NeuralGraphView& view, Brain& brain, History& history, int version,
        const std::vector<int>& counts, const BrainGraphRules& rules)
    {
        bool requestWeight = false;
        ImGui::PushID("BrainGraphEditor");
        contextPopupId = ImGui::GetID("##BrainContextMenu");
        weightPopupId = ImGui::GetID("##ConnectionWeightEditor");
        if (view.contextRequested)
        {
            sourceLayer = view.contextLayer; sourceIndex = view.contextIndex;
            targetLayer = view.contextTargetLayer; targetIndex = view.contextTargetIndex;
            contextOpen = true;
            ImGui::OpenPopup("##BrainContextMenu");
        }
        if (view.chosenTargetLayer >= 0)
        {
            sourceLayer = view.linkSourceLayer; sourceIndex = view.linkSourceIndex;
            targetLayer = view.chosenTargetLayer; targetIndex = view.chosenTargetIndex;
            adding = true; draftWeight = 1.0f;
            requestWeight = true;
            view.CancelLink();
        }
        if (ImGui::BeginPopup("##BrainContextMenu"))
        {
            if (!contextOpen || ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                contextOpen = false;
                ImGui::CloseCurrentPopup();
            }
            else if (Valid(sourceLayer, sourceIndex, counts))
            {
                auto& neuron = brain.allNeurons[sourceLayer][sourceIndex];
                auto record = [&](const auto& operation) { history.Record(version, sourceLayer, sourceIndex, neuron, operation); };
                const int edge = Connection(neuron, targetLayer, targetIndex);
                if (targetLayer >= 0)
                {
                    ImGui::Text("Связь %d:%d → %d:%d", sourceLayer+1, sourceIndex+1, targetLayer+1, targetIndex+1);
                    ImGui::Separator();
                    if (ImGui::MenuItem("Изменить вес...", nullptr, false, edge >= 0))
                    {
                        adding = false; draftWeight = neuron.allConnections[edge].weight * rules.weightScale;
                        requestWeight = true;
                    }
                    if (ImGui::MenuItem("Сменить знак веса", nullptr, false, edge >= 0))
                        record([&] {
                            using Weight = decltype(neuron.allConnections[edge].weight);
                            neuron.allConnections[edge].weight = NativeWeight<Weight>(-neuron.allConnections[edge].weight * rules.weightScale, rules);
                        });
                    if (ImGui::MenuItem("Удалить связь", nullptr, false, edge >= 0))
                        record([&] { neuron.RemoveConnection(edge); });
                }
                else
                {
                    ImGui::Text("Слой %d · нейрон %d", sourceLayer+1, sourceIndex+1);
                    ImGui::Separator();
                    const bool room = HasRoom(neuron, sourceLayer, counts, rules);
                    if (ImGui::MenuItem("Добавить связь...", nullptr, false, room))
                    {
                        view.linkSourceLayer = sourceLayer; view.linkSourceIndex = sourceIndex;
                    }
                    if (!room && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                        ImGui::SetTooltip(sourceLayer+1 == (int)counts.size() ?
                            "Выходные нейроны не создают исходящих связей." : "Достигнут лимит связей этого типа нейрона.");
                    ImGui::Separator();
                    const bool editable = (int)neuron.type != 1 && (int)neuron.type != 2;
                    if (ImGui::MenuItem("Случайный нейрон", nullptr, false, editable)) record([&] { neuron.SetRandom(); });
                    if (ImGui::MenuItem("Обнулить нейрон", nullptr, false, editable)) record([&] { neuron.SetZero(); });
                    if (ImGui::MenuItem("Удалить исходящие связи", nullptr, false, neuron.numConnections > 0))
                        record([&] { neuron.ClearConnections(); });
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Отменить", nullptr, false, history.CanUndo(version, sourceLayer, sourceIndex)))
                    history.Undo(version, sourceLayer, sourceIndex, neuron);
                if (ImGui::MenuItem("Повторить", nullptr, false, history.CanRedo(version, sourceLayer, sourceIndex)))
                    history.Redo(version, sourceLayer, sourceIndex, neuron);
            }
            else
            {
                if (ImGui::MenuItem("Вписать схему")) view.Fit();
                ImGui::MenuItem("Только связи выбранного нейрона", nullptr, &view.selectedConnectionsOnly);
                if (ImGui::MenuItem("Снять выделение")) view.ClearSelection();
            }
            ImGui::EndPopup();
        }
        if (requestWeight)
        {
            weightOpen = true;
            ImGui::OpenPopup("##ConnectionWeightEditor");
        }
        ImGui::SetNextWindowSizeConstraints(ImVec2(300, 0), ImVec2(400, (std::max)(1.0f, ImGui::GetIO().DisplaySize.y-24)));
        if (ImGui::BeginPopup("##ConnectionWeightEditor"))
        {
            if (!weightOpen || ImGui::IsKeyPressed(ImGuiKey_Escape) || !Valid(sourceLayer, sourceIndex, counts) || !Valid(targetLayer, targetIndex, counts))
                ImGui::CloseCurrentPopup();
            else
            {
                auto& neuron = brain.allNeurons[sourceLayer][sourceIndex];
                const int edge = Connection(neuron, targetLayer, targetIndex);
                const bool allowed = adding ? CanConnect(neuron, sourceLayer, targetLayer, targetIndex, counts, rules) : edge >= 0;
                ImGui::TextUnformatted(adding ? "Новая связь" : "Вес связи");
                ImGui::Text("Слой %d · нейрон %d → слой %d · нейрон %d", sourceLayer+1, sourceIndex+1, targetLayer+1, targetIndex+1);
                ImGui::Separator();
                ImGui::SetNextItemWidth(-1);
                ImGui::SliderFloat("##ConnectionWeight", &draftWeight, rules.minWeight, rules.maxWeight, "Вес: %+.2f", ImGuiSliderFlags_AlwaysClamp);
                ItemTooltip("Положительный вес усиливает сигнал, отрицательный — подавляет. Ctrl + клик — точный ввод.");
                if (ImGui::Button("+1", ImVec2(64, 26))) draftWeight = 1;
                ImGui::SameLine();
                if (ImGui::Button("-1", ImVec2(64, 26))) draftWeight = -1;
                ImGui::SameLine();
                if (ImGui::Button("0", ImVec2(64, 26))) draftWeight = 0;
                using Weight = decltype(neuron.allConnections[0].weight);
                const Weight native = NativeWeight<Weight>(draftWeight, rules);
                ImGui::Text("Будет сохранён вес: %+.3f", native * rules.weightScale);
                if (!allowed) ImGui::TextWrapped("Связь уже существует или больше недоступна.");
                ImGui::BeginDisabled(!allowed || !std::isfinite(draftWeight));
                if (ImGui::Button(adding ? "Добавить" : "Сохранить", ImVec2(120, 28)))
                {
                    history.Record(version, sourceLayer, sourceIndex, neuron, [&] {
                        if (adding) neuron.AddConnection(targetLayer, targetIndex, native);
                        else neuron.allConnections[edge].weight = native;
                    });
                    weightOpen = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndDisabled();
                ImGui::SameLine();
                if (ImGui::Button("Отмена", ImVec2(120, 28))) { weightOpen = false; ImGui::CloseCurrentPopup(); }
            }
            ImGui::EndPopup();
        }
        if (!ImGui::IsPopupOpen("##ConnectionWeightEditor")) weightOpen = false;
        ImGui::PopID();
    }
};
} // namespace vlsui
