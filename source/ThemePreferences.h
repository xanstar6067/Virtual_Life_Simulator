#pragma once

#include "Theme.h"
#include <SDL_filesystem.h>
#include <filesystem>
#include <fstream>
#include <string>

namespace vlsui
{
// Stable file identifiers are independent of enum ordering and translated labels.
inline const char* ThemePreferenceKey(ThemeId id)
{
    switch (id)
    {
    case ThemeId::Dark: return "theme=dark";
    case ThemeId::Light: return "theme=light";
    case ThemeId::Retro: return "theme=retro";
    default: return nullptr;
    }
}

inline ThemeId ReadThemePreference(const std::filesystem::path& path)
{
    std::ifstream input(path);
    char line[32]{};
    if (!input.getline(line, sizeof(line))) return ThemeId::Dark;
    for (auto id : BuiltinThemes)
        if (!std::strcmp(line, ThemePreferenceKey(id))) return id;
    return ThemeId::Dark;
}

inline bool WriteThemePreference(const std::filesystem::path& path, ThemeId id)
{
    const char* key = ThemePreferenceKey(id);
    if (!key || path.empty()) return false;
    std::error_code error;
    if (!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path(), error);
    if (error) return false;
    std::ofstream output(path, std::ios::trunc);
    output << key << '\n';
    output.close();
    return !output.fail();
}

namespace detail
{
inline std::filesystem::path themePreferencePath;
inline bool themePreferencesInitialized = false;
inline bool themePreferenceSaveFailed = false;
}

// Explicit path keeps regression tests isolated from the user's real preferences.
inline void InitializeThemePreferences(const std::filesystem::path& path)
{
    detail::themePreferencesInitialized = true;
    detail::themePreferencePath = path;
    detail::themePreferenceSaveFailed = false;
    ApplyTheme(MakeBuiltinTheme(ReadThemePreference(path)));
}

inline void InitializeThemePreferences()
{
    if (detail::themePreferencesInitialized) return;
    char* directory = SDL_GetPrefPath("VirtualLifeSimulator", "VirtualLifeSimulator");
    if (directory)
    {
        const auto path = std::filesystem::path(std::u8string(directory, directory + std::strlen(directory))) / "appearance.ini";
        SDL_free(directory);
        InitializeThemePreferences(path);
    }
    else
    {
        detail::themePreferencesInitialized = true;
        detail::themePreferenceSaveFailed = true;
    }
}

inline void SaveSelectedThemePreference()
{
    if (!detail::themePreferencesInitialized) return;
    detail::themePreferenceSaveFailed = !WriteThemePreference(detail::themePreferencePath, GetTheme().id);
}
inline bool ThemePreferenceSaveFailed() { return detail::themePreferenceSaveFailed; }
} // namespace vlsui
