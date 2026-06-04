
//-----------------------------------------------------------------
// 
// 
// Примерно как устроено:
// -Входная точка в файле Main.cpp
// -Все настраиваемые параметры в файле Settings.h в виде дефайнов
// -Все объекты на поле наследуется от Object
// -Neuron.h это нейрон, BotNeuralNet.h - вся нейросеть
// -Field, это класс игрового поля
// 
// Также:
// -NeuralNetRenderer это класс, который рисует мозг бота в отдельном окошке
// -ObjectSaver - сохраняет объекты и мир в файл
// -MyTypes - определение некоторых дополнительных типов данных для удобства
// 
//
//-----------------------------------------------------------------



#include "Main.h"

#include <algorithm>
#include <ctime>
#include <shellapi.h>


Main simulation;


static string FormatFileSize(uintmax_t size)
{
	string unit;

	if (size > 1000000)
	{
		size /= 1000000;
		unit += "МБ";
	}
	else if (size > 1000)
	{
		size /= 1000;
		unit += "КБ";
	}
	else
	{
		unit += "б";
	}

	return std::to_string(size) + unit;
}


static string FormatFileTime(std::filesystem::file_time_type fileTime)
{
	auto systemTime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
		fileTime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());

	std::time_t time = std::chrono::system_clock::to_time_t(systemTime);
	std::tm localTime;
	localtime_s(&localTime, &time);

	char buffer[32];
	std::strftime(buffer, sizeof(buffer), "%d.%m.%Y %H:%M", &localTime);

	return buffer;
}


static string TrimFileName(string fileName)
{
	while (!fileName.empty() && (fileName.front() == ' ' || fileName.front() == '\t'))
	{
		fileName.erase(fileName.begin());
	}

	while (!fileName.empty() && (fileName.back() == ' ' || fileName.back() == '\t'))
	{
		fileName.pop_back();
	}

	return fileName;
}


static std::filesystem::path PathFromUtf8(string fileName)
{
	int wideSize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, fileName.c_str(), -1, NULL, 0);

	if (wideSize <= 0)
	{
		return std::filesystem::path(fileName);
	}

	std::wstring wideName(wideSize - 1, L'\0');
	MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, fileName.c_str(), -1, wideName.data(), wideSize);

	return std::filesystem::path(wideName);
}

static string PathToUtf8(const std::filesystem::path& path)
{
	std::wstring wideName = path.wstring();

	if (wideName.empty())
	{
		return "";
	}

	int utf8Size = WideCharToMultiByte(CP_UTF8, 0, wideName.c_str(), (int)wideName.size(), NULL, 0, NULL, NULL);

	if (utf8Size <= 0)
	{
		return path.filename().string();
	}

	string utf8Name(utf8Size, '\0');
	WideCharToMultiByte(CP_UTF8, 0, wideName.c_str(), (int)wideName.size(), utf8Name.data(), utf8Size, NULL, NULL);

	return utf8Name;
}


static string MakeTimestampFileName(const char* prefix)
{
	std::time_t time = std::time(NULL);
	std::tm localTime;
	localtime_s(&localTime, &time);

	char buffer[64];
	std::strftime(buffer, sizeof(buffer), "%Y%m%d_%H%M%S", &localTime);

	return string(prefix) + "_" + buffer;
}


int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR    lpCmdLine, _In_ int       nCmdShow)
{
	
	InitSDL();

	if (!CreateWindowSDL())
		return -2;

	CreateRenderer();

	InitImGUI();

	Apple::CreateImage();
	Bot::CreateImage();
	Organics::CreateImage();

	//Main loop	
	SDL_Event e;

	for (;;)
	{
		//Events
		mouseState.wheel = 0;

		while (SDL_PollEvent(&e) != 0)
		{
			ImGui_ImplSDL2_ProcessEvent(&e);

			if (e.type == SDL_QUIT)
			{
				goto exitfor;
			}
			else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
			{
				UpdateWindowSize();
			}
			else if (e.type == SDL_MOUSEWHEEL)
			{
				mouseState.wheel += e.wheel.y;
			}
			else if (e.type == SDL_KEYDOWN)
			{
				simulation.CatchKeyboard();
			}
		}

		//Mouse down event
		bool mouseClick = ReadMouseState();

		simulation.HandleFieldNavigation();

		if (mouseClick)
		{
			simulation.MouseClick();
		}

		//Simulation
		if (simulation.simulate)
		{
			simulation.MakeStep();
		}
		else
		{
			//Delay so it would not eat too many resourses while on pause
			SDL_Delay(1);
		}

		simulation.Render();

		if (simulation.terminate)
			goto exitfor;

	}
exitfor:

	//Clear memory
	simulation.Shutdown();
	Apple::DeleteImage();
	Bot::DeleteImage();
	Organics::DeleteImage();

	DeInitImGUI();
	DeInitSDL();

	return 0;
}


