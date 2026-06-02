#pragma once

#include "Settings.h"
#include "MyTypes.h"

//Window
extern SDL_Renderer* renderer;
extern SDL_Window* window;
extern int windowWidth;
extern int windowHeight;

//Input output
extern ImGuiIO* io;


extern struct MouseState
{
	int
	buttons,
	wheel,
	mouseX, mouseY;
} mouseState;

bool ReadMouseState();


void InitSDL();
void DeInitSDL();
void UpdateWindowSize();

bool CreateWindowSDL();
bool CreateRenderer();



extern inline void SDLPresentScene();
