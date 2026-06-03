#pragma once

#include "SimulationMode.h"

#include <memory>

class Cb3Runtime final
{
public:
    Cb3Runtime();
    ~Cb3Runtime();

    Cb3Runtime(const Cb3Runtime&) = delete;
    Cb3Runtime& operator=(const Cb3Runtime&) = delete;

    void MakeStep();
    void Render();
    void HandleKeyboard();
    void HandleMouseClick();
    void Pause();

    bool IsTerminated() const;
    bool ConsumeModeSwitchRequest(SimulationMode& mode);

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