void Main::ChangeSeason()
{
	season = (Season)((int)season + 1);

	if (season > spring)
	{
		season = summer;
	}
}


void Main::Pause()
{
	if (!field)
	{
		return;
	}

	simulate = !simulate;

	if (simulate)
	{
		field->UnpauseThreads();
	}
	else
	{
		field->PauseThreads();
	}
}

void Main::MakeStep()
{
	if (!IsClassicMode())
	{
		if (cb3Runtime)
		{
			cb3Runtime->MakeStep();
			CheckRuntimeRequests();
		}
		return;
	}

	//Simulation step
	currentTick = clock.now();

	if (limit_ticks_per_second > 0)
	{
		limit_interval = 1000 / limit_ticks_per_second;
	}
	else
	{
		limit_interval = 0;
	}

	if ((TimeMSBetween(currentTick, prevTick) >= limit_interval) || (limit_interval == 0) || (renderType == noRender))
	{
		prevTick = currentTick;

		field->tick(ticknum);

		++ticknum;
		++tpsTickCounter;

	#ifdef UseSeasons	
		if (++changeSeasonCounter >= ChangeSeasonAfter)
		{
			ChangeSeason();

			changeSeasonCounter = 0;
		}
	#endif

		//Add data to chart
		if (--timeBeforeNextDataToChart == 0)
		{
			AddToChart(field->GetNumBots() * 1.0f,
				field->GetNumApples() * 1.0f, field->GetNumOrganics() * 1.0f);

			timeBeforeNextDataToChart = AddToChartEvery;
		}
	}

	//Calculate simulation speed
	if (TimeMSBetween(currentTick, lastSecondTick) >= 1000)
	{
		lastSecondTick = currentTick;

		realTPS = tpsTickCounter;
		tpsTickCounter = 0;
	}
}

void Main::HighlightSelection()
{
	if (selectionShadowScreen > 0)
	{
		SDL_Rect screenRect = { 0, 0, windowWidth, windowHeight };

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, selectionShadowScreen);
		SDL_RenderFillRect(renderer, &screenRect);
	}

	if (selectedObject)
	{
		if (cursorShow)
		{
			SDL_Rect viewport = field->GetViewportRect();

			SDL_RenderSetClipRect(renderer, &viewport);
			selectedObject->Object::draw();
			SDL_RenderSetClipRect(renderer, NULL);
		}
	}
}

void Main::SelectionShadowScreen()
{
	if (selectedObject)
	{
		if (selectionShadowScreen < 200)
			selectionShadowScreen += 5;
	}
	else
	{
		if (selectionShadowScreen > 0)
			selectionShadowScreen -= 5;
	}

	//Cursor blinking
	if (cursorBlink-- == 0)
	{
		cursorBlink = CursorBlinkRate;
		cursorShow = !cursorShow;
	}
}

void Main::ClearChart()
{
	memset(chartData_bots, 0, sizeof(chartData_bots));
	memset(chartData_organics, 0, sizeof(chartData_organics));
	memset(chartData_apples, 0, sizeof(chartData_apples));

	chart_numValues = 0;
	chart_currentPosition = 0;
}

void Main::AddToChart(float newVal_bots, float newVal_apples, float newVal_organics)
{
	chartData_bots[chart_currentPosition] = newVal_bots;
	chartData_apples[chart_currentPosition] = newVal_apples;
	chartData_organics[chart_currentPosition] = newVal_organics;

	if (chart_numValues < ChartNumValues)
	{
		++chart_numValues;
		++chart_currentPosition;
	}
	else
	{
		if (chart_currentPosition == ChartNumValues)
			ClearChart();
		else
			++chart_currentPosition;
	}
}

void Main::Deselect()
{
	selectedObject = NULL;
	showBrain = false;
	nn_renderer.selectedNeuron = NULL;
}


void Main::ClearLog()
{
	logText.clear();
}

void Main::LogPrint(const char* str, bool newLine)
{
	logText.append(str);

	if (newLine)
		LogPrint("\r\n");
}

void Main::LogPrint(int num, bool newLine)
{
	logText.appendf("%i", num);
	
	if(newLine)
		LogPrint("\r\n");
}

