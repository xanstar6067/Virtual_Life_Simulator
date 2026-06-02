
#include "GUI.h"



void InitImGUI()
{
	ImGui::CreateContext();

	io = &ImGui::GetIO();
	io->Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 16.0f, NULL, io->Fonts->GetGlyphRangesCyrillic());

	ImPlot::CreateContext();

	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	//Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer backends
	ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
	ImGui_ImplSDLRenderer_Init(renderer);
}

void DeInitImGUI()
{
	ImGui_ImplSDLRenderer_Shutdown();
	ImPlot::DestroyContext();
	ImGui::DestroyContext();
}



void GUIStartFrame()
{
	ImGui_ImplSDLRenderer_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
}



void Main::DrawDemoWindow()
{
	ImGui::SetNextWindowBgAlpha(1.0f);
	ImGui::SetNextWindowPos({ 20.0f,20.0f });

	ImGui::ShowDemoWindow();
}

void Main::DrawMainWindow()
{
	ImGui::SetNextWindowBgAlpha(1.0f);
	ImGui::SetNextWindowSize({ GUIWindowWidth * 1.0f, 135.0f });
	ImGui::SetNextWindowPos({ (2 * FieldX + FieldWidth) * 1.0f, InterfaceBorder * 1.0f });

	ImGui::Begin("Главное", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
	{
		//FPS text 
		ImGui::Text("шаги: %i", ticknum);
		ImGui::Text("(интервал %i, тиков/с: %i, кадров/с: %i)", limit_interval, realTPS, realFPS);
		ImGui::Text("Всего объектов: %i", field->GetNumObjects());
		ImGui::Text("Всего ботов: %i", field->GetNumBots());

		//Show season name
		/*
		switch (season)
		{
		case summer:
			ImGui::Text("Сезон: лето");
			break;
		case autumn:
			ImGui::Text("Сезон: осень");
			break;
		case winter:
			ImGui::Text("Сезон: зима");
			break;
		case spring:
			ImGui::Text("Сезон: весна");
			break;
		}

		ImGui::SameLine();
		ImGui::Text(" ( %i )", ChangeSeasonInterval - changeSeasonCounter);*/

		//Neural net params and FOV x
		ImGui::Text("Слоев: %i, нейронов: %i, сдвиг X: %i", NumNeuronLayers, NeuronsInLayer, field->renderX);

		//Simulation seed and unique id
		ImGui::Text("Зерно: %i, id симуляции: %i", seed, id);

	}
	ImGui::End();
}


void Main::DrawSystemWindow()
{
	ImGui::SetNextWindowBgAlpha(1.0f);
	ImGui::SetNextWindowSize({ GUIWindowWidth * 1.0f, 70.0f });
	ImGui::SetNextWindowPos({ (2 * FieldX + FieldWidth) * 1.0f, 140.0f });

	ImGui::Begin("Система", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
	{
		ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "Платформа");
		ImGui::SameLine();
		ImGui::Text(" %s", SDL_GetPlatform());

		ImGui::SameLine();

		ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "Ядер процессора: %d", SDL_GetCPUCount());

		ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "Память: %.2f ГБ", SDL_GetSystemRAM() / 1024.0f);

		ImGui::SameLine();

		ImGui::Text(", потоков: %u", field->GetNumThreads());
	}
	ImGui::End();
}

