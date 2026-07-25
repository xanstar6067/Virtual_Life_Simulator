
//-----------------------------------------------------------------
// 
// 
// Примерно как устроено:
// - Входная точка в файле Main.cpp
// - Все настраиваемые параметры в файле Settings.h
// - Все объекты на поле наследуется от Object
// - Neuron.h это нейрон, BotNeuralNet.h - вся нейросеть
// - Field, это класс игрового поля
// 
// Также:
// - NeuralNetRenderer это класс, который рисует мозг бота в отдельном окошке
// - ObjectSaver - сохраняет объекты и мир в файл
// - ImageFactory - создает текстуры для объектов
// - AutomaticAdaptation - автоматизированный эксперимент адаптация
// 
//
//-----------------------------------------------------------------


#include "Main.h"

#include <algorithm>
#include <ctime>
#include <shellapi.h>

namespace cb3
{

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

void Main::SwitchPause()
{
	simulate = !simulate;
}

void Main::BrushIterate(Point p, void(*callback)(uint, uint, Field*))
{
	uint X = p.x, Y = p.y;

	for (int cx = -brushSize; cx < brushSize + 1; ++cx)
	{
		for (int cy = -brushSize; cy < brushSize + 1; ++cy)
		{
			if (field->IsInBounds(X + cx, Y + cy))
			{
				if ((brushSize * brushSize) > ((cx * cx) + (cy * cy)))
				{
					callback(X + cx, Y + cy, field);
				}
			}
		}
	}
}

void Main::Start()
{
	simulate = true;
}

void Main::Pause()
{
	simulate = false;
}

void Main::SpawnInitialPopulation()
{
	field->SpawnControlGroup();
	LogPrint("Добавлена стартовая группа ботов\r\n");
}

void Main::PlaceWorldWall()
{
	field->placeWall();
	LogPrint("Добавлена вертикальная стена\r\n");
}

void Main::DropWorldOrganics()
{
	for (int X = 0; X < FieldCellsWidth; ++X)
	{
		for (int Y = 0; Y < 25 + RandomVal(20); ++Y)
		{
			field->AddObject(new Organics(X, Y, BotMaxEnergyInitial / 2));
		}
	}

	LogPrint("В мир добавлена органика\r\n");
}

void Main::SpawnWorldRocks()
{
	for (int i = 0; i < SpawnRocksSize; ++i)
	{
		Rock* rock = new Rock(RandomVal(FieldCellsWidth), RandomVal(FieldCellsHeight));

		if (!field->AddObject(rock))
		{
			delete rock;
		}
	}

	LogPrint("В мир добавлены случайные камни\r\n");
}

void Main::MutateWholeWorld()
{
	field->mutateWorld();
	LogPrint("[Солнечная вспышка!]");
}

void Main::QuickSaveWorld()
{
	if (saver.SaveWorld(field, (char*)OuicksaveFilename, id, ticknum))
	{
		LogPrint("Мир сохранен\r\n");
		LoadFilenames();
	}
	else
	{
		LogPrint("Ошибка сохранения мира\r\n");
	}
}

void Main::QuickLoadWorld()
{
	Deselect();

	ObjectSaver::WorldParams ret = saver.LoadWorld(field, (char*)OuicksaveFilename);

	if (ret.id != -1)
	{
		if (ret.width != FieldCellsWidth)
			LogPrint("Мир загружен (ширина не совпадает)\r\n");
		else
			LogPrint("Мир загружен\r\n");

		seed = ret.seed;
		ticknum = ret.tick;
		id = ret.id;
		field->seed = seed;
	}
	else
	{
		LogPrint("Ошибка загрузки мира\r\n");
	}
}

bool Main::MakeStep()
{
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

	bool stepped = false;

	if ((TimeMSBetween(currentTick, prevTick) >= limit_interval) or (limit_interval == 0))
	{
		prevTick = currentTick;
		stepped = true;

		field->tick(ticknum);

		//Manage auto adaptation
		auto_adapt->AdaptationStep(ticknum);

		if (selectedObject &&
			(!field->ValidateObjectExistance(selectedObject) || selectedObject->type() != bot))
		{
			Deselect();
		}

		++ticknum;
		++tpsTickCounter;

		//Add data to chart
		if (chart.Tick())
		{
			chart.AddToChart(field->GetNumBots() * 1.0f,
				field->GetNumApples() * 1.0f,
				field->GetNumOrganics() * 1.0f,
				field->GetNumPredators() * 1.0f,
				field->GetAverageLifetime() * 1.0f);
		}
	}

	//Calculate simulation speed
	if (TimeMSBetween(currentTick, lastSecondTick) >= 1000)
	{
		lastSecondTick = currentTick;

		realTPS = tpsTickCounter;
		tpsTickCounter = 0;
	}

	return stepped;
}

bool Main::RunFrameSimulation()
{
	if (simulate)
	{
		return MakeStep();
	}

	return false;
}

void Main::HighlightSelection()
{
	if (selectionShadowScreen > 0)
	{
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

	if (cursorBlink-- == 0)
	{
		cursorBlink = CursorBlinkRate;
		cursorShow = !cursorShow;
	}
}

void Main::Deselect()
{
	if (field)
		field->TrackObject(NULL);

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
		std::filesystem::create_directories(DirectoryName);
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
		auto size = entry.file_size();
		auto originalSize = size;
		f.fileSize = FormatFileSize(size);
		f.modifiedTime = entry.last_write_time();
		f.modifiedTimeText = FormatFileTime(f.modifiedTime);

		//Is world (open file briefly and look for file type)
		MyInputStream file(f.pathFull, std::ios::in | std::ios::binary | std::ios::beg);

		if (!file.is_open())
			continue;

		int magicNumber = 0;

		if (originalSize > 0)
			magicNumber = file.ReadInt();

		int modeId = 0;

		if ((magicNumber == MagicNumber_WorldFile) || (magicNumber == MagicNumber_ObjectFile))
		{
			modeId = file.ReadInt();
		}

		f.mode = SimulationModeFromId(modeId);
		f.modeText = (modeId > 0) ? SimulationModeName(f.mode) : "-";
		f.isWorld = (magicNumber == MagicNumber_WorldFile);
		f.fileType = f.isWorld ? "мир" : ((magicNumber == MagicNumber_ObjectFile) ? "бот" : "файл");

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
		SelectFileByPath(newPath);
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

void Main::HandleKeyboard()
{
	CatchKeyboard();
}

void Main::HandleMouseClick()
{
	MouseClick();
}

void Main::HandleFieldNavigation()
{
	bool middleDown = (mouseState.buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
	bool mouseOverField = field->IsInBoundsScreenCoords(mouseState.mouseX, mouseState.mouseY);

	if ((mouseState.wheel != 0) and mouseOverField and !io->WantCaptureMouse)
	{
		field->ZoomAtScreenPoint(mouseState.mouseX, mouseState.mouseY, mouseState.wheel);
	}

	if (middleDown)
	{
		if (!fieldPanActive)
		{
			if (mouseOverField and !io->WantCaptureMouse)
			{
				fieldPanActive = true;
				fieldPanMouseX = mouseState.mouseX;
				fieldPanMouseY = mouseState.mouseY;
			}
		}
		else
		{
			field->PanView(mouseState.mouseX - fieldPanMouseX, mouseState.mouseY - fieldPanMouseY);
			fieldPanMouseX = mouseState.mouseX;
			fieldPanMouseY = mouseState.mouseY;
		}
	}
	else
	{
		fieldPanActive = false;
	}
}

bool Main::IsTerminated() const
{
	return terminate;
}

bool Main::ConsumeModeSwitchRequest(SimulationMode& mode)
{
	if (!modeSwitchRequested)
		return false;

	mode = requestedMode;
	modeSwitchRequested = false;
	return true;
}


Main::Main()
{
	LogPrint((char*)"Запущено. Зерно:\r\n");

	//Set seed and id
	#ifdef RandomSeed		
		seed = (uint)GetTickCount64();
	#else
		seed = Seed;
	#endif

	Field::seed = seed;
	srand(seed);

	id = rand();

	LogPrint(seed);

	field = new Field();
	Field::renderX = 0;
	Field::viewX = 0;
	Field::viewY = 0;
	Field::zoom = 1.0;
	auto_adapt = new AutomaticAdaptation(field, this);

	Pause();

	LoadFilenames();

	keyboard = SDL_GetKeyboardState(NULL);
}

Main::~Main()
{
	delete field;
	delete auto_adapt;
}

void Main::MainLoop()
{
	//Main loop	
	SDL_Event e;

	for (;!terminate;)
	{
		//Events
		mouseState.wheel = 0;

		while (SDL_PollEvent(&e) != 0)
		{
			if (e.type == SDL_QUIT)
			{
				return;
			}
			else if (e.type == SDL_MOUSEWHEEL)
			{
				mouseState.wheel = e.wheel.y;
			}
			else if (e.type == SDL_KEYDOWN)
			{
				CatchKeyboard();
			}
			else if (e.type == SDL_TEXTINPUT)
			{
				io->AddInputCharacter(*e.text.text);
			}
			else if (e.type == SDL_WINDOWEVENT)
			{
				if(e.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
				{
					windowIsVisible = false;
				}
				else if (e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
				{
					windowIsVisible = true;
				}
			}

		}

		//Mouse down event
		if (ReadMouseState())
		{
			MouseClick();
		}

		//Simulation
		bool didWork = false;

		if (simulate)
		{
			didWork = MakeStep();
		}

		if(windowIsVisible)
			didWork = Render() || didWork;

		if (!didWork)
			SDL_Delay(1);
	}
}

void Main::CatchKeyboard()
{
	if (saveFileNameInputActive)
	{
		return;
	}

	if (keyboard[Keyboard_Pause] || keyboard[Keyboard_Pause2])
	{
		SwitchPause();
	}
	else if (keyboard[Keyboard_SpawnRandoms])
	{
		SpawnInitialPopulation();
	}
	else if (keyboard[Keyboard_PlaceWall])
	{
		PlaceWorldWall();
	}
	else if (keyboard[Keyboard_DropOrganics])
	{
		DropWorldOrganics();
	}
	else if (keyboard[Keyboard_SpawnRocks])
	{
		SpawnWorldRocks();
	}
	else if (keyboard[Keyboard_MutateScreen])
	{
		MutateWholeWorld();
	}
	else if (keyboard[Keyboard_Quicksave])
	{
		QuickSaveWorld();
	}
	else if (keyboard[Keyboard_Quickload])
	{
		QuickLoadWorld();
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
	else if (keyboard[Keyboard_ShowAutomaticAdaptation_Window])
	{
		showAutomaticAdaptation = !showAutomaticAdaptation;
	}
}


}