void Main::LoadFilenames()
{
	std::filesystem::path selectedPath;

	if (selectedFile)
	{
		selectedPath = selectedFile->pathFull;
	}

	//Check if folder exists
	if (!std::filesystem::exists(DirectoryName))
	{
		//If not create a folder
		std::filesystem::create_directory(DirectoryName);
	}

	//Load list of filenames
	allFilenames.clear();
	selectedFile = NULL;
	int selectedIndex = -1;

	for (const auto& entry : std::filesystem::directory_iterator(DirectoryName))
	{
		//Skip folders
		if (entry.is_directory())
			continue;

		listed_file f;

		//Full paths to files
		f.pathFull = entry.path();
		f.nameFull = PathToUtf8(entry.path());

		//Only file name
		f.nameShort = PathToUtf8(entry.path().filename());

		//File size and modified time
		uintmax_t size = entry.file_size();
		f.fileSize = FormatFileSize(size);
		f.modifiedTime = entry.last_write_time();
		f.modifiedTimeText = FormatFileTime(f.modifiedTime);

		//Is world (open file briefly and look for file type)
		MyInputStream file(f.pathFull, std::ios::in | std::ios::binary | std::ios::beg);

		if (!file.is_open())
			continue;

		int magicNumber = 0;

		if (size > 0)
			magicNumber = file.ReadInt();

		int modeId = 0;

		if ((magicNumber == MagicNumber_WorldFileV2) || (magicNumber == MagicNumber_ObjectFile))
		{
			modeId = file.ReadInt();
		}

		f.mode = SimulationModeFromId(modeId);
		f.modeText = (modeId > 0) ? SimulationModeName(f.mode) : "-";

		if (magicNumber == MagicNumber_WorldFileV2)
			f.isWorld = true;
		else
			f.isWorld = false;

		if (f.isWorld)
			f.fileType = "мир";
		else if (magicNumber == MagicNumber_ObjectFile)
			f.fileType = "бот";
		else
			f.fileType = "файл";

		file.close();

		allFilenames.push_back(f);
	}

	std::sort(allFilenames.begin(), allFilenames.end(), [](const listed_file& left, const listed_file& right)
	{
		return left.modifiedTime > right.modifiedTime;
	});

	for (size_t i = 0; i < allFilenames.size(); ++i)
	{
		if (allFilenames[i].pathFull == selectedPath)
		{
			selectedIndex = (int)i;
			break;
		}
	}

	SelectFile(selectedIndex);
}




void Main::SelectFile(int index)
{
	if (index < 0 || index >= (int)allFilenames.size())
	{
		selectedFile = NULL;
		renameFileName[0] = '\0';
		return;
	}

	for (size_t i = 0; i < allFilenames.size(); ++i)
	{
		allFilenames[i].isSelected = false;
	}

	allFilenames[index].isSelected = true;
	selectedFile = &allFilenames[index];

	size_t len = selectedFile->nameShort.size();
	if (len >= sizeof(renameFileName))
	{
		len = sizeof(renameFileName) - 1;
	}

	memcpy(renameFileName, selectedFile->nameShort.c_str(), len);
	renameFileName[len] = '\0';
}


void Main::RenameSelectedFile()
{
	if (!selectedFile)
	{
		LogPrint("Файл не выбран\r\n");
		return;
	}

	std::string newName = renameFileName;

	if (newName.empty())
	{
		LogPrint("Имя файла пустое\r\n");
		return;
	}

	std::filesystem::path newNamePath = PathFromUtf8(newName);

	if (newNamePath.filename() != newNamePath)
	{
		LogPrint("Имя файла не должно содержать путь\r\n");
		return;
	}

	std::filesystem::path oldPath = selectedFile->pathFull;
	std::filesystem::path newPath = oldPath.parent_path() / newNamePath.filename();

	if (oldPath == newPath)
	{
		return;
	}

	if (std::filesystem::exists(newPath))
	{
		LogPrint("Файл с таким именем уже существует\r\n");
		return;
	}

	try
	{
		std::filesystem::rename(oldPath, newPath);
		LogPrint("Файл переименован\r\n");

		LoadFilenames();

		for (size_t i = 0; i < allFilenames.size(); ++i)
		{
			if (allFilenames[i].pathFull == newPath)
			{
				SelectFile((int)i);
				break;
			}
		}
	}
	catch (const std::filesystem::filesystem_error&)
	{
		LogPrint("Ошибка переименования файла\r\n");
	}
}


