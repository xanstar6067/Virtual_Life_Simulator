#pragma once

#include "GUI.h"
#include "Field.h"
#include "NeuralNetRenderer.h"
#include "Cb3Runtime.h"


enum MouseFunction
{
	mouse_select,
	mouse_remove,
	mouse_place_rock,
	mouse_from_file,
	mouse_force_mutation
};


class Main final
{

private:

	Clock clock;

	//Save/load object interface
	ObjectSaver saver;

	//Keyboard
	const Uint8* keyboard;

	//Simulation
	int seed, id;
	uint realTPS = 0;
	int limit_interval = 0;
	int limit_ticks_per_second = FPSLimitAtStart;

	Field* field = NULL;

	uint ticknum = 0;

	TimePoint prevTick;
	TimePoint lastSecondTick;
	uint tpsTickCounter = 0;
	TimePoint currentTick;

	//FPS
	int limitFPS = LimitFPSAtStart;
	int fpsInterval;
	uint realFPS = 0;
	uint fpsCounter = 0;
	TimePoint lastTickFps;
	TimePoint lastSecondTickFps;

	RenderTypes renderType = RenderTypeAtStart;
	MouseFunction mouseFunc = mouse_select;

	//Seasons
	Season season = summer;
	uint changeSeasonCounter = 0;

	//Windows
	void DrawDemoWindow();
	void DrawMainWindow();
	void DrawSystemWindow();
	void DrawControlsWindow();
	void DrawSelectionWindow();
	void DrawRenderWindow();
	void DrawConsoleWindow();
	void DrawMouseFunctionWindow();
	void DrawAdditionalsWindow();
	void DrawSidePanelWindow();
	void DrawFieldScrollbars();

	//Hidden windows
	void DrawSaveLoadWindow();
	void DrawDangerousWindow();
	void DrawSummaryWindow();
	void DrawAdaptationWindow();
	void DrawChartWindow();
	void DrawBotBrainWindow();
	void DrawInfoWindow();
	void DrawExitConfirmWindow();
	void DrawModeSwitchConfirmWindow();

	//Show more windows
	bool showSaveLoad = false;
	bool showDangerous = false;
	bool showBrain = false;
	bool showAdaptation = false;
	bool showChart = false;	
	bool showInfo = false;
	bool showExitConfirm = false;
	bool showModeSwitchConfirm = false;

	bool fieldPanActive = false;
	int fieldPanMouseX = 0;
	int fieldPanMouseY = 0;

	//Chart (TODO)
	float chartData_bots[ChartNumValues];
	float chartData_apples[ChartNumValues];
	float chartData_organics[ChartNumValues];
	int chart_numValues = 0;
	int chart_currentPosition = 0;

	int timeBeforeNextDataToChart = AddToChartEvery;

	bool chartShow_apples = false;
	bool chartShow_organics = false;

	void ClearChart();
	void AddToChart(float, float, float);

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

		bool isWorld;
		SimulationMode mode = static_cast<SimulationMode>(0);
	};

	std::vector<listed_file> allFilenames;
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

	void ChangeSeason();

	void Pause();
	void SpawnInitialPopulation();
	void PlaceWorldWall();
	void DropWorldOrganics();

	void ClearWorld();
	void ResetClassicWorld();
	void RequestSimulationMode(SimulationMode mode);
	void SwitchSimulationMode(SimulationMode mode);
	void CheckRuntimeRequests();
	bool IsClassicMode() const;
	

public:		

	//Set false to pause
	bool simulate = true;

	//Set to true to close the app
	bool terminate = false;

	SimulationMode activeMode = SimulationMode::Classic;


	bool MakeStep();

	void HandleFieldNavigation();
	
	void MouseClick();

	bool Render();
	

	Main();
	~Main();
	void Shutdown();

	void CatchKeyboard();

private:

	std::unique_ptr<Cb3Runtime> cb3Runtime;
	SimulationMode pendingMode = SimulationMode::Classic;

};


extern Main simulation;

