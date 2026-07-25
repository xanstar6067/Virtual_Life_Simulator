#pragma once

#include "GUI.h"
#include "Field.h"
#include "NeuralNetRenderer.h"
#include "AutomaticAdaptation.h"
#include "../SimulationMode.h"

#include <filesystem>


namespace cb3
{

enum MouseFunction
{
	mouse_select,
	mouse_remove,
	mouse_place_rock,
	mouse_from_file,
	mouse_force_mutation
};


class Main
{
protected:

	Clock clock;

	const SDL_Rect screenRect = { 0, 0, WindowWidth, WindowHeight };

	//Automatic adaptation
	AutomaticAdaptation* auto_adapt;

	//Keyboard
	const Uint8* keyboard;

	//Simulation
	uint seed, id;
	uint realTPS = 0;
	int limit_interval = 0;
	int limit_ticks_per_second = TPSLimitAtStart;
	uint ticknum = 0;

	TimePoint prevTick;
	TimePoint lastSecondTick;
	uint tpsTickCounter = 0;
	TimePoint currentTick;

	Field* field = NULL;			

	//FPS
	int limitFPS = LimitFPSAtStart;
	int fpsInterval;
	uint realFPS = 0;
	uint fpsCounter = 0;
	TimePoint lastTickFps;
	TimePoint lastSecondTickFps;

	RenderTypes renderType = natural;
	MouseFunction mouseFunc = mouse_select;

	//Windows
	void DrawDemoWindow();
	void DrawMainWindow();
	void DrawSystemWindow();
	void DrawControlsWindow();
	void DrawSelectionWindow();
	void DrawDisplayWindow();
	void DrawLogWindow();
	void DrawMouseFunctionWindow();
	void DrawAdditionalsWindow();
	void DrawSidePanelWindow();
	void DrawFieldScrollbars();
	void DrawModeSwitchWindow();
	void DrawModeSwitchConfirmWindow();

	//Hidden windows
	void DrawSaveLoadWindow();
	void DrawDangerousWindow();
	void DrawSummaryWindow();
	void DrawAdaptationWindow();
	void DrawChartWindow();
	void DrawBotBrainWindow();
	void DrawAAWindow();

	//Show more windows
	bool showSaveLoad = false;
	bool showDangerous = false;
	bool showBrain = false;
	bool showAdaptation = false;
	bool showChart = false;	
	bool showAutomaticAdaptation = false;
	bool showModeSwitchConfirm = false;
	bool modeSwitchRequested = false;
	SimulationMode requestedMode = SimulationMode::CyberBiology3;
	bool fieldPanActive = false;
	int fieldPanMouseX = 0;
	int fieldPanMouseY = 0;

	//Chart
	Chart chart;

	//Neural net renderer
	NeuralNetRenderer nn_renderer;
	//Show initial brain or active brain
	int brainToShow = 0;

	//Bot selection
	Object* selectedObject = NULL;

	int cursorBlink = 0;
	bool cursorShow = true;	
	int selectionShadowScreen = 0;

	void Deselect();

	int brushSize = DefaultBrushRadius;

	//Log
	ImGuiTextBuffer logText;

	void ClearLog();
	void LogPrint(const char* str, bool newLine = false);
	void LogPrint(int num, bool newLine = true);

	//Save/load
	ObjectSaver saver;

	struct listed_file
	{
		std::filesystem::path pathFull;
		string nameFull;
		string nameShort;
		string fileSize;
		string fileType;
		string modeText;
		string modifiedTimeText;
		std::filesystem::file_time_type modifiedTime;

		bool isSelected = false;
		bool isWorld = false;
		SimulationMode mode = static_cast<SimulationMode>(0);
	};

	vector<listed_file> allFilenames;
	listed_file* selectedFile = NULL;
	char renameFileName[128] = "";
	bool saveFileNameInputActive = false;

	void LoadFilenames();
	void SelectFile(int index);
	void RenameSelectedFile();
	void DeleteSelectedFile();
	void SaveSelectedObjectToNamedFile();
	void SaveWorldToNamedFile();
	std::filesystem::path BuildSavePath(const char* defaultPrefix);
	void SelectFileByPath(const std::filesystem::path& filePath);

	void DrawWindows();

	void HighlightSelection();
	void SelectionShadowScreen();

	//Set false to pause
	bool simulate = true;
	//Set to true to close the app
	bool terminate = false;
	
	bool windowIsVisible = true;

	void SwitchPause();		
	void SpawnInitialPopulation();
	void PlaceWorldWall();
	void DropWorldOrganics();
	void SpawnWorldRocks();
	void MutateWholeWorld();
	void QuickSaveWorld();
	void QuickLoadWorld();

	void BrushIterate(Point p, void (*callback)(uint, uint, Field*));

	void MouseClick();
	void CatchKeyboard();
	

public:		

	void ClearWorld();

	bool MakeStep();
	bool RunFrameSimulation();
	bool Render();

	void Start();
	void Pause();
	void RunWithNoRender();
	void RunWithMinimumRender();

	void Print(string s);

	Main();
	~Main();

	void MainLoop();
	void HandleKeyboard();
	void HandleMouseClick();
	void HandleFieldNavigation();
	bool IsTerminated() const;
	bool ConsumeModeSwitchRequest(SimulationMode& mode);
};


}