std::filesystem::path Main::BuildSavePath(const char* defaultPrefix)
{
	string fileName = TrimFileName(renameFileName);

	if (fileName.empty())
	{
		fileName = MakeTimestampFileName(defaultPrefix);
	}

	std::filesystem::path fileNamePath = PathFromUtf8(fileName);

	if (fileNamePath.filename() != fileNamePath)
	{
		fileName = MakeTimestampFileName(defaultPrefix);
		fileNamePath = PathFromUtf8(fileName);
	}

	std::filesystem::path savePath = std::filesystem::path(DirectoryName) / fileNamePath.filename();

	if (!std::filesystem::exists(savePath))
	{
		return savePath;
	}

	std::filesystem::path parentPath = savePath.parent_path();
	std::wstring baseName = savePath.stem().wstring();
	std::wstring extension = savePath.extension().wstring();

	for (int i = 2;; ++i)
	{
		std::filesystem::path candidate = parentPath / (baseName + L"_" + std::to_wstring(i) + extension);

		if (!std::filesystem::exists(candidate))
		{
			return candidate;
		}
	}
}


void Main::SelectFileByPath(const std::filesystem::path& filePath)
{
	for (size_t i = 0; i < allFilenames.size(); ++i)
	{
		if (allFilenames[i].pathFull == filePath)
		{
			SelectFile((int)i);
			break;
		}
	}
}


void Main::SaveSelectedObjectToNamedFile()
{
	if (!selectedObject)
	{
		LogPrint("Бот не выбран\r\n");
		return;
	}

	std::filesystem::path savePath = BuildSavePath("Bot");

	if (saver.SaveObject(selectedObject, savePath))
	{
		LogPrint("Объект сохранен\r\n");
		LoadFilenames();
		SelectFileByPath(savePath);
	}
	else
	{
		LogPrint("Ошибка сохранения объекта\r\n");
	}
}


void Main::SaveWorldToNamedFile()
{
	std::filesystem::path savePath = BuildSavePath("World");

	if (saver.SaveWorld(field, savePath, id, ticknum))
	{
		LogPrint("Мир сохранен\r\n");
		LoadFilenames();
		SelectFileByPath(savePath);
	}
	else
	{
		LogPrint("Ошибка сохранения мира\r\n");
	}
}


void Main::DeleteSelectedFile()
{
	if (!selectedFile)
	{
		LogPrint("Файл не выбран\r\n");
		return;
	}

	std::filesystem::path filePath = selectedFile->pathFull;

	std::wstring widePath = filePath.wstring();
	widePath.push_back(L'\0');

	SHFILEOPSTRUCTW fileOperation = {};
	fileOperation.wFunc = FO_DELETE;
	fileOperation.pFrom = widePath.c_str();
	fileOperation.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI;

	if (SHFileOperationW(&fileOperation) == 0 && !fileOperation.fAnyOperationsAborted)
	{
		LogPrint("Файл перемещен в корзину\r\n");
		LoadFilenames();
	}
	else
	{
		LogPrint("Ошибка удаления файла\r\n");
	}
}


Main::Main()
{
	LogPrint((char*)"Запущено. Seed:\r\n");

	//Set seed
	#ifdef RandomSeed		
		seed = GetTickCount();
	#else
		seed = Seed;
	#endif

	Field::seed = seed;
	srand(seed);

	id = rand();

	LogPrint(seed);

	field = new Field();

	#ifdef StartOnPause
		Pause();
	#endif

	LoadFilenames();

	keyboard = SDL_GetKeyboardState(NULL);
}

Main::~Main()
{
	Shutdown();
}

void Main::Shutdown()
{
	cb3Runtime.reset();

	delete field;
	field = NULL;
}

bool Main::IsClassicMode() const
{
	return activeMode == SimulationMode::Classic;
}

void Main::ResetClassicWorld()
{
	if (field)
	{
		delete field;
		field = NULL;
	}

	Deselect();
	ClearChart();
	Field::renderX = 0;
	Field::viewX = 0;
	Field::viewY = 0;
	Field::zoom = 1.0;
	ticknum = 0;
	tpsTickCounter = 0;
	realTPS = 0;
	realFPS = 0;
	fpsCounter = 0;
	timeBeforeNextDataToChart = AddToChartEvery;

#ifdef RandomSeed
	seed = GetTickCount();
#else
	seed = Seed;
#endif

	Field::seed = seed;
	srand(seed);
	id = rand();

	field = new Field();

#ifdef StartOnPause
	simulate = true;
	Pause();
#else
	simulate = true;
#endif

	LoadFilenames();
}

