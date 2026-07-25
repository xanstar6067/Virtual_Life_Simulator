#include "GUI.h"
#include "../UITheme.h"

namespace cb3
{

using namespace ImGui;


void InitImGUI()
{
	CreateContext();

	io = &GetIO();
	io->Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 16.0f, NULL, io->Fonts->GetGlyphRangesCyrillic());

	ImPlot::CreateContext();

	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	//Setup Dear ImGui style
	vlsui::ApplyModernTheme();

	// Setup Platform/Renderer backends
	ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
	ImGui_ImplSDLRenderer_Init(renderer);
}

void DeInitImGUI()
{
	ImGui_ImplSDLRenderer_Shutdown();
	ImPlot::DestroyContext();
	DestroyContext();
}

void GUIStartFrame()
{
	ImGui_ImplSDLRenderer_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	NewFrame();
}

void Main::DrawDemoWindow()
{
	SetNextWindowBgAlpha(1.0f);
	SetNextWindowPos({ 20.0f,20.0f });

	ShowDemoWindow();
}

void Main::DrawMainWindow()
{
	SetNextWindowBgAlpha(1.0f);
	SetNextWindowSize({ GUIWindowWidth * 1.0f, 175.0f });
	SetNextWindowPos({ (2 * FieldX + FieldWidth) * 1.0f, InterfaceBorder * 1.0f });

	Begin("Главное", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
	{
		Text("Режим симуляции");
		if (RadioButton("Classic", false))
		{
			showModeSwitchConfirm = true;
		}
		SameLine();
		RadioButton("CyberBiology3", true);

		//FPS text 
		Text("шаги: %i", ticknum);
		Text("(интервал %i, тиков/с: %i, кадров/с: %i)", limit_interval, realTPS, realFPS);
		Text("Всего объектов: %i", field->GetNumObjects());
		Text("Всего ботов: %i", field->GetNumBots());

		//Show season name
		if(field->params.useSeasons)
			Text("Сезон: %s ( %i/%i )", SeasonNames[field->GetSeason()], field->GetSeasonCounter(), field->params.seasonInterval);

		//Neural net params and FOV x
		Text("Слоев: %i, нейронов: %i, сдвиг X: %i", NumNeuronLayers, NumHiddenNeurons, field->renderX);

		//Simulation seed and unique id
		Text("Зерно: %i, id симуляции: %i", seed, id);

		//World size and avg lifetime
		Text("Размер мира: %i (%i экранов)", FieldCellsWidth, (FieldCellsWidth) / (ScreenCellsWidth));

		Text("Средний возраст ботов: %i (макс.: %i)", field->GetAverageLifetime(), field->params.botMaxLifetime);

	}
	End();
}


void Main::DrawSystemWindow()
{
	SetNextWindowBgAlpha(1.0f);
	SetNextWindowSize({ GUIWindowWidth * 1.0f, 70.0f });
	SetNextWindowPos({ (2 * FieldX + FieldWidth) * 1.0f, 195.0f });

	Begin("Система", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
	{
		TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "Платформа");
		SameLine();
		Text(" %s", SDL_GetPlatform());

		SameLine();

		TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "Ядер процессора: %d", SDL_GetCPUCount());
		TextColored(ImVec4(1.0f, 0.0f, 1.0f, 1.0f), "Память: %.2f ГБ", SDL_GetSystemRAM() / 1024.0f);
		
		SameLine();

		#if NumThreads == 1
		{
			Text(", один поток");
	}
		#else
		{
			uint threads = NumThreads;
			Text(", потоков: %i", threads);
		}
		#endif
	}
	End();
}