void Main::DrawControlsWindow()
{
	ImGui::SetNextWindowBgAlpha(1.0f);
	ImGui::SetNextWindowSize({ GUIWindowWidth * 1.0f, 160.0f });
	ImGui::SetNextWindowPos({ (2 * FieldX + FieldWidth) * 1.0f, 220.0f });

	ImGui::Begin("Управление", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
	{
		if (ImGui::Button((simulate) ? "Стоп" : "Старт", { 200, 25 }))
		{
			Pause();
		}

		//Sliders
		ImGui::PushItemWidth(200);
		ImGui::SliderInt("лимит тиков", &limit_ticks_per_second, 0, GUI_Max_tps, "%d");
		ImGui::SliderInt("лимит кадров", &limitFPS, 0, GUI_Max_fps, "%d");

		ImGui::SliderInt("энергия ФС", &(field->photosynthesisReward), 0, GUI_Max_food);
		ImGui::SliderInt("кисть", &brushSize, GUI_Max_brush, 0, "%d");
	}
	ImGui::End();
}

void Main::DrawSelectionWindow()
{
	ImGui::SetNextWindowBgAlpha(1.0f);
	ImGui::SetNextWindowSize({ GUIWindowWidth * 1.0f, 150.0f });
	ImGui::SetNextWindowPos({ (2 * FieldX + FieldWidth) * 1.0f, 390.0f });

	ImGui::Begin("Выбор", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
	{
		if (selectedObject)
		{
			if (field->ValidateObjectExistance(selectedObject))
			{
				//Object info
				ImGui::Text("тип: бот	X: %i, Y: %i", selectedObject->x, selectedObject->y);
				ImGui::Text("возраст: %i / %i", selectedObject->GetLifetime(), MaxBotLifetime);
				ImGui::Text("энергия: %i (ФС: %i, охота: %i)", selectedObject->energy, ((Bot*)selectedObject)->GetEnergyFromPS(), ((Bot*)selectedObject)->GetEnergyFromKills());

				//Mutation markers
				int m[NumberOfMutationMarkers];
				memcpy(m, ((Bot*)selectedObject)->GetMarkers(), sizeof(m));
				ImGui::Text("метки: {");

				repeat(NumberOfMutationMarkers)
				{
					ImGui::SameLine();
					ImGui::Text("%i", m[i]);
				}

				ImGui::SameLine();
				ImGui::Text("}");

				//Color
				Uint8 c[3];

				memcpy(c, ((Bot*)selectedObject)->GetColor(), sizeof(c));
				ImGui::Text("цвет: {%i, %i, %i}", c[0], c[1], c[2]);

				ImGui::SameLine();
				ImGui::TextColored(ImVec4(((c[0] * 1.0f) / 255.0f), ((c[1] * 1.0f) / 255.0f), ((c[2] * 1.0f) / 255.0f), 1.0f), "*****");

				ImGui::SameLine();
				if (ImGui::Button("Новый", { 55, 20 }))
				{
					field->RepaintBot((Bot*)selectedObject, Bot::GetRandomColor(), 1);
				}

				if (ImGui::Button("Показать мозг", { 120, 25 }))
				{
					showBrain = !showBrain;
				}
			}
			else
			{
				Deselect();
			}
		}
	}
	ImGui::End();
}

void Main::DrawRenderWindow()
{
	ImGui::SetNextWindowBgAlpha(1.0f);
	ImGui::SetNextWindowSize({ GUIWindowWidth * 1.0f, 140.0f});
	ImGui::SetNextWindowPos({ (2 * FieldX + FieldWidth) * 1.0f, 550.0f });

	ImGui::Begin("Вид", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
	{
		ImGui::BeginGroup();

		ImGui::Text("Режим:");

		if (ImGui::BeginTable("##RenderModes", 2))
		{
			ImGui::TableSetupColumn("##RenderModeLeft", ImGuiTableColumnFlags_WidthFixed, 105.0f);
			ImGui::TableSetupColumn("##RenderModeRight", ImGuiTableColumnFlags_WidthFixed, 105.0f);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::RadioButton("Обычный", (int*)&renderType, 0);
			ImGui::TableSetColumnIndex(1);
			ImGui::RadioButton("Охота", (int*)&renderType, 1);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::RadioButton("Энергия", (int*)&renderType, 2);
			ImGui::TableSetColumnIndex(1);
			ImGui::RadioButton("Без отрис.", (int*)&renderType, 3);

			ImGui::EndTable();
		}

		ImGui::EndGroup();
	}
	ImGui::End();
}

void Main::DrawConsoleWindow()
{
	ImGui::SetNextWindowBgAlpha(1.0f);
	ImGui::SetNextWindowSize({ GUIWindowWidth * 1.0f, 120.0f });
	ImGui::SetNextWindowPos({ (2 * FieldX + FieldWidth) * 1.0f, 700.0f});

	ImGui::Begin("Журнал", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
	{
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(LogBackgroundColor));

		ImGui::BeginChild("scrolling", ImVec2(240, 80), true);
		{
			ImGui::TextUnformatted(logText.Buf.Data);

			if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
				ImGui::SetScrollHereY(1.0f);
		}
		ImGui::EndChild();

		ImGui::PopStyleColor();
	}
	ImGui::End();
}

void Main::DrawMouseFunctionWindow()
{
	ImGui::SetNextWindowBgAlpha(1.0f);
	ImGui::SetNextWindowSize({ GUIWindowWidth * 1.0f, 130.0f});
	ImGui::SetNextWindowPos({ (2 * FieldX + FieldWidth) * 1.0f, 830.0f });

	ImGui::Begin("Действие мыши", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
	{
		ImGui::BeginGroup();

		if (ImGui::BeginTable("##MouseFunctions", 2))
		{
			ImGui::TableSetupColumn("##MouseFunctionLeft", ImGuiTableColumnFlags_WidthFixed, 105.0f);
			ImGui::TableSetupColumn("##MouseFunctionRight", ImGuiTableColumnFlags_WidthFixed, 105.0f);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::RadioButton("Выбрать", (int*)&mouseFunc, 0);
			ImGui::TableSetColumnIndex(1);
			ImGui::RadioButton("Удалить", (int*)&mouseFunc, 1);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::RadioButton("Камень", (int*)&mouseFunc, 2);
			ImGui::TableSetColumnIndex(1);
			ImGui::RadioButton("Из файла", (int*)&mouseFunc, 3);

			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::RadioButton("Мутировать", (int*)&mouseFunc, 4);

			ImGui::EndTable();
		}

		ImGui::EndGroup();
	}
	ImGui::End();
}

void Main::DrawAdditionalsWindow()
{
	ImGui::SetNextWindowBgAlpha(1.0f);
	ImGui::SetNextWindowSize({ 320.0f, 115.0f });
	ImGui::SetNextWindowPos({ (2 * FieldX + FieldWidth) * 1.0f, 945.0f });

	ImGui::Begin("Окна", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
	{

		const ImVec2 windowButtonSize = { 92, 28 };

		if (ImGui::Button("Файлы", windowButtonSize))
		{
			LoadFilenames();

			showSaveLoad = !showSaveLoad;
		}
		ImGui::SameLine();

		if (ImGui::Button("Инструменты", windowButtonSize))
		{
			showDangerous = !showDangerous;
		}
		ImGui::SameLine();

		if (ImGui::Button("Среда", windowButtonSize))
		{
			showAdaptation = !showAdaptation;
		}

		if (ImGui::Button("График", windowButtonSize))
		{
			showChart = !showChart;
		}
		ImGui::SameLine();

		if (ImGui::Button("Инфо", windowButtonSize))
		{
			showInfo = !showInfo;
		}

		ImGui::SameLine();

		if (ImGui::Button("Выход", windowButtonSize))
		{
			showExitConfirm = true;
		}

	}
	ImGui::End();
}

static float GetSidePanelX()
{
	float fixedX = (2 * FieldX + FieldWidth) * 1.0f;
	float maxVisibleX = windowWidth - GUIWindowWidth * 1.0f - InterfaceBorder * 1.0f;

	if (maxVisibleX < InterfaceBorder)
	{
		maxVisibleX = InterfaceBorder * 1.0f;
	}

	return (fixedX < maxVisibleX) ? fixedX : maxVisibleX;
}

void Main::DrawSidePanelWindow()
{
	float panelHeight = windowHeight - InterfaceBorder * 2.0f;

	if (panelHeight < 200.0f)
	{
		panelHeight = 200.0f;
	}

	ImGui::SetNextWindowBgAlpha(1.0f);
	ImGui::SetNextWindowSize({ GUIWindowWidth * 1.0f, panelHeight });
	ImGui::SetNextWindowPos({ GetSidePanelX(), InterfaceBorder * 1.0f });

	ImGui::Begin("Панель", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
	{
		if (ImGui::CollapsingHeader("Главное", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Text("шаги: %i", ticknum);
			ImGui::Text("(интервал %i, тиков/с: %i, кадров/с: %i)", limit_interval, realTPS, realFPS);
			ImGui::Text("Всего объектов: %i", field->GetNumObjects());
			ImGui::Text("Всего ботов: %i", field->GetNumBots());
			ImGui::Text("Слоев: %i, нейронов: %i, сдвиг X: %i", NumNeuronLayers, NeuronsInLayer, field->renderX);
			ImGui::Text("Зерно: %i, id симуляции: %i", seed, id);
		}

		if (ImGui::CollapsingHeader("Система", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "Платформа");
			ImGui::SameLine();
			ImGui::Text(" %s", SDL_GetPlatform());

			ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "Ядер процессора: %d", SDL_GetCPUCount());
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "Память: %.2f ГБ", SDL_GetSystemRAM() / 1024.0f);
			ImGui::Text(", потоков: %u", field->GetNumThreads());
		}

		if (ImGui::CollapsingHeader("Управление", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (ImGui::Button((simulate) ? "Стоп" : "Старт", { 200, 25 }))
			{
				Pause();
			}

			ImGui::PushItemWidth(200);
			ImGui::SliderInt("лимит тиков", &limit_ticks_per_second, 0, GUI_Max_tps, "%d");
			ImGui::SliderInt("лимит кадров", &limitFPS, 0, GUI_Max_fps, "%d");
			ImGui::SliderInt("энергия ФС", &(field->photosynthesisReward), 0, GUI_Max_food);
			ImGui::SliderInt("кисть", &brushSize, GUI_Max_brush, 0, "%d");
			ImGui::PopItemWidth();
		}

		if (ImGui::CollapsingHeader("Выбор", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (selectedObject)
			{
				if (field->ValidateObjectExistance(selectedObject))
				{
					ImGui::Text("тип: бот	X: %i, Y: %i", selectedObject->x, selectedObject->y);
					ImGui::Text("возраст: %i / %i", selectedObject->GetLifetime(), MaxBotLifetime);
					ImGui::Text("энергия: %i (ФС: %i, охота: %i)", selectedObject->energy, ((Bot*)selectedObject)->GetEnergyFromPS(), ((Bot*)selectedObject)->GetEnergyFromKills());

					int m[NumberOfMutationMarkers];
					memcpy(m, ((Bot*)selectedObject)->GetMarkers(), sizeof(m));
					ImGui::Text("метки: {");

					repeat(NumberOfMutationMarkers)
					{
						ImGui::SameLine();
						ImGui::Text("%i", m[i]);
					}

					ImGui::SameLine();
					ImGui::Text("}");

					Uint8 c[3];
					memcpy(c, ((Bot*)selectedObject)->GetColor(), sizeof(c));
					ImGui::Text("цвет: {%i, %i, %i}", c[0], c[1], c[2]);

					ImGui::SameLine();
					ImGui::TextColored(ImVec4(((c[0] * 1.0f) / 255.0f), ((c[1] * 1.0f) / 255.0f), ((c[2] * 1.0f) / 255.0f), 1.0f), "*****");

					ImGui::SameLine();
					if (ImGui::Button("Новый", { 55, 20 }))
					{
						field->RepaintBot((Bot*)selectedObject, Bot::GetRandomColor(), 1);
					}

					if (ImGui::Button("Показать мозг", { 120, 25 }))
					{
						showBrain = !showBrain;
					}
				}
				else
				{
					Deselect();
				}
			}
		}

		if (ImGui::CollapsingHeader("Вид", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Text("Режим:");

			if (ImGui::BeginTable("##RenderModesPanel", 2))
			{
				ImGui::TableSetupColumn("##RenderModeLeft", ImGuiTableColumnFlags_WidthFixed, 105.0f);
				ImGui::TableSetupColumn("##RenderModeRight", ImGuiTableColumnFlags_WidthFixed, 105.0f);

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::RadioButton("Обычный", (int*)&renderType, 0);
				ImGui::TableSetColumnIndex(1);
				ImGui::RadioButton("Охота", (int*)&renderType, 1);

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::RadioButton("Энергия", (int*)&renderType, 2);
				ImGui::TableSetColumnIndex(1);
				ImGui::RadioButton("Без отрис.", (int*)&renderType, 3);

				ImGui::EndTable();
			}
		}

		if (ImGui::CollapsingHeader("Журнал", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(LogBackgroundColor));

			ImGui::BeginChild("scrolling_panel", ImVec2(0, 80), true);
			{
				ImGui::TextUnformatted(logText.Buf.Data);

				if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
					ImGui::SetScrollHereY(1.0f);
			}
			ImGui::EndChild();

			ImGui::PopStyleColor();
		}

		if (ImGui::CollapsingHeader("Действие мыши", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (ImGui::BeginTable("##MouseFunctionsPanel", 2))
			{
				ImGui::TableSetupColumn("##MouseFunctionLeft", ImGuiTableColumnFlags_WidthFixed, 105.0f);
				ImGui::TableSetupColumn("##MouseFunctionRight", ImGuiTableColumnFlags_WidthFixed, 105.0f);

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::RadioButton("Выбрать", (int*)&mouseFunc, 0);
				ImGui::TableSetColumnIndex(1);
				ImGui::RadioButton("Удалить", (int*)&mouseFunc, 1);

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::RadioButton("Камень", (int*)&mouseFunc, 2);
				ImGui::TableSetColumnIndex(1);
				ImGui::RadioButton("Из файла", (int*)&mouseFunc, 3);

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::RadioButton("Мутировать", (int*)&mouseFunc, 4);

				ImGui::EndTable();
			}
		}

		if (ImGui::CollapsingHeader("Окна", ImGuiTreeNodeFlags_DefaultOpen))
		{
			const ImVec2 windowButtonSize = { 92, 28 };

			if (ImGui::Button("Файлы", windowButtonSize))
			{
				LoadFilenames();
				showSaveLoad = !showSaveLoad;
			}
			ImGui::SameLine();

			if (ImGui::Button("Инструменты", windowButtonSize))
			{
				showDangerous = !showDangerous;
			}

			if (ImGui::Button("Среда", windowButtonSize))
			{
				showAdaptation = !showAdaptation;
			}
			ImGui::SameLine();

			if (ImGui::Button("График", windowButtonSize))
			{
				showChart = !showChart;
			}

			if (ImGui::Button("Инфо", windowButtonSize))
			{
				showInfo = !showInfo;
			}
			ImGui::SameLine();

			if (ImGui::Button("Выход", windowButtonSize))
			{
				showExitConfirm = true;
			}
		}
	}
	ImGui::End();
}

void Main::DrawSaveLoadWindow()
{
	if (showSaveLoad)
	{
		//Save/load window
		ImGui::SetNextWindowBgAlpha(1.0f);
		ImGui::SetNextWindowSize({ 520.0f, 260.0f });
		ImGui::SetNextWindowPos({ 100 * 1.0f, 100.0f }, ImGuiCond_Once);

		ImGui::Begin("Сохранение и загрузка", &showSaveLoad, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
		{
			//List of files
			ImGui::Text("Выберите файл");

			if (ImGui::BeginTable("##SaveFiles", 4, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(500, 110)))
			{
				ImGui::TableSetupColumn("Имя", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("Размер", ImGuiTableColumnFlags_WidthFixed, 70.0f);
				ImGui::TableSetupColumn("Тип", ImGuiTableColumnFlags_WidthFixed, 55.0f);
				ImGui::TableSetupColumn("Дата", ImGuiTableColumnFlags_WidthFixed, 125.0f);
				ImGui::TableHeadersRow();

				for (int i = 0; i < allFilenames.size(); ++i)
				{
					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex(0);
					if (ImGui::Selectable(allFilenames[i].nameShort.c_str(), allFilenames[i].isSelected, ImGuiSelectableFlags_SpanAllColumns))
					{
						SelectFile(i);
					}

					ImGui::TableSetColumnIndex(1);
					ImGui::TextUnformatted(allFilenames[i].fileSize.c_str());

					ImGui::TableSetColumnIndex(2);
					ImGui::TextUnformatted(allFilenames[i].fileType.c_str());

					ImGui::TableSetColumnIndex(3);
					ImGui::TextUnformatted(allFilenames[i].modifiedTimeText.c_str());
				}

				ImGui::EndTable();
			}

			ImGui::Text("Имя файла");
			ImGui::SameLine();
			ImGui::PushItemWidth(295);
			ImGui::InputText("##RenameFileName", renameFileName, sizeof(renameFileName));
			ImGui::PopItemWidth();

			ImGui::SameLine();

			if (ImGui::Button("Переименовать", { 120, 25 }))
			{
				RenameSelectedFile();
			}

			//Buttons

			const ImVec2 fileButtonSize = { 120, 26 };

			if (ImGui::Button("Загрузить", fileButtonSize))
			{
				if (selectedFile)
				{
					if (selectedFile->isWorld)
					{
						ObjectSaver::WorldParams ret = saver.LoadWorld(field, selectedFile->pathFull);

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
							LogPrint("Ошибка загрузки мира\r\n");
					}
					else
					{
						if (selectedObject)
						{
							delete selectedObject;
						}

						selectedObject = saver.LoadObject(selectedFile->pathFull);

						if (selectedObject)
							LogPrint("Объект загружен\r\n");
						else
							LogPrint("Ошибка загрузки объекта\r\n");
					}
				}
			}			

			ImGui::SameLine();

			if (ImGui::Button("Сохр. бота", fileButtonSize))
			{
				SaveSelectedObjectToNamedFile();
			}

			ImGui::SameLine();

			if (ImGui::Button("Сохр. мир", fileButtonSize))
			{
				SaveWorldToNamedFile();
			}

			ImGui::SameLine();

			if (ImGui::Button("Удалить", fileButtonSize))
			{
				DeleteSelectedFile();
			}
		}
		ImGui::End();
	}
}

void Main::DrawDangerousWindow()
{
	if (showDangerous)
	{
		ImGui::SetNextWindowBgAlpha(1.0f);
		ImGui::SetNextWindowSize({ 290.0f, 95.0f });
		ImGui::SetNextWindowPos({ 100 * 1.0f, 300.0f }, ImGuiCond_Once);

		ImGui::Begin("Инструменты", &showDangerous, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
		{

			if (ImGui::Button("Очистить мир", { 130, 30 }))
			{
				ClearWorld();
			}

			ImGui::SameLine();

			if (ImGui::Button("Убить ботов", { 130, 30 }))
			{
				Deselect();

				for (int cx = 0; cx < FieldCellsWidth; ++cx)
				{
					for (int cy = 0; cy < FieldCellsHeight; ++cy)
					{
						Object* o = field->GetObjectLocalCoords(cx, cy);

						if (o == NULL)
							continue;

						if (o->type == bot)
							field->RemoveObject(cx, cy);
					}
				}
			}

			if (ImGui::Button("Обнулить время", { 130, 30 }))
			{
				ticknum = 0;
			}
		}
		ImGui::End();
	}
}

void Main::DrawSummaryWindow()
{
	if (showBrain)
	{
		ImGui::SetNextWindowBgAlpha(1.0f);
		ImGui::SetNextWindowSize({ 330.0f, 180.0f });
		ImGui::SetNextWindowPos({ 100 * 1.0f, 150.0f }, ImGuiCond_Once);

		ImGui::Begin("Сводка бота", &showBrain, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
		{
			if (selectedObject)
			{
				if (field->ValidateObjectExistance(selectedObject))
				{
					auto summ = ((Bot*)selectedObject)->GetNeuronSummary();

					ImGui::Text("Нейроны:");
					ImGui::Text("Простые: %i", summ.simple);
					ImGui::Text("Радиальные: %i", summ.radialBasis);
					ImGui::Text("Случайные: %i", summ.random);
					ImGui::Text("Память: %i", summ.memory);

					ImGui::NewLine();

					ImGui::Text("Всего нейронов: %i, тупиков: %i", summ.neurons, summ.deadend);
					ImGui::Text("Всего связей: %i", summ.connections);
				}
				else
					goto Nothing;

			}
			else
			{
			Nothing:
				ImGui::Text("Ничего не выбрано");
			}

		}
		ImGui::End();
	}
}

void Main::DrawAdaptationWindow()
{
	if (showAdaptation)
	{
		ImGui::SetNextWindowBgAlpha(1.0f);
		ImGui::SetNextWindowSize({ 500.0f, 500.0f });
		ImGui::SetNextWindowPos({ 100 * 1.0f, 250.0f }, ImGuiCond_Once);

		ImGui::Begin("Условия среды", &showAdaptation, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
		{
			if(ImGui::CollapsingHeader("Ветры"))
			{
				ImGui::SliderInt("Фаза", &field->params.adaptation_DeathChance_Winds, 0, 1000);
				ImGui::SliderInt("Шаги", &field->params.adaptation_StepsNum_Winds, 0, 20);
			}

			ImGui::NewLine();
			
			if (ImGui::CollapsingHeader("Вода и суша", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::SliderInt("Запрет размн. на суше", &field->params.adaptation_landBirthBlock, 0, 1000);
				ImGui::SliderInt("Запрет размн. в море", &field->params.adaptation_seaBirthBlock, 0, 1000);

				ImGui::NewLine();

				ImGui::SliderInt("Нет ФС в океане", &field->params.adaptation_PSInOceanBlock, 0, 1000, "%d");
				ImGui::SliderInt("Нет ФС в грязи", &field->params.adaptation_PSInMudBlock, 0, 1000, "%d");
				ImGui::SliderInt("Был на суше", &field->params.adaptation_botShouldBeOnLandOnceToMultiply, 0, 1000, "%d");
				ImGui::SliderInt("ФС на суше", &field->params.adaptation_botShouldDoPSOnLandOnceToMultiply, 0, 1000, "%d");
				ImGui::SliderInt("Движение по Y", &field->params.adaptation_forceBotMovements, 0, 1000);

				ImGui::NewLine();

				ImGui::SliderInt("Уровень океана", &field->params.oceanLevel, 0, FieldCellsHeight);
				ImGui::SliderInt("Уровень грязи", &field->params.mudLevel, 0, FieldCellsHeight);
			}

			ImGui::NewLine();

			if (ImGui::CollapsingHeader("Органика"))
			{
				ImGui::SliderInt("Появление органики", &field->params.adaptation_organicSpawnRate, 0, 1000);
			}

			ImGui::NewLine();

			if (ImGui::CollapsingHeader("Яблоки"))
			{
				ImGui::SliderInt("Энергия яблока", &field->params.appleEnergy, 1, 200);

				ImGui::Checkbox("Создавать яблоки", &field->params.spawnApples);
			}

			ImGui::NewLine();

			if (ImGui::Button("Сброс", { 70, 20 }))
			{
				field->params.Reset();
			}
		}
		ImGui::End();
	}
}

void Main::DrawChartWindow()
{
	if (showChart)
	{
		ImGui::SetNextWindowBgAlpha(1.0f);
		ImGui::SetNextWindowSize({ 900.0f, 600.0f });
		ImGui::SetNextWindowPos({ 700.0f, 250.0f }, ImGuiCond_Once);

		ImGui::Begin("График популяции", &showChart, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
		{
			if (ImPlot::BeginPlot("Объекты", { 800, 550 }))
			{
				
				//Axes
				ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, 250.0, ImPlotCond_Always);
				ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, 26000.0);

				ImPlot::SetupAxis(ImAxis_X1, "Время");
				ImPlot::SetupAxis(ImAxis_Y1, "Количество");

				//Bots number
				ImPlot::SetNextLineStyle({ 1, 0, 0, 1 }, ChartLineThickness);

				ImPlot::PlotLine("Боты", chartData_bots, chart_numValues - 1, 1.0f, 0.0f, ImPlotLineFlags_None);

				//Apples number
				if(chartShow_apples)
				{
					ImPlot::SetNextLineStyle({ 0, 1, 0, 1 }, ChartLineThickness);

					ImPlot::PlotLine("Яблоки", chartData_apples, chart_numValues - 1, 1.0f, 0.0f, ImPlotLineFlags_None);
				}

				//Organics number
				if(chartShow_organics)
				{
					ImPlot::SetNextLineStyle({ 0, 0, 1, 1 }, ChartLineThickness);

					ImPlot::PlotLine("Органика", chartData_organics, chart_numValues - 1, 1.0f, 0.0f, ImPlotLineFlags_None);
				}

				ImPlot::EndPlot();
			}

			ImGui::SameLine();

			ImGui::BeginGroup();			

			if (ImGui::Button("Очистить", { 80.0f, 30.0f }))
				ClearChart();

			ImGui::Checkbox("Яблоки", &chartShow_apples);
			ImGui::Checkbox("Органика", &chartShow_organics);

			ImGui::EndGroup();
		}
		ImGui::End();
	}
}

void Main::DrawInfoWindow()
{
	if (showInfo)
	{
		ImGui::SetNextWindowBgAlpha(1.0f);
		ImGui::SetNextWindowSize({ 560.0f, 520.0f });
		ImGui::SetNextWindowPos({ 140.0f, 120.0f }, ImGuiCond_Once);

		ImGui::Begin("Инфо", &showInfo, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
		{
			ImGui::Text("Горячие клавиши");

			if (ImGui::BeginTable("##Hotkeys", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(540, 465)))
			{
				ImGui::TableSetupColumn("Клавиша", ImGuiTableColumnFlags_WidthFixed, 150.0f);
				ImGui::TableSetupColumn("Действие", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableHeadersRow();

				struct hotkey_info
				{
					const char* key;
					const char* action;
				};

				const hotkey_info hotkeys[] =
				{
					{ "Space или Pause", "Старт/стоп симуляции" },
					{ "Num +", "Один шаг вперед, если симуляция на паузе" },
					{ "F1", "Создать стартовую группу случайных ботов" },
					{ "F2", "Поставить вертикальную стену из камней в X=0" },
					{ "F3", "Насыпать органику в верхней части мира" },
					{ "Num 1", "Обычный режим вида" },
					{ "Num 2", "Режим \"Охота\"" },
					{ "Num 3", "Режим \"Энергия\"" },
					{ "Num 4", "Без отрисовки поля" },
					{ "Left / Right", "Двигать камеру по X" },
					{ "Shift + Left / Right", "Двигать камеру быстрее" },
					{ "Home", "Сбросить сдвиг камеры к X=0" },
					{ "PageUp / PageDown", "Прыжок камеры по X" },
					{ "End", "Прыгнуть к первому найденному боту" },
					{ "Z", "Окно файлов" },
					{ "X", "Окно опасных действий" },
					{ "C", "Окно среды" },
					{ "V", "Окно графика" },
					{ "B", "Окно мозга выбранного бота" },
				};

				for (int i = 0; i < sizeof(hotkeys) / sizeof(hotkeys[0]); ++i)
				{
					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex(0);
					ImGui::TextUnformatted(hotkeys[i].key);

					ImGui::TableSetColumnIndex(1);
					ImGui::TextUnformatted(hotkeys[i].action);
				}

				ImGui::EndTable();
			}
		}
		ImGui::End();
	}
}

void Main::DrawExitConfirmWindow()
{
	if (showExitConfirm)
	{
		ImGui::OpenPopup("Подтверждение выхода");
		showExitConfirm = false;
	}

	if (ImGui::BeginPopupModal("Подтверждение выхода", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Выйти из приложения?");

		if (ImGui::Button("Да", { 90, 30 }))
		{
			terminate = true;
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		if (ImGui::Button("Нет", { 90, 30 }))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void Main::DrawBotBrainWindow()
{	
	if (showBrain)
	{
		if (selectedObject)
		{
			//Bot brain window
			ImGui::SetNextWindowBgAlpha(1.0f);
			ImGui::SetNextWindowSize({ 330.0f, 240.0f });
			ImGui::SetNextWindowPos({ 650 * 1.0f, 350.0f });

			ImGui::Begin("Данные мозга", &showBrain, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
			{
				ImGui::BeginGroup();

				ImGui::RadioButton("Текущий мозг", &brainToShow, 0);
				ImGui::RadioButton("Начальный мозг", &brainToShow, 1);

				ImGui::EndGroup();

				if (nn_renderer.selectedNeuron)
				{
					//Show neuron type
					ImGui::Text("Тип нейрона:");
					ImGui::SameLine();
					ImGui::Text(Neuron::GetTextFromType(nn_renderer.selectedNeuron->type));

					//Show bias
					ImGui::Text("Смещение: %f", nn_renderer.selectedNeuron->bias);

					//Show connections
					repeat(nn_renderer.selectedNeuron->numConnections)
					{
						ImGui::Text("Связь к слою: %i, нейрону: %i, вес: %f", nn_renderer.selectedNeuron->allConnections[i].dest_layer,
							nn_renderer.selectedNeuron->allConnections[i].dest_neuron, nn_renderer.selectedNeuron->allConnections[i].weight);
					}

					//Show memory data
					//ImGui::Text("Memory data: %i", nn_renderer.selectedBrain->allMemory[][]);

					//TODO
					if((nn_renderer.selectedNeuron->type!=input) && (nn_renderer.selectedNeuron->type != output))
					{
						if (ImGui::Button("случайно", { 100,30 }))
						{
							nn_renderer.selectedNeuron->SetRandom();
						}

						ImGui::SameLine();

						if (ImGui::Button("обнулить", { 100,30 }))
						{
							nn_renderer.selectedNeuron->SetZero();
						}
					}
				}
				else
				{
					ImGui::Text("Ничего не выбрано");
				}
			}
			ImGui::End();

			if(brainToShow == 0)
				nn_renderer.Render(((Bot*)selectedObject)->GetActiveBrain());
			else
				nn_renderer.Render(((Bot*)selectedObject)->GetInitialBrain());
		}
	}
}

void Main::DrawWindows()
{
	//ImGUI demo window
	#ifdef ShowGUIDemoWindow
		DrawDemoWindow();
	#endif

	DrawSidePanelWindow();

	//Below windows that are hidden at startup

	DrawSaveLoadWindow();
	DrawDangerousWindow();
	DrawSummaryWindow();
	DrawAdaptationWindow();
	DrawChartWindow();
	DrawBotBrainWindow();
	DrawInfoWindow();
	DrawExitConfirmWindow();
}

void Main::MouseClick()
{
	if (!(nn_renderer.MouseClick({ mouseState.mouseX, mouseState.mouseY }) && (showBrain)))
	{
		if (field->IsInBoundsScreenCoords(mouseState.mouseX, mouseState.mouseY))
		{
			Point fieldCoords = field->ScreenCoordsToLocal(mouseState.mouseX, mouseState.mouseY);

			if (fieldCoords.x < 0)
			{
				fieldCoords.x = 0;
			}
			else if (fieldCoords.x >= FieldCellsWidth)
			{
				fieldCoords.x = FieldCellsWidth - 1;
			}

			Object* obj = field->GetObjectLocalCoords(fieldCoords.x, fieldCoords.y);

			if (mouseFunc == mouse_select)
			{
				if (obj)
				{
					if (obj->type == bot)
					{
						selectedObject = obj;
					}
				}
				else
				{
					Deselect();
				}
			}
			else if (mouseFunc == mouse_remove)
			{
				Deselect();

				for (int cx = -brushSize; cx < brushSize + 1; ++cx)
				{
					for (int cy = -brushSize; cy < brushSize + 1; ++cy)
					{
						if (field->IsInBounds(fieldCoords.x + cx, fieldCoords.y + cy))
							field->RemoveObject(fieldCoords.x + cx, fieldCoords.y + cy);
					}
				}
			}
			else if (mouseFunc == mouse_place_rock)
			{
				for (int cx = -brushSize; cx < brushSize + 1; ++cx)
				{
					for (int cy = -brushSize; cy < brushSize + 1; ++cy)
					{
						if (field->IsInBounds(fieldCoords.x + cx, fieldCoords.y + cy))
						{
							obj = field->GetObjectLocalCoords(fieldCoords.x + cx, fieldCoords.y + cy);

							if (!obj)
							{
								field->AddObject(new Rock(fieldCoords.x + cx, fieldCoords.y + cy));
							}
						}
					}
				}
			}
			else if (mouseFunc == mouse_from_file)
			{
				//Cell is empty				
				if (!obj)
				{
					if (selectedFile)
					{
						obj = saver.LoadObject(selectedFile->pathFull);

						if (obj)
						{
							obj->x = fieldCoords.x;
							obj->y = fieldCoords.y;
							obj->energy = MaxPossibleEnergyForABot;

							if (field->AddObject(obj))
							{
								LogPrint("Объект загружен\r\n");
							}
						}
						else
						{
							LogPrint("Ошибка загрузки объекта\r\n");
						}
					}
				}				
			}
			else if (mouseFunc == mouse_force_mutation)
			{
				for (int cx = -brushSize; cx < brushSize + 1; ++cx)
				{
					for (int cy = -brushSize; cy < brushSize + 1; ++cy)
					{
						if (field->IsInBounds(fieldCoords.x + cx, fieldCoords.y + cy))
						{
							Bot* obj = (Bot*)field->GetObjectLocalCoords(fieldCoords.x + cx, fieldCoords.y + cy);

							if (obj)
							{
								if (obj->type == bot)
								{
									obj->Mutagen();
								}
							}
						}
					}
				}
			}
		}
	}
}

void Main::Render()
{
	
	//Limit FPS
	TimePoint currentTickFps = clock.now();

	if (renderType == noRender)
	{
		fpsInterval = 1000 / GUI_FPSWhenNoRender;
	}
	else
	{
		if (limitFPS > 0)
		{
			fpsInterval = 1000 / limitFPS;
		}
		else
		{
			fpsInterval = 0;
		}
	}

	if (TimeMSBetween(currentTickFps, lastTickFps) < fpsInterval)
	{
		return;
	}
	else
	{
		lastTickFps = currentTickFps;
	}

	//Begin frame

	//Clear background
	UpdateWindowSize();
	glClearColor(BackgroundColorFloat);
	glClear(GL_COLOR_BUFFER_BIT);

	//Render
	if (renderType != noRender)
	{
		field->draw(renderType);

		//Highlight selected object
		HighlightSelection();

		++fpsCounter;
	}

	SelectionShadowScreen();

	//Draw GUI
	GUIStartFrame();
	
	DrawWindows();

	ImGui::EndFrame();

	//Present scene
	SDLPresentScene();

	//Calculate fps
	if (TimeMSBetween(currentTickFps, lastSecondTickFps) >= 1000)
	{
		lastSecondTickFps = currentTickFps;

		realFPS = fpsCounter;
		fpsCounter = 0;
	}
}


void Main::ClearWorld()
{
	Deselect();

	field->RemoveAllObjects();

	ticknum = 0;
}