void Main::RequestSimulationMode(SimulationMode mode)
{
	if (mode == activeMode)
		return;

	pendingMode = mode;
	showModeSwitchConfirm = true;
}

void Main::SwitchSimulationMode(SimulationMode mode)
{
	if (mode == activeMode)
		return;

	Deselect();
	showSaveLoad = false;
	showDangerous = false;
	showBrain = false;
	showAdaptation = false;
	showChart = false;
	showInfo = false;
	saveFileNameInputActive = false;
	selectedFile = NULL;
	allFilenames.clear();

	if (mode == SimulationMode::CyberBiology3)
	{
		if (field)
		{
			delete field;
			field = NULL;
		}

		cb3Runtime = std::make_unique<Cb3Runtime>();
		activeMode = SimulationMode::CyberBiology3;
		simulate = true;
	}
	else
	{
		cb3Runtime.reset();
		activeMode = SimulationMode::Classic;
		ResetClassicWorld();
	}
}

void Main::CheckRuntimeRequests()
{
	if (!cb3Runtime)
		return;

	if (cb3Runtime->IsTerminated())
	{
		terminate = true;
		return;
	}

	SimulationMode requestedMode;
	if (cb3Runtime->ConsumeModeSwitchRequest(requestedMode))
	{
		SwitchSimulationMode(requestedMode);
	}
}

void Main::CatchKeyboard()
{
	if (!IsClassicMode())
	{
		if (cb3Runtime)
		{
			cb3Runtime->HandleKeyboard();
			CheckRuntimeRequests();
		}
		return;
	}

	if (saveFileNameInputActive)
	{
		return;
	}

	if (keyboard[Keyboard_Pause] || keyboard[Keyboard_Pause2])
	{
		Pause();
	}
	else if (keyboard[Keyboard_SpawnRandoms])
	{
		field->SpawnControlGroup();
	}
	else if (keyboard[Keyboard_PlaceWall])
	{
		repeat(FieldCellsHeight)
			field->AddObject(new Rock(0, i));
	}
	else if (keyboard[Keyboard_DropOrganics])
	{
		for (int X = 0; X < FieldCellsWidth; ++X)
		{
			for (int Y = 0; Y < 25 + RandomVal(20); ++Y)
			{
				field->AddObject(new Organics(X, Y, MaxPossibleEnergyForABot/2));
			}
		}
	}
	else if (keyboard[Keyboard_NextFrame])
	{
		if (!simulate)
		{
			MakeStep();
		}
	}
	else if (keyboard[Keyboard_RenderNatural])
	{
		renderType = natural;
	}
	else if (keyboard[Keyboard_RenderPredators])
	{
		renderType = predators;
	}
	else if (keyboard[Keyboard_RenderEnergy])
	{
		renderType = energy;
	}
	else if (keyboard[Keyboard_NoRender])
	{
		renderType = noRender;
	}
	else if (keyboard[SDL_SCANCODE_RIGHT])
	{
		field->shiftRenderPoint((io->KeyShift)	? MoveCameraFastSpeed : MoveCameraSpeed);
	}
	else if (keyboard[SDL_SCANCODE_LEFT])
	{
		field->shiftRenderPoint((io->KeyShift) ? -MoveCameraFastSpeed : -MoveCameraSpeed);
	}
	else if (keyboard[Keyboard_Reset_RenderX])
	{
		field->renderX = 0;
	}
	else if (keyboard[Keyboard_Jump_Up_RenderX])
	{
		field->shiftRenderPoint(MoveCameraJump);
	}
	else if (keyboard[Keyboard_Jump_Down_RenderX])
	{
		field->shiftRenderPoint(-MoveCameraJump);
	}
	else if (keyboard[Keyboard_Jump_To_First_bot])
	{
		field->jumpToFirstBot();
	}
	//Additional windows hotkeys
	else if (keyboard[Keyboard_ShowSaveLoad_Window])
	{
		LoadFilenames();

		showSaveLoad = !showSaveLoad;
	}
	else if (keyboard[Keyboard_ShowDangerous_Window])
	{
		showDangerous = !showDangerous;
	}
	else if (keyboard[Keyboard_ShowAdaptation_Window])
	{
		showAdaptation = !showAdaptation;
	}
	else if (keyboard[Keyboard_ShowChart_Window])
	{
		showChart = !showChart;
	}
	else if (keyboard[Keyboard_ShowBrain_Window])
	{
		showBrain = !showBrain;
	}	
}