void Main::DrawControlsWindow()
{
	SetNextWindowBgAlpha(1.0f);
	SetNextWindowSize({ GUIWindowWidth * 1.0f, 160.0f });
	SetNextWindowPos({ (2 * FieldX + FieldWidth) * 1.0f, 275.0f });

	Begin("Управление", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
	{
		if (Button((simulate) ? "Стоп" : "Старт", { 200, 25 }))
		{
			SwitchPause();
		}

		PushItemWidth(200);
		SliderInt("лимит тиков", &limit_ticks_per_second, 0, GUI_Max_tps, "%d");
		SliderInt("лимит кадров", &limitFPS, 0, GUI_Max_fps, "%d");

		SliderInt("энергия ФС", &(field->params.PSreward), 0, GUI_Max_food);		
		SliderInt("кисть", &brushSize, GUI_Max_brush, 1, "%d");
	}
	End();
}

void Main::DrawSelectionWindow()
{
	SetNextWindowBgAlpha(1.0f);
	SetNextWindowSize({ GUIWindowWidth * 1.0f, 150.0f });
	SetNextWindowPos({ (2 * FieldX + FieldWidth) * 1.0f, 445.0f });

	Begin("Выбор", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
	{
		if (selectedObject)
		{
			if (field->ValidateObjectExistance(selectedObject))
			{
				//Object info
				Text("тип: бот	X: %i, Y: %i", selectedObject->x, selectedObject->y);
				Text("возраст: %i / %i", selectedObject->GetLifetime(), field->params.botMaxLifetime);
				Text("энергия: %i (ФС: %i, охота: %i)", selectedObject->energy, 
					((Bot*)selectedObject)->GetEnergyFromPS(), ((Bot*)selectedObject)->GetEnergyFromKills());

				//Mutation markers
				int m[NumberOfMutationMarkers];

				memcpy(m, ((Bot*)selectedObject)->GetMarkers(), sizeof(m));

				Text("метки: {");

				repeat(NumberOfMutationMarkers)
				{
					SameLine();
					Text("%i", m[i]);
				}

				SameLine();
				Text("}");

				//Color
				Color& c = *((Bot*)selectedObject)->GetColor();

				Text("цвет: {%i, %i, %i}", c.c[0], c.c[1], c.c[2]);

				SameLine();
				TextColored(ImVec4(((c.c[0] * 1.0f) / 255.0f), ((c.c[1] * 1.0f) / 255.0f), 
					((c.c[2] * 1.0f) / 255.0f), 1.0f), "*****");
				
				if (Button("Показать мозг", { 100, 25 }))
				{
					showBrain = !showBrain;
				}

				SameLine();

				if (Button("Новый цвет", { 70, 25 }))
				{
					field->RepaintBot((Bot*)selectedObject, Color::GetRandomColor(), RepaintTolerance);
				}
			}
			else
			{
				Deselect();
			}
		}
	}
	End();
}

void Main::DrawDisplayWindow()
{
	SetNextWindowBgAlpha(1.0f);
	SetNextWindowSize({ GUIWindowWidth * 1.0f, 100.0f});
	SetNextWindowPos({ (2 * FieldX + FieldWidth) * 1.0f, 605.0f });

	Begin("Вид", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
	{
		BeginGroup();

		Text("Режим:");

		RadioButton("Обычный", (int*)&renderType, 0);
		SameLine();
		RadioButton("Хищники", (int*)&renderType, 1);

		RadioButton("Энергия", (int*)&renderType, 2);
		SameLine();
		RadioButton("Без отрис.", (int*)&renderType, 3);

		EndGroup();
	}
	End();
}

void Main::DrawLogWindow()
{
	SetNextWindowBgAlpha(1.0f);
	SetNextWindowSize({ GUIWindowWidth * 1.0f, 120.0f });
	SetNextWindowPos({ (2 * FieldX + FieldWidth) * 1.0f, 715.0f});

	Begin("Журнал", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
	{
		PushStyleColor(ImGuiCol_ChildBg, ImVec4(LogBackgroundColor));

		BeginChild("scrolling", ImVec2(265, 85), true);
		{
			TextUnformatted(logText.Buf.Data);

			if (GetScrollY() >= GetScrollMaxY())
				SetScrollHereY(1.0f);
		}
		EndChild();

		PopStyleColor();
	}
	End();
}

void Main::DrawMouseFunctionWindow()
{
	SetNextWindowBgAlpha(1.0f);
	SetNextWindowSize({ GUIWindowWidth * 1.0f, 100.0f});
	SetNextWindowPos({ (2 * FieldX + FieldWidth) * 1.0f, 845.0f });

	Begin("Действие мыши", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
	{
		BeginGroup();

		RadioButton("Выбрать", (int*)&mouseFunc, 0);
		SameLine();
		RadioButton("Удалить", (int*)&mouseFunc, 1);

		RadioButton("Камень", (int*)&mouseFunc, 2);
		SameLine();
		RadioButton("Из файла", (int*)&mouseFunc, 3);

		RadioButton("Мутировать", (int*)&mouseFunc, 4);

		EndGroup();
	}
	End();
}

void Main::DrawAdditionalsWindow()
{
	SetNextWindowBgAlpha(1.0f);
	SetNextWindowSize({ GUIWindowWidth * 1.0f, 100.0f });
	SetNextWindowPos({ (2 * FieldX + FieldWidth) * 1.0f, 955.0f });

	Begin("Окна", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
	{

		if (Button("Файлы", { 85, 30 }))
		{
			LoadFilenames();

			showSaveLoad = !showSaveLoad;
		}
		SameLine();

		if (Button("Опасное", { 85, 30 }))
		{
			showDangerous = !showDangerous;
		}
		SameLine();

		if (Button("Среда", { 85, 30 }))
		{
			showAdaptation = !showAdaptation;
		}

		if (Button("График", { 85, 30 }))
		{
			showChart = !showChart;
		}
		SameLine();

		if (Button("Автоадапт.", { 85, 30 }))
		{
			showAutomaticAdaptation = !showAutomaticAdaptation;
		}

		SameLine();

		if (Button("Classic", { 85, 30 }))
		{
			showModeSwitchConfirm = true;
		}

	}
	End();
}

static float GetSidePanelX()
{
	float maxVisibleX = windowWidth - GUISidePanelWidth * 1.0f - InterfaceBorder * 1.0f;

	if (maxVisibleX < InterfaceBorder)
	{
		maxVisibleX = InterfaceBorder * 1.0f;
	}

	return maxVisibleX;
}

void Main::DrawSidePanelWindow()
{
	float panelHeight = windowHeight - InterfaceBorder * 2.0f;

	if (panelHeight < 200.0f)
	{
		panelHeight = 200.0f;
	}

	SetNextWindowBgAlpha(1.0f);
	SetNextWindowSize({ GUISidePanelWidth * 1.0f, panelHeight });
	SetNextWindowPos({ GetSidePanelX(), InterfaceBorder * 1.0f });

	ImGuiWindowFlags panelFlags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse;

	Begin("Панель", NULL, panelFlags);
	{
		TextColored(vlsui::Accent(), "VIRTUAL LIFE");
		SameLine();
		TextDisabled("/ CYBERBIOLOGY3");
		TextDisabled("Расширенная модель эволюции");
		vlsui::DrawStatus(simulate);

		Spacing();
		float halfWidth = vlsui::HalfWidth();
		const ImVec4 runButtonColor = simulate ? vlsui::Warning() : vlsui::Success();

		if (vlsui::ColoredButton(simulate ? "Пауза" : "Продолжить", ImVec2(halfWidth, 38.0f), runButtonColor))
		{
			SwitchPause();
		}
		vlsui::ItemTooltip("Space / Pause");

		SameLine();
		BeginDisabled(simulate);
		if (Button("Один шаг", ImVec2(halfWidth, 38.0f)))
		{
			MakeStep();
		}
		EndDisabled();
		vlsui::ItemTooltip(simulate ? "Сначала поставьте симуляцию на паузу" : "Num +");

		if (vlsui::ColoredButton("Заселить мир", ImVec2(-1.0f, 38.0f), vlsui::Accent()))
		{
			SpawnInitialPopulation();
		}
		vlsui::ItemTooltip("Добавить стартовую группу случайных ботов (F1)");

		Spacing();
		Separator();
		BeginChild("##Cb3PanelContent", ImVec2(0.0f, 0.0f), false);
		{
			if (BeginTabBar("##Cb3PanelTabs"))
			{
				if (BeginTabItem("Симуляция"))
				{
					vlsui::SectionTitle("Состояние мира");
					if (BeginTable("##Cb3Metrics", 2, ImGuiTableFlags_SizingStretchSame))
					{
						TableNextRow();
						TableSetColumnIndex(0);
						TextDisabled("Шаг");
						Text("%u", ticknum);
						TableSetColumnIndex(1);
						TextDisabled("Скорость");
						Text("%u TPS / %u FPS", realTPS, realFPS);

						TableNextRow();
						TableSetColumnIndex(0);
						TextDisabled("Объекты");
						Text("%i", field->GetNumObjects());
						TableSetColumnIndex(1);
						TextDisabled("Боты");
						Text("%i", field->GetNumBots());

						TableNextRow();
						TableSetColumnIndex(0);
						TextDisabled("Средний возраст");
						Text("%i", field->GetAverageLifetime());
						TableSetColumnIndex(1);
						TextDisabled("Размер мира");
						Text("%i клеток", FieldCellsWidth);
						EndTable();
					}

					if (field->params.useSeasons)
					{
						TextDisabled("Сезон: %s  •  %i / %i",
							SeasonNames[field->GetSeason()],
							field->GetSeasonCounter(),
							field->params.seasonInterval);
					}

					vlsui::SectionTitle("Скорость и среда");
					TextDisabled("Лимит симуляции");
					SetNextItemWidth(GetContentRegionAvail().x);
					SliderInt("##Cb3TPS", &limit_ticks_per_second, 0, GUI_Max_tps, "%d тиков/с");
					if (limit_ticks_per_second == 0)
					{
						TextDisabled("Без ограничения скорости");
					}
					TextDisabled("Лимит отрисовки");
					SetNextItemWidth(GetContentRegionAvail().x);
					SliderInt("##Cb3FPS", &limitFPS, 0, GUI_Max_fps, "%d кадров/с");
					TextDisabled("Энергия фотосинтеза");
					SetNextItemWidth(GetContentRegionAvail().x);
					SliderInt("##Cb3PS", &field->params.PSreward, 0, GUI_Max_food, "%d");

					vlsui::SectionTitle("Быстрые действия", "были доступны только с клавиатуры");
					float actionWidth = vlsui::HalfWidth();
					if (Button("Стена", ImVec2(actionWidth, 34.0f)))
						PlaceWorldWall();
					vlsui::ItemTooltip("Вертикальная стена (F2)");
					SameLine();
					if (Button("Органика", ImVec2(actionWidth, 34.0f)))
						DropWorldOrganics();
					vlsui::ItemTooltip("Добавить органику (F3)");
					if (Button("Камни", ImVec2(actionWidth, 34.0f)))
						SpawnWorldRocks();
					vlsui::ItemTooltip("Случайные камни (F4)");
					SameLine();
					if (vlsui::ColoredButton("Мутация мира", ImVec2(actionWidth, 34.0f), vlsui::Warning()))
						MutateWholeWorld();
					vlsui::ItemTooltip("Солнечная вспышка — мутировать ботов (F11)");

					vlsui::SectionTitle("Быстрое сохранение");
					if (Button("Сохранить", ImVec2(actionWidth, 34.0f)))
						QuickSaveWorld();
					vlsui::ItemTooltip("F5");
					SameLine();
					if (Button("Загрузить", ImVec2(actionWidth, 34.0f)))
						QuickLoadWorld();
					vlsui::ItemTooltip("F9 — заменяет текущий мир быстрым сохранением");

					if (CollapsingHeader("Техническая информация"))
					{
						int logicalProcessors = SDL_GetCPUCount();
						Text("Платформа: %s", SDL_GetPlatform());
						Text("Процессоры: %d", logicalProcessors);
						Text("Память: %.2f ГБ", SDL_GetSystemRAM() / 1024.0f);
						Text("Потоки симуляции: %u", (uint)NumThreads);
						if (NumThreads > logicalProcessors)
						{
							TextColored(vlsui::Warning(), "Потоков больше, чем процессоров");
						}
						TextDisabled("Seed %u  •  ID %u", seed, id);
						TextDisabled("Слоев %i  •  нейронов %i", NumNeuronLayers, NumHiddenNeurons);
					}

					EndTabItem();
				}

				if (BeginTabItem("Объект"))
				{
					vlsui::SectionTitle("Выбранный объект");
					if (selectedObject && field->ValidateObjectExistance(selectedObject))
					{
						Text("Бот  •  X %i  •  Y %i", selectedObject->x, selectedObject->y);
						Text("Возраст: %i / %i", selectedObject->GetLifetime(), field->params.botMaxLifetime);
						Text("Энергия: %i", selectedObject->energy);
						TextDisabled("Фотосинтез %i  •  охота %i",
							((Bot*)selectedObject)->GetEnergyFromPS(),
							((Bot*)selectedObject)->GetEnergyFromKills());

						Color& color = *((Bot*)selectedObject)->GetColor();
						ColorButton("##Cb3BotColor",
							ImVec4(color.c[0] / 255.0f, color.c[1] / 255.0f, color.c[2] / 255.0f, 1.0f),
							ImGuiColorEditFlags_NoTooltip, ImVec2(28.0f, 28.0f));
						SameLine();
						if (Button("Новый цвет", ImVec2(110.0f, 28.0f)))
							field->RepaintBot((Bot*)selectedObject, Color::GetRandomColor(), RepaintTolerance);
						SameLine();
						if (Button("Мозг", ImVec2(-1.0f, 28.0f)))
							showBrain = !showBrain;
					}
					else
					{
						if (selectedObject)
							Deselect();
						TextDisabled("Ничего не выбрано.");
						TextDisabled("Выберите инструмент «Выбор» и нажмите на бота.");
					}

					vlsui::SectionTitle("Инструмент мыши");
					float toolWidth = vlsui::HalfWidth();
					if (vlsui::ModeButton("Выбор", mouseFunc == mouse_select, ImVec2(toolWidth, 34.0f)))
						mouseFunc = mouse_select;
					SameLine();
					if (vlsui::ModeButton("Удаление", mouseFunc == mouse_remove, ImVec2(toolWidth, 34.0f)))
						mouseFunc = mouse_remove;
					if (vlsui::ModeButton("Камень", mouseFunc == mouse_place_rock, ImVec2(toolWidth, 34.0f)))
						mouseFunc = mouse_place_rock;
					SameLine();
					if (vlsui::ModeButton("Из файла", mouseFunc == mouse_from_file, ImVec2(toolWidth, 34.0f)))
						mouseFunc = mouse_from_file;
					if (vlsui::ModeButton("Мутация", mouseFunc == mouse_force_mutation, ImVec2(toolWidth, 34.0f)))
						mouseFunc = mouse_force_mutation;

					TextDisabled("Размер кисти");
					SetNextItemWidth(GetContentRegionAvail().x);
					SliderInt("##Cb3Brush", &brushSize, 1, GUI_Max_brush, "%d");

					EndTabItem();
				}

				if (BeginTabItem("Вид и окна"))
				{
					vlsui::SectionTitle("Отображение");
					float viewWidth = vlsui::HalfWidth();
					if (vlsui::ModeButton("Обычный", renderType == natural, ImVec2(viewWidth, 34.0f)))
						renderType = natural;
					SameLine();
					if (vlsui::ModeButton("Хищники", renderType == predators, ImVec2(viewWidth, 34.0f)))
						renderType = predators;
					if (vlsui::ModeButton("Энергия", renderType == energy, ImVec2(viewWidth, 34.0f)))
						renderType = energy;
					SameLine();
					if (vlsui::ModeButton("Без отрисовки", renderType == noRender, ImVec2(viewWidth, 34.0f)))
						renderType = noRender;

					vlsui::SectionTitle("Окна");
					if (BeginTable("##Cb3Windows", 2, ImGuiTableFlags_SizingStretchSame))
					{
						TableNextRow();
						TableSetColumnIndex(0);
						if (Button("Файлы", ImVec2(-1.0f, 34.0f)))
						{
							LoadFilenames();
							showSaveLoad = !showSaveLoad;
						}
						TableSetColumnIndex(1);
						if (Button("Инструменты", ImVec2(-1.0f, 34.0f)))
							showDangerous = !showDangerous;

						TableNextRow();
						TableSetColumnIndex(0);
						if (Button("Среда", ImVec2(-1.0f, 34.0f)))
							showAdaptation = !showAdaptation;
						TableSetColumnIndex(1);
						if (Button("График", ImVec2(-1.0f, 34.0f)))
							showChart = !showChart;

						TableNextRow();
						TableSetColumnIndex(0);
						if (Button("Автоадаптация", ImVec2(-1.0f, 34.0f)))
							showAutomaticAdaptation = !showAutomaticAdaptation;
						TableSetColumnIndex(1);
						if (Button("Выход", ImVec2(-1.0f, 34.0f)))
							terminate = true;
						EndTable();
					}

					vlsui::SectionTitle("Режим симуляции");
					if (vlsui::ColoredButton("Перейти в Classic", ImVec2(-1.0f, 36.0f), vlsui::Accent()))
					{
						showModeSwitchConfirm = true;
					}

					if (CollapsingHeader("Журнал"))
					{
						BeginChild("##Cb3Log", ImVec2(0.0f, 100.0f), true);
						TextUnformatted(logText.Buf.Data);
						if (GetScrollY() >= GetScrollMaxY())
							SetScrollHereY(1.0f);
						EndChild();
					}

					EndTabItem();
				}

				EndTabBar();
			}
		}
		EndChild();
	}
	End();
}

void Main::DrawSaveLoadWindow()
{
	if (!showSaveLoad)
	{
		saveFileNameInputActive = false;
		return;
	}

	if (showSaveLoad)
	{
		//Save/load window
		SetNextWindowBgAlpha(1.0f);
		SetNextWindowSize({ 650.0f, 325.0f });
		SetNextWindowPos({ 100 * 1.0f, 100.0f }, ImGuiCond_Once);

		Begin("Сохранение и загрузка", &showSaveLoad, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
		{
			//List of files
			Text("Выберите файл");

			if (BeginTable("##SaveFilesCB3", 5, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(630, 148)))
			{
				TableSetupColumn("Имя", ImGuiTableColumnFlags_WidthStretch);
				TableSetupColumn("Размер", ImGuiTableColumnFlags_WidthFixed, 70.0f);
				TableSetupColumn("Тип", ImGuiTableColumnFlags_WidthFixed, 55.0f);
				TableSetupColumn("Режим", ImGuiTableColumnFlags_WidthFixed, 115.0f);
				TableSetupColumn("Дата", ImGuiTableColumnFlags_WidthFixed, 125.0f);
				TableHeadersRow();

				for (int i = 0; i < allFilenames.size(); ++i)
				{
					TableNextRow();
					TableSetColumnIndex(0);

					PushID(i);
					if (Selectable(allFilenames[i].nameShort.c_str(), allFilenames[i].isSelected, ImGuiSelectableFlags_SpanAllColumns))
					{
						SelectFile(i);
					}
					PopID();

					TableSetColumnIndex(1);
					TextUnformatted(allFilenames[i].fileSize.c_str());
					TableSetColumnIndex(2);
					TextUnformatted(allFilenames[i].fileType.c_str());
					TableSetColumnIndex(3);
					TextUnformatted(allFilenames[i].modeText.c_str());
					TableSetColumnIndex(4);
					TextUnformatted(allFilenames[i].modifiedTimeText.c_str());
				}

				EndTable();
			}

			Text("Имя файла");
			SameLine();
			PushItemWidth(295);
			InputText("##RenameFileNameCB3", renameFileName, sizeof(renameFileName));
			saveFileNameInputActive = IsItemActive();
			PopItemWidth();

			SameLine();

			if (Button("Переименовать", { 120, 25 }))
			{
				RenameSelectedFile();
			}

			//Buttons
			const float buttonSpacing = GetStyle().ItemSpacing.x;
			const float buttonWidth = (GetContentRegionAvail().x - buttonSpacing * 3.0f) / 4.0f;
			const ImVec2 fileButtonSize = { buttonWidth, 26.0f };

			if (Button("Загрузить", fileButtonSize))
			{
				if (selectedFile)
				{
					if (selectedFile->mode != SimulationMode::CyberBiology3)
					{
						LogPrint("Режим файла: ");
						LogPrint(SimulationModeName(selectedFile->mode));
						LogPrint(", текущий режим: CyberBiology3. Загрузка отменена.\r\n");
					}
					else if (selectedFile->isWorld)
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
							delete selectedObject;

						selectedObject = saver.LoadObject(selectedFile->pathFull);

						if (selectedObject)
							LogPrint("Объект загружен\r\n");
						else
							LogPrint("Ошибка загрузки объекта\r\n");
					}
				}
			}			

			SameLine();

			if (Button("Сохр. бота", fileButtonSize))
			{
				SaveSelectedObjectToNamedFile();
			}

			SameLine();

			if (Button("Сохр. мир", fileButtonSize))
			{
				SaveWorldToNamedFile();
			}

			SameLine();

			if (Button("Удалить", fileButtonSize))
			{
				DeleteSelectedFile();
			}

			if (selectedFile)
			{
				if (selectedFile->isWorld)
				{
					if (selectedFile->mode != SimulationMode::CyberBiology3)
					{
						TextWrapped("Этот мир сохранен в режиме %s и не может быть загружен в CyberBiology3.", SimulationModeName(selectedFile->mode));
					}
					else if (Button("Ландшафт, ботов оставить", { 150, 30 }))
					{
						ObjectSaver::WorldParams ret = saver.LoadWorld(field, selectedFile->pathFull,
							false, false, true, false);

						if (ret.id != -1)
						{
							if (ret.width != FieldCellsWidth)
								LogPrint("Ландшафт загружен (ширина не совпадает)\r\n");
							else
								LogPrint("Ландшафт загружен\r\n");
						}
						else
							LogPrint("Ошибка загрузки ландшафта\r\n");
					}

					if (selectedFile->mode == SimulationMode::CyberBiology3)
					{
						SameLine();

						if (Button("Только боты", { 150, 30 }))
						{
							ObjectSaver::WorldParams ret = saver.LoadWorld(field, selectedFile->pathFull,
								false, false, false, true);

							if (ret.id != -1)
							{
								if (ret.width != FieldCellsWidth)
									LogPrint("Ландшафт загружен (ширина не совпадает)\r\n");
								else
									LogPrint("Ландшафт загружен\r\n");
							}
							else
								LogPrint("Ошибка загрузки ландшафта\r\n");
						}
					}
				}
			}
		}
		End();
	}
}

void Main::DrawDangerousWindow()
{
	if (showDangerous)
	{
		SetNextWindowBgAlpha(1.0f);
		SetNextWindowSize({ 290.0f, 100.0f });
		SetNextWindowPos({ 100 * 1.0f, 300.0f }, ImGuiCond_Once);

		Begin("Инструменты", &showDangerous, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
		{
			if (Button("Очистить мир", { 130, 30 }))
			{
				ClearWorld();
				field->params.Reset();
				chart.ClearChart();
			}

			SameLine();

			if (Button("Убить ботов", { 130, 30 }))
			{
				Deselect();

				for (int cx = 0; cx < FieldCellsWidth; ++cx)
				{
					for (int cy = 0; cy < FieldCellsHeight; ++cy)
					{
						Object* o = field->GetObjectLocalCoords(cx, cy);

						if (o == NULL)
							continue;

						if (o->type() == bot)
							field->RemoveObject(cx, cy);
					}
				}
			}

			if (Button("Выход", { 130, 30 }))
			{
				terminate = true;
			}

			SameLine();

			if (Button("Обнулить время", { 130, 30 }))
			{
				ticknum = 0;
			}
		}
		End();
	}
}

void Main::DrawSummaryWindow()
{
	if (showBrain)
	{
		SetNextWindowBgAlpha(1.0f);
		SetNextWindowSize({ 330.0f, 180.0f });
		SetNextWindowPos({ 100 * 1.0f, 150.0f }, ImGuiCond_Once);

		Begin("Сводка бота", &showBrain, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
		{
			if (selectedObject)
			{
				if (field->ValidateObjectExistance(selectedObject))
				{
					auto summ = ((Bot*)selectedObject)->GetNeuronSummary();

					Text("Нейроны:");
					Text("Простые: %i", summ.simple);
					Text("Радиальные: %i", summ.radialBasis);
					Text("Случайные: %i", summ.random);
					Text("Память: %i", summ.memory);

					NewLine();

					Text("Всего нейронов: %i, тупиковых: %i", summ.neurons, summ.deadend);
					Text("Всего связей: %i", summ.connections);
				}
				else
					goto Nothing;

			}
			else
			{
			Nothing:
				Text("Ничего не выбрано");
			}

		}
		End();
	}
}

void Main::DrawAdaptationWindow()
{
	if (showAdaptation)
	{
		SetNextWindowBgAlpha(1.0f);
		SetNextWindowSize({ 600.0f, 650.0f });
		SetNextWindowPos({ 100 * 1.0f, 250.0f }, ImGuiCond_Once);

		Begin("Условия среды", &showAdaptation, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
		{
			if(CollapsingHeader("Ветры"))			
			{
				SliderInt("Шагов X до деления", &field->params.adaptation_StepsNumToDivide_Winds, 0, 200);
				SliderInt("Сила движения X", &field->params.adaptation_forceBotMovementsX, 0, 1000);
			}

			NewLine();
			
			if (CollapsingHeader("Ныряльщики", ImGuiTreeNodeFlags_DefaultOpen))
			{
				SliderInt("Блок деления на суше", &field->params.adaptation_landBirthBlock, 0, 1000);
				SliderInt("Блок деления в море", &field->params.adaptation_seaBirthBlock, 0, 1000);

				NewLine();

				SliderInt("Блок ФС в океане", &field->params.adaptation_PSInOceanBlock, 0, 1000, "%d");
				SliderInt("Блок ФС в иле", &field->params.adaptation_PSInMudBlock, 0, 1000, "%d");
				SliderInt("ФС на суше хотя бы раз", &field->params.adaptation_botShouldDoPSOnLandOnceToMultiply, 0, 1000, "%d");				
				SliderInt("Сила движения Y", &field->params.adaptation_forceBotMovementsY, 0, 1000);

				NewLine();

				SliderInt("Уровень океана", &field->params.oceanLevel, 0, FieldCellsHeight);
				SliderInt("Уровень ила", &field->params.mudLevel, 0, FieldCellsHeight);
			}

			NewLine();

			if (CollapsingHeader("Органика"))
			{
				SliderInt("Частота появления органики", &field->params.adaptation_organicSpawnRate, 0, 1000);
			}

			NewLine();

			if (CollapsingHeader("Яблоки"))
			{
				SliderInt("Энергия яблока", &field->params.appleEnergy, 1, 200);
				Checkbox("Создавать яблоки", &field->params.spawnApples);
			}

			NewLine();

			if (CollapsingHeader("Прочее"))
			{
				SliderInt("Макс. возраст бота", &field->params.botMaxLifetime, MaxBotLifetime_Min, MaxBotLifetime_Max);
				SliderInt("Макс. энергия бота", &field->params.botMaxEnergy, 100, 2000);
				SliderInt("Задержка размножения", &field->params.fertility_delay, 0, field->params.botMaxLifetime);

				Checkbox("Без мутаций", &field->params.noMutations);
				Checkbox("Без хищников", &field->params.noPredators);
			}

			NewLine();

			if (CollapsingHeader("Сезоны"))
			{	
				Checkbox("Использовать сезоны", &field->params.useSeasons);
				SliderInt("Интервал смены сезона", &field->params.seasonInterval, 200, 4000);
			}

			NewLine();

			if (Button("Сброс", { 70, 20 }))
			{
				field->params.Reset();
			}
		}
		End();
	}
}

void Main::DrawChartWindow()
{
	if (showChart)
	{
		SetNextWindowBgAlpha(1.0f);
		SetNextWindowSize({ 920.0f, 600.0f });
		SetNextWindowPos({ 700.0f, 250.0f }, ImGuiCond_Once);

		Begin("График популяции", &showChart, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
		{
			chart.Plot();
		}
		End();
	}
}

void Main::DrawBotBrainWindow()
{	
	if (showBrain)
	{
		if (selectedObject)
		{
			SetNextWindowBgAlpha(1.0f);
			SetNextWindowSize({ 330.0f, 280.0f });
			SetNextWindowPos({ 650 * 1.0f, 350.0f });

			Begin("Данные мозга бота", &showBrain, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
			{
				BeginGroup();

				RadioButton("Текущий мозг", &brainToShow, 0);
				RadioButton("Начальный мозг", &brainToShow, 1);

				EndGroup();

				if (nn_renderer.selectedNeuron)
				{
					//Show neuron type
					Text("Тип нейрона:");
					SameLine();
					Text(NeuronTypeNames[nn_renderer.selectedNeuron->type]);

					if (nn_renderer.selectedNeuronCaption != "")
					{
						SameLine();
						Text(" (%s)", nn_renderer.selectedNeuronCaption);
					}

					//Show value 
					Text("Значение: %f", nn_renderer.selectedNeuronValue);

					//Show memory
					if(nn_renderer.selectedNeuron->type == memory)
					{
						Text("Память: %i", nn_renderer.selectedNeuronMemory);
					}

					//Show bias
					Text("Смещение: %f", (nn_renderer.selectedNeuron->bias * BiasMultiplier));

					//Show connections
					repeat(nn_renderer.selectedNeuron->numConnections)
					{
						Text("Связь к слою: %i, нейрону: %i, вес: %f", nn_renderer.selectedNeuron->allConnections[i].dest_layer,
							nn_renderer.selectedNeuron->allConnections[i].dest_neuron, (nn_renderer.selectedNeuron->allConnections[i].weight * WeightMultiplier));
					}

					//Show memory data somehow(TODO)
					//Text("Memory data: %i", nn_renderer.selectedBrain->allMemory[][]);

					//TODO
					if((nn_renderer.selectedNeuron->type!=input) and (nn_renderer.selectedNeuron->type != output))
					{
						if (Button("случайно", { 100,30 }))
						{
							nn_renderer.selectedNeuron->SetRandom();
						}

						SameLine();

						if (Button("обнулить", { 100,30 }))
						{
							nn_renderer.selectedNeuron->SetZero();
						}
					}
				}
				else
				{
					Text("Ничего не выбрано");
				}

				NewLine();

				if (Button("Мутировать мозг", { 100,30 }))
				{
					if(nn_renderer.selectedBrain)
						nn_renderer.selectedBrain->Mutate();
				}
			}

			End();

			if(brainToShow == 0)
				nn_renderer.Render(((Bot*)selectedObject)->GetActiveBrain());
			else
				nn_renderer.Render(((Bot*)selectedObject)->GetInitialBrain());
		}
	}
}

void Main::DrawAAWindow()
{
	if (showAutomaticAdaptation)
	{
		auto_adapt->DrawWindow(&showAutomaticAdaptation);
	}
}

void Main::DrawFieldScrollbars()
{
	field->ClampViewOffset();

	SDL_Rect viewport = field->GetViewportRect();
	int maxViewX = field->GetMaxViewX();
	int maxViewY = field->GetMaxViewY();

	if ((maxViewX <= 0) and (maxViewY <= 0))
	{
		return;
	}

	ImGuiWindowFlags flags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoScrollWithMouse;

	PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
	PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

	if (maxViewX > 0)
	{
		SetNextWindowBgAlpha(1.0f);
		SetNextWindowPos({ viewport.x * 1.0f, (viewport.y + viewport.h) * 1.0f });
		SetNextWindowSize({ viewport.w * 1.0f, FieldScrollbarSize * 1.0f });

		Begin("##FieldScrollXWindowCB3", NULL, flags);
		{
			PushItemWidth(viewport.w * 1.0f);
			SliderInt("##FieldScrollXCB3", &Field::viewX, 0, maxViewX, "");
			PopItemWidth();
		}
		End();
	}

	if (maxViewY > 0)
	{
		SetNextWindowBgAlpha(1.0f);
		SetNextWindowPos({ (viewport.x + viewport.w) * 1.0f, viewport.y * 1.0f });
		SetNextWindowSize({ FieldScrollbarSize * 1.0f, viewport.h * 1.0f });

		Begin("##FieldScrollYWindowCB3", NULL, flags);
		{
			VSliderInt("##FieldScrollYCB3", { FieldScrollbarSize * 1.0f, viewport.h * 1.0f }, &Field::viewY, 0, maxViewY, "");
		}
		End();
	}

	PopStyleVar(3);

	field->ClampViewOffset();
}

void Main::DrawModeSwitchWindow()
{
}

void Main::DrawModeSwitchConfirmWindow()
{
	if (showModeSwitchConfirm)
	{
		OpenPopup("Подтверждение смены режима##CB3");
		showModeSwitchConfirm = false;
	}

	if (BeginPopupModal("Подтверждение смены режима##CB3", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		TextWrapped("Переключение режима остановит текущую симуляцию и создаст новый мир Classic. Текущий мир не конвертируется.");

		if (Button("Переключить", { 120, 30 }))
		{
			requestedMode = SimulationMode::Classic;
			modeSwitchRequested = true;
			CloseCurrentPopup();
		}

		SameLine();

		if (Button("Отмена", { 90, 30 }))
		{
			CloseCurrentPopup();
		}

		EndPopup();
	}
}

void Main::DrawWindows()
{
	//ImGUI demo window
	#ifdef ShowGUIDemoWindow
		DrawDemoWindow();
	#endif

	DrawSidePanelWindow();
	DrawFieldScrollbars();
	DrawModeSwitchWindow();

	//Below windows that are hidden at startup

	DrawSaveLoadWindow();
	DrawDangerousWindow();
	DrawSummaryWindow();
	DrawAdaptationWindow();
	DrawChartWindow();
	DrawBotBrainWindow();
	DrawAAWindow();
	DrawModeSwitchConfirmWindow();
}

void Main::MouseClick()
{
	if (!(nn_renderer.MouseClick({ mouseState.mouseX, mouseState.mouseY }) and (showBrain)))
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
					if (obj->type() == bot)
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
				BrushIterate(fieldCoords, [](uint X, uint Y, Field* field) 
					{ 
						field->RemoveObject(X, Y); 
					}
				);
			}
			else if (mouseFunc == mouse_place_rock)
			{
				BrushIterate(fieldCoords, [](uint X, uint Y, Field* field) 
					{ 
						field->ObjectAddOrReplace(new Rock(X, Y)); 
					}
				);
			}
			else if (mouseFunc == mouse_from_file)
			{
				//Cell is empty				
				if (!obj)
				{
					if (selectedFile)
					{
						if (selectedFile->mode != SimulationMode::CyberBiology3)
						{
							LogPrint("Режим файла: ");
							LogPrint(SimulationModeName(selectedFile->mode));
							LogPrint(", текущий режим: CyberBiology3. Загрузка отменена.\r\n");
							return;
						}

						obj = saver.LoadObject(selectedFile->pathFull);

						if (obj)
						{
							obj->x = fieldCoords.x;
							obj->y = fieldCoords.y;
							obj->energy = field->params.botMaxEnergy;

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
				BrushIterate(fieldCoords, [](uint X, uint Y, Field* field)
					{
						Bot* obj = (Bot*)field->GetObjectLocalCoords(X, Y);

						if (obj)
						{
							if (obj->type() == bot)
							{
								obj->Mutagen();
							}
						}
					}
				);
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
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

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

	EndFrame();

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

void Main::RunWithNoRender()
{
	limit_ticks_per_second = 0;
	renderType = noRender;

	Start();
}

void Main::RunWithMinimumRender()
{
	limit_ticks_per_second = 0;
	limitFPS = 25;
	renderType = natural;

	Start();
}

void Main::Print(string s)
{
	LogPrint(s.c_str());
}


void Main::ClearWorld()
{
	Deselect();

	field->RemoveAllObjects();

	ticknum = 0;
}


}
