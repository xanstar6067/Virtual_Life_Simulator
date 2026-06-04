#pragma once

//System headers
#include <Windows.h>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <vector>

//ImGUI
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_sdlrenderer.h"
#include "imgui_impl_opengl3.h"

//ImPlot
#include "implot.h"

//SDL + OpenGL
#include <SDL.h>
#include <SDL_opengl.h>
