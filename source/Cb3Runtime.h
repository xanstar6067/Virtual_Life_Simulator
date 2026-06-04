#pragma once

#include "SimulationMode.h"
#include "cb3/WorldConfig.h"

#include <memory>

class Cb3Runtime final
{
public:
    explicit Cb3Runtime(const cb3::WorldConfig& config = cb3::WorldConfig());
    ~Cb3Runtime();

    Cb3Runtime(const Cb3Runtime&) = delete;
    Cb3Runtime& operator=(const Cb3Runtime&) = delete;

    void MakeStep();
    void Render();
    void HandleKeyboard();
    void HandleMouseClick();
    void HandleFieldNavigation();
    void Pause();

    bool IsTerminated() const;
    bool ConsumeModeSwitchRequest(SimulationMode& mode);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
