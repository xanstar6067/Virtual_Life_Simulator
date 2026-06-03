#include "Cb3Runtime.h"

#include "cb3/ImageFactory.h"
#include "cb3/Main.h"

struct Cb3Runtime::Impl
{
    std::unique_ptr<cb3::Main> simulation;

    Impl()
    {
        cb3::ImageFactory::CreateImages();
        simulation = std::make_unique<cb3::Main>();
    }

    ~Impl()
    {
        simulation.reset();
        cb3::ImageFactory::DeleteImages();
    }
};

Cb3Runtime::Cb3Runtime() :
    impl(std::make_unique<Impl>())
{
}

Cb3Runtime::~Cb3Runtime() = default;

void Cb3Runtime::MakeStep()
{
    impl->simulation->RunFrameSimulation();
}

void Cb3Runtime::Render()
{
    impl->simulation->Render();
}

void Cb3Runtime::HandleKeyboard()
{
    impl->simulation->HandleKeyboard();
}

void Cb3Runtime::HandleMouseClick()
{
    impl->simulation->HandleMouseClick();
}

void Cb3Runtime::HandleFieldNavigation()
{
    impl->simulation->HandleFieldNavigation();
}

void Cb3Runtime::Pause()
{
    impl->simulation->Pause();
}

bool Cb3Runtime::IsTerminated() const
{
    return impl->simulation->IsTerminated();
}

bool Cb3Runtime::ConsumeModeSwitchRequest(SimulationMode& mode)
{
    return impl->simulation->ConsumeModeSwitchRequest(mode);
}
