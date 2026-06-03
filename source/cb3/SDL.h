#pragma once

#include "Settings.h"
#include "Utils.h"

// These symbols are owned by the host application in source/SDL.cpp.
// CB3 uses them through cb3 namespace aliases below so it renders into the same window.
struct MouseState
{
    int buttons;
    int wheel;
    int mouseX;
    int mouseY;
};

extern SDL_Renderer* renderer;
extern SDL_Window* window;
extern int windowWidth;
extern int windowHeight;
extern ImGuiIO* io;
extern MouseState mouseState;

bool ReadMouseState();
void InitSDL();
void DeInitSDL();
void UpdateWindowSize();
bool CreateWindowSDL();
bool CreateRenderer();
void SDLPresentScene();

namespace cb3
{
    using ::renderer;
    using ::window;
    using ::windowWidth;
    using ::windowHeight;
    using ::io;
    using ::MouseState;
    using ::mouseState;
    using ::ReadMouseState;
    using ::InitSDL;
    using ::DeInitSDL;
    using ::UpdateWindowSize;
    using ::CreateWindowSDL;
    using ::CreateRenderer;
    using ::SDLPresentScene;
}
