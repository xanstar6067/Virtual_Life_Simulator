#pragma once

enum class SimulationMode
{
    Classic = 1,
    CyberBiology3 = 2
};

inline const char* SimulationModeName(SimulationMode mode)
{
    switch (mode)
    {
    case SimulationMode::Classic:
        return "Classic";
    case SimulationMode::CyberBiology3:
        return "CyberBiology3";
    default:
        return "Unknown";
    }
}

inline SimulationMode SimulationModeFromId(int id)
{
    switch (id)
    {
    case 1:
        return SimulationMode::Classic;
    case 2:
        return SimulationMode::CyberBiology3;
    default:
        return static_cast<SimulationMode>(0);
    }
}
