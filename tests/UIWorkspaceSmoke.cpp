// Headless integration checks: renders the real UI into RAM using SDL software
// rendering. It never creates a desktop window, captures the screen, or sends input.
#ifdef TEST_CB3
#include "../source/cb3/Main.h"
#else
#include "../source/Main.h"
#endif
#include "../source/UITheme.h"
#include "imgui_internal.h"
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

    static void Frame(Main& app, SDL_Surface* surface, float x = -100, float y = -100, bool down = false)
    {
        windowWidth = surface->w;
        windowHeight = surface->h;
        io->DisplaySize = ImVec2((float)windowWidth, (float)windowHeight);
        io->DeltaTime = 1.0f / 60.0f;
        io->MousePos = ImVec2(x, y);
        io->MouseDown[0] = down;
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
        assert(ImGui::FindWindowByName("##WorkspaceInspector"));
        assert(ImGui::FindWindowByName("##WorkspaceTools"));
        const auto footer = ImGui::FindWindowByName("##WorkspaceFooter");
        if (footer && footer->ScrollbarY)
        {
            std::fprintf(stderr, "Footer overflow at %dx%d: content %.1f, available %.1f\n",
                windowWidth, windowHeight, footer->ContentSize.y, footer->InnerRect.GetHeight());
            IMG_SavePNG(surface, "footer-overflow.png");
        }
        assert(footer && !footer->ScrollbarY);
    }

    static void Click(Main& app, SDL_Surface* surface, float x, float y)
    {
        Frame(app, surface, x, y);
        Frame(app, surface, x, y, true);
        Frame(app, surface, x, y, false);
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

    static void CheckSelection(Main& app)
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
                    app.MouseClick();
                    assert(app.selectedObject == obj);
                    assert(app.revealSelection);
                    return;
                }
        assert(false && "No bot found in the visible test area");
    }

    static void Run(Main& app, SDL_Surface* surface)
    {
        srand(12345);
        assert(!app.simulate);
        Frame(app, surface);
        Frame(app, surface);
        assert(ImGui::FindWindowByName("##FirstPopulation")->Active);
        assert(IMG_SavePNG(surface, "empty-world.png") == 0);

        // Exercise the actual header button, rather than calling the seed handler.
        Click(app, surface, 95, 95);
        assert(CountBots(app) > 0);
        assert(!app.showFirstPopulationHint);
        assert(!app.simulate);
        assert(!ImGui::FindWindowByName("##FirstPopulation")->Active);

        Click(app, surface, 240, 95);
        assert(app.simulate);
        Click(app, surface, 240, 95);
        assert(!app.simulate);
        SDL_Delay(30);
        const auto previousTick = app.ticknum;
        Click(app, surface, 340, 95);
        assert(app.ticknum == previousTick + 1);

        // Test SDL hit testing at the new origin, also after zooming and resizing.
        CheckSelection(app);
        Field::zoom = 2.0;
        app.field->ClampViewOffset();
        CheckSelection(app);
        Field::zoom = 1.0;
        Frame(app, surface);
        assert(IMG_SavePNG(surface, "selected-organism.png") == 0);

        // Brush tools remain reachable independently of the inspector tab.
        const auto layout = vlsui::CalculateLayout(windowWidth, windowHeight);
        Click(app, surface, 55, (float)layout.tools.y + 82);
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
            CheckSelection(app);
            if (size.x == 640) assert(IMG_SavePNG(resized, "compact-workspace.png") == 0);
            previousSurface = resized;
        }
        ImGui_ImplSDLRenderer_Shutdown();
        SDL_DestroyRenderer(renderer);
        SDL_FreeSurface(previousSurface);
        renderer = SDL_CreateSoftwareRenderer(surface);
        assert(renderer);
        ImGui_ImplSDLRenderer_Init(renderer);
        std::puts("PASS: seeding, pause/run, step, selection, zoom, brush tools, save/load, tool windows, resize");
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
