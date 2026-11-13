#pragma once

#include "UITheme.h"
#include "imgui_internal.h"
#include <algorithm>
#include <chrono>
#include <functional>
#include <string>
#include <utility>

namespace vlsui
{
// Per simulation session: callbacks own their arguments, never file-list pointers.
class ActionFeedback
{
    std::string success, error, title, consequences;
    std::chrono::steady_clock::time_point successUntil;
    std::function<void()> pending;
    bool openConfirmation = false;

public:
    static constexpr const char* ConfirmationTitle = "Подтверждение действия###WorldActionConfirm";
    bool HasConfirmation() const { return bool(pending); }
    const std::string& Error() const { return error; }
    const std::string& Success() const { return success; }
    void ClearNotices() { error.clear(); success.clear(); }

    void Notify(const char* message, bool failed = false)
    {
        std::string text = message;
        const auto end = text.find_last_not_of("\r\n \t");
        if (end == std::string::npos || text.find("Запущено.") == 0) return;
        text.resize(end + 1);
        if (failed) error = std::move(text);
        else
        {
            success = std::move(text);
            successUntil = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        }
    }

    void Confirm(const char* actionTitle, const char* description, std::function<void()> action)
    {
        if (pending) return;
        title = actionTitle;
        consequences = description;
        pending = std::move(action);
        openConfirmation = true;
    }

    void Draw()
    {
        if (!success.empty() && std::chrono::steady_clock::now() >= successUntil) success.clear();
        if (!success.empty() || !error.empty())
        {
            const auto layout = CalculateLayout((int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
            const float width = (std::min)(420.0f, (float)layout.world.w);
            ImGui::SetNextWindowPos(ImVec2((float)layout.world.x, (float)(layout.footer.y - 10)), ImGuiCond_Always, ImVec2(0, 1));
            ImGui::SetNextWindowSize(ImVec2(width, 0));
            ImGui::SetNextWindowBgAlpha(0.98f);
            const auto flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                (error.empty() ? ImGuiWindowFlags_NoInputs : 0);
            ImGui::Begin("##ActionNotice", nullptr, flags);
            if (!ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
                ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
            if (!error.empty())
            {
                ImGui::PushStyleColor(ImGuiCol_Text, Warning());
                ImGui::TextWrapped("Не удалось выполнить действие");
                ImGui::PopStyleColor();
                ImGui::TextWrapped("%s", error.c_str());
                if (ImGui::Button("Закрыть уведомление")) error.clear();
            }
            if (!success.empty()) ImGui::TextWrapped("%s", success.c_str());
            ImGui::End();
        }

        if (openConfirmation)
        {
            ImGui::OpenPopup(ConfirmationTitle);
            openConfirmation = false;
        }
        std::function<void()> accepted;
        const float width = (std::min)(500.0f, ImGui::GetIO().DisplaySize.x - 32.0f);
        ImGui::SetNextWindowSize(ImVec2(width, 0));
        const auto display = ImGui::GetIO().DisplaySize;
        ImGui::SetNextWindowPos(ImVec2(display.x * 0.5f, display.y * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal(ConfirmationTitle, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
        {
            ImGui::TextWrapped("%s", title.c_str());
            ImGui::Spacing();
            ImGui::TextWrapped("%s", consequences.c_str());
            ImGui::TextWrapped("Несохранённые изменения нельзя будет восстановить.");
            ImGui::Spacing();
            if (ImGui::Button("Отмена", ImVec2(HalfWidth(), 34)) || ImGui::IsKeyPressed(ImGuiKey_Escape))
            {
                pending = {};
                ImGui::CloseCurrentPopup();
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ColoredButton("Подтвердить", ImVec2(-1, 34), ImVec4(0.62f, 0.22f, 0.19f, 1)))
            {
                accepted = std::move(pending);
                pending = {};
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
        if (pending && !ImGui::IsPopupOpen(ConfirmationTitle)) pending = {};
        // Run after all inspectors and popups have finished using old objects.
        if (accepted) accepted();
    }
};
}
