#pragma once

#include "UITheme.h"
#include "SimulationMode.h"
#include <filesystem>
#include <string>

namespace vlsui
{
// Paths survive library refreshes; browsing saves never changes the spawn source.
struct SavedBotPicker
{
    std::filesystem::path sourcePath, candidatePath;
    std::string sourceName, candidateName, error;
    bool openRequested = false;

    void Open()
    {
        candidatePath = sourcePath;
        candidateName = sourceName;
        error.clear();
        openRequested = true;
    }

    void ClearSource()
    {
        sourcePath.clear();
        sourceName.clear();
    }

    template<class Files>
    void RefreshSource(const Files& files, SimulationMode mode)
    {
        if (sourcePath.empty()) return;
        for (const auto& file : files)
            if (file.pathFull == sourcePath && file.fileType == "бот" && file.mode == mode)
            {
                sourceName = file.nameShort;
                return;
            }
        ClearSource();
    }

    template<class Files, class Accept, class Refresh>
    void Draw(const Files& files, SimulationMode mode, Accept accept, Refresh refresh)
    {
        constexpr const char* title = "Выберите бота##SavedBotPicker";
        if (openRequested)
        {
            ImGui::OpenPopup(title);
            openRequested = false;
        }
        const auto display = ImGui::GetIO().DisplaySize;
        ImGui::SetNextWindowSize(ImVec2(display.x < 660 ? display.x - 24 : 620,
            display.y < 480 ? display.y - 24 : 440), ImGuiCond_Always);
        ImGui::SetNextWindowPos(ImVec2(display.x * .5f, display.y * .5f), ImGuiCond_Appearing, ImVec2(.5f, .5f));
        bool open = true;
        if (!ImGui::BeginPopupModal(title, &open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings)) return;

        ImGui::TextWrapped("Выберите сохранённого бота для размещения в мире.");
        ImGui::TextDisabled("Режим: %s", SimulationModeName(mode));
        if (ImGui::Button("Обновить")) refresh();
        bool hasBots = false, hasCandidate = false;
        ImGui::BeginChild("##SavedBotList", ImVec2(0, ImGui::GetContentRegionAvail().y - 115), true);
        for (const auto& file : files)
        {
            if (file.fileType != "бот" || file.mode != mode) continue;
            hasBots = true;
            hasCandidate |= file.pathFull == candidatePath;
            if (ImGui::Selectable(file.nameShort.c_str(), file.pathFull == candidatePath))
            {
                candidatePath = file.pathFull;
                candidateName = file.nameShort;
                hasCandidate = true;
                error.clear();
            }
        }
        if (!hasBots)
            ImGui::TextWrapped("Сохранённых ботов пока нет. Выберите бота в мире и сохраните его через «Сохранения».");
        ImGui::EndChild();
        if (error.empty())
            ImGui::TextWrapped("После выбора нажмите на пустую клетку. Можно разместить несколько копий.");
        else
            ImGui::TextWrapped("%s", error.c_str());

        ImGui::BeginDisabled(!hasCandidate);
        if (ColoredButton("Размещать", ImVec2(150, 34), Accent()))
        {
            if (accept(candidatePath))
            {
                sourcePath = candidatePath;
                sourceName = candidateName;
                ImGui::CloseCurrentPopup();
            }
            else
                error = "Не удалось загрузить бота. Выберите другой файл или обновите список.";
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Отмена", ImVec2(110, 34)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
};
}
