
#include "GUI.h"
#include "UITheme.h"



void InitImGUI()
{
	ImGui::CreateContext();

	io = &ImGui::GetIO();
	vlsui::AddInterfaceFont(*io);

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

void Main::DrawSidePanelWindow()
{
	const auto layout = vlsui::CalculateLayout(windowWidth, windowHeight);
	int displayMode = (int)renderType;
	const auto actions =
		vlsui::DrawWorkspaceHeader(layout, "CLASSIC", simulate, limit_ticks_per_second, GUI_Max_tps, displayMode);
	renderType = (RenderTypes)displayMode;
	if (actions.seed)
		SpawnInitialPopulation();
	if (actions.pause)
		Pause();
	if (actions.step)
		MakeStep();
	if (actions.saves)
	{
		LoadFilenames();
		showSaveLoad = true;
	}
	mouseFunc = (MouseFunction)vlsui::DrawWorkspaceTools(layout, (int)mouseFunc, brushSize, 0);

	// Tick counters are stale after editing a paused world. Refresh only the
	// displayed counts; this does not step the world or alter simulation state.
	static unsigned displayedBots = 0, displayedObjects = 0;
	static double nextCountRefresh = 0.0;
	static const Field *countedField = nullptr;
	if (simulate)
	{
		displayedBots = field->GetNumBots();
		displayedObjects = field->GetNumObjects();
	}
	else if (field != countedField || ImGui::GetTime() >= nextCountRefresh || actions.seed || actions.step)
	{
		displayedBots = displayedObjects = 0;
		for (int x = 0; x < FieldCellsWidth; ++x)
			for (int y = 0; y < FieldCellsHeight; ++y)
				if (const auto object = field->GetObjectLocalCoords(x, y))
				{
					++displayedObjects;
					if (object->type == bot)
						++displayedBots;
				}
		nextCountRefresh = ImGui::GetTime() + 0.25;
		countedField = field;
	}

	vlsui::BeginWorkspaceWindow("##WorkspaceInspector", layout.inspector);
	if (ImGui::BeginTabBar("##InspectorTabs"))
	{
		if (ImGui::BeginTabItem("Организм", nullptr,
								revealSelection ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None))
		{
			ImGui::BeginChild("##OrganismInspectorContent", ImVec2(0.0f, 0.0f), false);
			vlsui::SectionTitle("Выбранный объект");
			if (selectedObject && field->ValidateObjectExistance(selectedObject))
			{
				ImGui::Text("Бот  •  X %i  •  Y %i", selectedObject->x, selectedObject->y);
				ImGui::Text("Возраст: %i / %i", selectedObject->GetLifetime(), MaxBotLifetime);
				ImGui::Text("Энергия: %i / %i", selectedObject->energy, MaxPossibleEnergyForABot);
				ImGui::ProgressBar((float)selectedObject->energy / MaxPossibleEnergyForABot, ImVec2(-1.0f, 6.0f), "");
				ImGui::TextDisabled("Фотосинтез %i  •  охота %i", ((Bot *)selectedObject)->GetEnergyFromPS(),
									((Bot *)selectedObject)->GetEnergyFromKills());

				ImGui::TextDisabled("Метки родства");
				for (int i = 0; i < NumberOfMutationMarkers; ++i)
				{
					if (i)
						ImGui::SameLine();
					ImGui::Text("%i", ((Bot *)selectedObject)->GetMarkers()[i]);
				}
				Uint8 color[3];
				memcpy(color, ((Bot *)selectedObject)->GetColor(), sizeof(color));
				ImGui::ColorButton("##ClassicBotColor",
								   ImVec4(color[0] / 255.0f, color[1] / 255.0f, color[2] / 255.0f, 1.0f),
								   ImGuiColorEditFlags_NoTooltip, ImVec2(28.0f, 28.0f));
				ImGui::SameLine();
				if (ImGui::Button("Новый цвет", ImVec2(110.0f, 28.0f)))
				{
					field->RepaintBot((Bot *)selectedObject, Bot::GetRandomColor(), 1);
				}
				ImGui::SameLine();
				if (ImGui::Button("Мозг", ImVec2(-1.0f, 28.0f)))
				{
					showBrain = !showBrain;
				}
			}
			else
			{
				if (selectedObject)
				{
					Deselect();
				}
				ImGui::TextDisabled("Ничего не выбрано.");
				ImGui::TextWrapped("Выберите инструмент «Выбор» слева и нажмите на бота в мире.");
			}

			ImGui::EndChild();
			ImGui::EndTabItem();
		}
		revealSelection = false;
		if (ImGui::BeginTabItem("Среда"))
		{
			ImGui::BeginChild("##EnvironmentInspectorContent", ImVec2(0.0f, 0.0f), false);

			vlsui::SectionTitle("Условия мира");
			ImGui::TextDisabled("Уровень океана");
			ImGui::SetNextItemWidth(-1.0f);
			ImGui::SliderInt("##InspectorOcean", &field->params.oceanLevel, 0, FieldCellsHeight, "%d клеток");
			ImGui::TextDisabled("Уровень ила / грязи");
			ImGui::SetNextItemWidth(-1.0f);
			ImGui::SliderInt("##InspectorMud", &field->params.mudLevel, 0, FieldCellsHeight, "%d клеток");
			ImGui::TextDisabled("Энергия фотосинтеза");
			ImGui::SetNextItemWidth(-1.0f);
			ImGui::SliderInt("##InspectorPS", &field->photosynthesisReward, 0, GUI_Max_food, "%d");
			ImGui::Checkbox("Появление яблок", &field->params.spawnApples);
			if (ImGui::Button("Все условия среды", ImVec2(-1.0f, 34.0f)))
				showAdaptation = true;
			vlsui::ItemTooltip("C · ограничения размножения, отбор, органика и остальные условия");
			vlsui::SectionTitle("Отрисовка");
			ImGui::TextDisabled("Лимит кадров в секунду");
			ImGui::SetNextItemWidth(-1.0f);
			ImGui::SliderInt("##InspectorFPS", &limitFPS, 0, GUI_Max_fps, "%d кадров/с");
			ImGui::TextWrapped("0 — без ограничения. Отключить отрисовку можно в списке режимов над миром.");

			ImGui::EndChild();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Мир"))
		{
			ImGui::BeginChild("##WorldInspectorContent", ImVec2(0.0f, 0.0f), false);
			vlsui::SectionTitle("Состояние мира");
			if (ImGui::BeginTable("##ClassicMetrics", 2, ImGuiTableFlags_SizingStretchSame))
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextDisabled("Шаг");
				ImGui::Text("%u", ticknum);
				ImGui::TableSetColumnIndex(1);
				ImGui::TextDisabled("Скорость");
				ImGui::Text("%u TPS / %u FPS", realTPS, realFPS);

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextDisabled("Объекты");
				ImGui::Text("%i", displayedObjects);
				ImGui::TableSetColumnIndex(1);
				ImGui::TextDisabled("Боты");
				ImGui::Text("%i", displayedBots);
				ImGui::EndTable();
			}

			vlsui::SectionTitle("Быстрые действия");
			float actionWidth = vlsui::HalfWidth();
			if (ImGui::Button("Стена", ImVec2(actionWidth, 34.0f)))
			{
				PlaceWorldWall();
			}
			vlsui::ItemTooltip("Вертикальная стена в X=0 (F2)");
			ImGui::SameLine();
			if (ImGui::Button("Органика", ImVec2(actionWidth, 34.0f)))
			{
				DropWorldOrganics();
			}
			vlsui::ItemTooltip("Насыпать органику в верхней части мира (F3)");

			vlsui::SectionTitle("Быстрое сохранение", "отдельный слот Classic");
			if (ImGui::Button("Сохранить F5", ImVec2(actionWidth, 34.0f)))
			{
				QuickSaveWorld();
			}
			vlsui::ItemTooltip("F5 — перезаписывает быстрый слот Classic");
			ImGui::SameLine();
			const bool hasClassicQuickSave = std::filesystem::exists(QuicksaveFilename);
			ImGui::BeginDisabled(!hasClassicQuickSave);
			if (ImGui::Button("Загрузить F9", ImVec2(actionWidth, 34.0f)))
			{
				QuickLoadWorld();
			}
			ImGui::EndDisabled();
			vlsui::ItemTooltip(hasClassicQuickSave ? "F9 — заменяет текущий мир быстрым сохранением"
												   : "Быстрое сохранение Classic еще не создано");

			if (ImGui::CollapsingHeader("Техническая информация"))
			{
				ImGui::Text("Платформа: %s", SDL_GetPlatform());
				ImGui::Text("Процессоры: %d", SDL_GetCPUCount());
				ImGui::Text("Память: %.2f ГБ", SDL_GetSystemRAM() / 1024.0f);
				ImGui::Text("Потоки симуляции: %u", field->GetNumThreads());
				ImGui::TextDisabled("Seed %i  •  ID %i", seed, id);
				ImGui::TextDisabled("Слоев %i  •  нейронов %i", NumNeuronLayers, NeuronsInLayer);
			}

			ImGui::EndChild();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Ещё"))
		{
			ImGui::BeginChild("##WindowsInspectorContent", ImVec2(0.0f, 0.0f), false);
			vlsui::SectionTitle("Окна");
			if (ImGui::BeginTable("##ClassicWindows", 2, ImGuiTableFlags_SizingStretchSame))
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				if (ImGui::Button("Сохранения", ImVec2(-1.0f, 34.0f)))
				{
					LoadFilenames();
					showSaveLoad = !showSaveLoad;
				}
				ImGui::TableSetColumnIndex(1);
				if (ImGui::Button("Инструменты", ImVec2(-1.0f, 34.0f)))
					showDangerous = !showDangerous;

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				if (ImGui::Button("Среда", ImVec2(-1.0f, 34.0f)))
					showAdaptation = !showAdaptation;
				ImGui::TableSetColumnIndex(1);
				if (ImGui::Button("График", ImVec2(-1.0f, 34.0f)))
					showChart = !showChart;

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				if (ImGui::Button("Справка", ImVec2(-1.0f, 34.0f)))
					showInfo = !showInfo;
				ImGui::TableSetColumnIndex(1);
				if (ImGui::Button("Выход", ImVec2(-1.0f, 34.0f)))
					showExitConfirm = true;
				ImGui::EndTable();
			}

			vlsui::SectionTitle("Режим симуляции");
			if (vlsui::ColoredButton("Перейти в CyberBiology3", ImVec2(-1.0f, 36.0f), vlsui::Accent()))
			{
				RequestSimulationMode(SimulationMode::CyberBiology3);
			}

			if (ImGui::CollapsingHeader("Журнал"))
			{
				ImGui::BeginChild("##ClassicLog", ImVec2(0.0f, 100.0f), true);
				ImGui::TextUnformatted(logText.Buf.Data);
				if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
					ImGui::SetScrollHereY(1.0f);
				ImGui::EndChild();
			}

			ImGui::EndChild();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	ImGui::End();

	vlsui::BeginWorkspaceWindow("##WorkspaceFooter", layout.footer);
	if (ImGui::BeginTable("##WorkspaceStats", 2, ImGuiTableFlags_SizingStretchProp))
	{
		ImGui::TableSetupColumn("##LiveMetrics", ImGuiTableColumnFlags_WidthFixed, layout.compact ? 275.0f : 340.0f);
		ImGui::TableSetupColumn("##Population", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		ImGui::Text("Боты: %u", displayedBots);
		ImGui::SameLine(140.0f);
		ImGui::Text("Шаг: %u", ticknum);
		ImGui::TextDisabled("Объекты: %u", displayedObjects);
		ImGui::SameLine(140.0f);
		ImGui::TextDisabled("%u TPS / %u FPS", realTPS, realFPS);
		vlsui::DrawStatus(simulate);
		ImGui::TableSetColumnIndex(1);
		ImGui::TextDisabled("Популяция");
		ImGui::SameLine();
		if (ImGui::SmallButton("Развернуть график"))
			showChart = true;
		vlsui::DrawPopulationPreview(chartData_bots, chart_numValues, ImVec2(-1.0f, 42.0f));
		ImGui::EndTable();
	}
	ImGui::End();

	if (showFirstPopulationHint && ticknum == 0 && mouseFunc == mouse_select && renderType != noRender)
	{
		if (vlsui::DrawEmptyWorld(layout.world))
			SpawnInitialPopulation();
	}
}

void Main::DrawFieldScrollbars()
{
	field->ClampViewOffset();

	SDL_Rect viewport = field->GetViewportRect();
	int maxViewX = field->GetMaxViewX();
	int maxViewY = field->GetMaxViewY();

	if ((maxViewX <= 0) && (maxViewY <= 0))
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

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

	if (maxViewX > 0)
	{
		ImGui::SetNextWindowBgAlpha(1.0f);
		ImGui::SetNextWindowPos({ viewport.x * 1.0f, (viewport.y + viewport.h) * 1.0f });
		ImGui::SetNextWindowSize({ viewport.w * 1.0f, FieldScrollbarSize * 1.0f });

		ImGui::Begin("##FieldScrollXWindow", NULL, flags);
		{
			ImGui::PushItemWidth(viewport.w * 1.0f);
			ImGui::SliderInt("##FieldScrollX", &Field::viewX, 0, maxViewX, "");
			ImGui::PopItemWidth();
		}
		ImGui::End();
	}

	if (maxViewY > 0)
	{
		ImGui::SetNextWindowBgAlpha(1.0f);
		ImGui::SetNextWindowPos({ (viewport.x + viewport.w) * 1.0f, viewport.y * 1.0f });
		ImGui::SetNextWindowSize({ FieldScrollbarSize * 1.0f, viewport.h * 1.0f });

		ImGui::Begin("##FieldScrollYWindow", NULL, flags);
		{
			ImGui::VSliderInt("##FieldScrollY", { FieldScrollbarSize * 1.0f, viewport.h * 1.0f }, &Field::viewY, 0, maxViewY, "");
		}
		ImGui::End();
	}

	ImGui::PopStyleVar(3);

	field->ClampViewOffset();
}

void Main::DrawSaveLoadWindow()
{
	if (!showSaveLoad)
	{
		saveFileNameInputActive = false;
		return;
	}

	ImGui::SetNextWindowBgAlpha(1.0f);
	vlsui::PrepareToolWindow(ImVec2(860.0f, 560.0f), ImVec2(680.0f, 480.0f));

	ImGui::Begin("Сохранения и загрузка", &showSaveLoad, ImGuiWindowFlags_NoCollapse);
	{
		ImGui::TextColored(vlsui::Accent(), "CLASSIC");
		ImGui::SameLine();
		ImGui::TextDisabled("/ SavedObjects");
		ImGui::TextDisabled("Сохранения этого режима отделены от CyberBiology3 и используют собственный быстрый слот.");

		vlsui::SectionTitle("Новое сохранение", "пустое имя создаст имя по дате и времени");
		bool newNameInputActive = false;
		const bool canSaveBot = selectedObject && field->ValidateObjectExistance(selectedObject);

		if (ImGui::BeginTable("##ClassicCreateSave", 3, ImGuiTableFlags_SizingStretchProp))
		{
			ImGui::TableSetupColumn("##ClassicSaveName", ImGuiTableColumnFlags_WidthStretch, 1.7f);
			ImGui::TableSetupColumn("##ClassicSaveWorld", ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableSetupColumn("##ClassicSaveBot", ImGuiTableColumnFlags_WidthStretch, 1.0f);
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::SetNextItemWidth(-1.0f);
			ImGui::InputTextWithHint("##ClassicNewSaveName", "Имя нового сохранения", newSaveFileName, sizeof(newSaveFileName));
			newNameInputActive = ImGui::IsItemActive();

			ImGui::TableSetColumnIndex(1);
			if (vlsui::ColoredButton("Сохранить мир", ImVec2(-1.0f, 34.0f), vlsui::Accent()))
			{
				SaveWorldToNamedFile();
			}

			ImGui::TableSetColumnIndex(2);
			ImGui::BeginDisabled(!canSaveBot);
			if (ImGui::Button("Сохранить бота", ImVec2(-1.0f, 34.0f)))
			{
				SaveSelectedObjectToNamedFile();
			}
			ImGui::EndDisabled();
			vlsui::ItemTooltip(canSaveBot ? "Сохранить выбранного бота" : "Сначала выберите бота в мире");
			ImGui::EndTable();
		}

		vlsui::SectionTitle("Библиотека сохранений");
		ImGui::TextDisabled("Файлов: %d", (int)allFilenames.size());
		ImGui::SameLine();
		const float refreshWidth = 105.0f;
		ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - refreshWidth);
		if (ImGui::Button("Обновить", ImVec2(refreshWidth, 0.0f)))
		{
			LoadFilenames();
		}

		float fileTableHeight = ImGui::GetContentRegionAvail().y - 145.0f;
		if (fileTableHeight < 160.0f)
			fileTableHeight = 160.0f;

		if (ImGui::BeginTable("##SaveFiles", 5,
			ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
			ImGuiTableFlags_Resizable, ImVec2(0.0f, fileTableHeight)))
		{
			ImGui::TableSetupColumn("Имя", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Размер", ImGuiTableColumnFlags_WidthFixed, 70.0f);
			ImGui::TableSetupColumn("Тип", ImGuiTableColumnFlags_WidthFixed, 95.0f);
			ImGui::TableSetupColumn("Режим", ImGuiTableColumnFlags_WidthFixed, 130.0f);
			ImGui::TableSetupColumn("Дата", ImGuiTableColumnFlags_WidthFixed, 125.0f);
			ImGui::TableHeadersRow();

			if (allFilenames.empty())
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextDisabled("Сохранений Classic пока нет");
			}

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
				ImGui::TextUnformatted(allFilenames[i].modeText.c_str());
				ImGui::TableSetColumnIndex(4);
				ImGui::TextUnformatted(allFilenames[i].modifiedTimeText.c_str());
			}

			ImGui::EndTable();
		}

		bool renameInputActive = false;
		if (selectedFile)
		{
			if (ImGui::BeginTable("##ClassicRenameRow", 3, ImGuiTableFlags_SizingStretchProp))
			{
				ImGui::TableSetupColumn("##ClassicRenameLabel", ImGuiTableColumnFlags_WidthFixed, 85.0f);
				ImGui::TableSetupColumn("##ClassicRenameInput", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("##ClassicRenameButton", ImGuiTableColumnFlags_WidthFixed, 145.0f);
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::AlignTextToFramePadding();
				ImGui::TextUnformatted("Имя файла");
				ImGui::TableSetColumnIndex(1);
				ImGui::SetNextItemWidth(-1.0f);
				ImGui::InputText("##RenameFileName", renameFileName, sizeof(renameFileName));
				renameInputActive = ImGui::IsItemActive();
				ImGui::TableSetColumnIndex(2);
				if (ImGui::Button("Переименовать", ImVec2(-1.0f, 0.0f)))
				{
					RenameSelectedFile();
				}
				ImGui::EndTable();
			}
		}
		else
		{
			ImGui::TextDisabled("Выберите сохранение, чтобы загрузить, переименовать или удалить его.");
		}

		saveFileNameInputActive = newNameInputActive || renameInputActive;
		const bool canLoadSelected = selectedFile && selectedFile->mode == activeMode;

		if (selectedFile && !canLoadSelected)
		{
			ImGui::TextColored(vlsui::Warning(), "Файл создан в другом или устаревшем режиме и не может быть загружен в Classic.");
		}

		if (ImGui::BeginTable("##ClassicFileActions", 2, ImGuiTableFlags_SizingStretchSame))
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::BeginDisabled(!canLoadSelected);
			const char* loadLabel = (selectedFile && !selectedFile->isWorld) ? "Загрузить бота" : "Загрузить мир";
			if (vlsui::ColoredButton(loadLabel, ImVec2(-1.0f, 34.0f), vlsui::Accent()))
			{
				if (selectedFile->isWorld)
				{
					LoadWorldFromFile(selectedFile->pathFull);
				}
				else
				{
					Object* loadedObject = saver.LoadObject(selectedFile->pathFull);

					if (loadedObject)
					{
						int targetX = loadedObject->x;
						int targetY = loadedObject->y;

						if (selectedObject && field->ValidateObjectExistance(selectedObject))
						{
							targetX = selectedObject->x;
							targetY = selectedObject->y;
							Deselect();
							field->RemoveObject(targetX, targetY);
						}
						else if (selectedObject)
						{
							Deselect();
						}

						loadedObject->x = targetX;
						loadedObject->y = targetY;

						if (field->IsInBounds(targetX, targetY) && field->AddObject(loadedObject))
						{
							selectedObject = loadedObject;
							revealSelection = true;
							field->TrackObject(selectedObject);
							showFirstPopulationHint = false;
							LogPrint("Объект загружен\r\n");
						}
						else
						{
							delete loadedObject;
							LogPrint("Ошибка загрузки объекта: клетка занята или вне поля\r\n");
						}
					}
					else
						LogPrint("Ошибка загрузки объекта\r\n");
				}
			}
			ImGui::EndDisabled();

			ImGui::TableSetColumnIndex(1);
			ImGui::BeginDisabled(!selectedFile);
			if (ImGui::Button("Удалить в корзину", ImVec2(-1.0f, 34.0f)))
			{
				DeleteSelectedFile();
			}
			ImGui::EndDisabled();
			ImGui::EndTable();
		}
	}
	ImGui::End();
}

void Main::DrawDangerousWindow()
{
	if (showDangerous)
	{
		ImGui::SetNextWindowBgAlpha(1.0f);
		vlsui::PrepareToolWindow(ImVec2(380.0f, 190.0f), ImVec2(340.0f, 170.0f));

		ImGui::Begin("Инструменты", &showDangerous, ImGuiWindowFlags_NoCollapse);
		{
			ImGui::TextDisabled("Операции применяются ко всему миру");
			if (ImGui::BeginTable("##ClassicDangerActions", 2, ImGuiTableFlags_SizingStretchSame))
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				if (ImGui::Button("Очистить мир", ImVec2(-1.0f, 36.0f)))
				{
					ClearWorld();
				}
				ImGui::TableSetColumnIndex(1);
				if (ImGui::Button("Убить всех ботов", ImVec2(-1.0f, 36.0f)))
				{
					Deselect();
					for (int cx = 0; cx < FieldCellsWidth; ++cx)
					{
						for (int cy = 0; cy < FieldCellsHeight; ++cy)
						{
							Object* o = field->GetObjectLocalCoords(cx, cy);
							if (o && o->type == bot)
								field->RemoveObject(cx, cy);
						}
					}
				}

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				if (ImGui::Button("Обнулить время", ImVec2(-1.0f, 36.0f)))
				{
					ticknum = 0;
				}
				ImGui::EndTable();
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
		vlsui::PrepareToolWindow(ImVec2(390.0f, 270.0f), ImVec2(340.0f, 220.0f));

		ImGui::Begin("Сводка бота", &showBrain, ImGuiWindowFlags_NoCollapse);
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
		vlsui::PrepareToolWindow(ImVec2(580.0f, 620.0f), ImVec2(460.0f, 460.0f));

		ImGui::Begin("Условия среды", &showAdaptation, ImGuiWindowFlags_NoCollapse);
		{
			ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x * 0.56f);
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
			ImGui::PopItemWidth();
		}
		ImGui::End();
	}
}

void Main::DrawChartWindow()
{
	if (showChart)
	{
		ImGui::SetNextWindowBgAlpha(1.0f);
		vlsui::PrepareToolWindow(ImVec2(940.0f, 680.0f), ImVec2(620.0f, 440.0f));

		ImGui::Begin("График популяции", &showChart, ImGuiWindowFlags_NoCollapse);
		{
			if (ImGui::Button("Очистить", ImVec2(100.0f, 32.0f)))
				ClearChart();
			ImGui::SameLine();
			ImGui::Checkbox("Яблоки", &chartShow_apples);
			ImGui::SameLine();
			ImGui::Checkbox("Органика", &chartShow_organics);

			if (ImPlot::BeginPlot("Объекты", ImVec2(-1.0f, -1.0f)))
			{
				const int chartValuesToPlot = chart_numValues;

				//Axes
				ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, 250.0, ImPlotCond_Always);
				ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, 26000.0);

				ImPlot::SetupAxis(ImAxis_X1, "Время");
				ImPlot::SetupAxis(ImAxis_Y1, "Количество");

				//Bots number
				ImPlot::SetNextLineStyle({ 1, 0, 0, 1 }, ChartLineThickness);

				ImPlot::PlotLine("Боты", chartData_bots, chartValuesToPlot, 1.0f, 0.0f, ImPlotLineFlags_None);

				//Apples number
				if(chartShow_apples)
				{
					ImPlot::SetNextLineStyle({ 0, 1, 0, 1 }, ChartLineThickness);

					ImPlot::PlotLine("Яблоки", chartData_apples, chartValuesToPlot, 1.0f, 0.0f, ImPlotLineFlags_None);
				}

				//Organics number
				if(chartShow_organics)
				{
					ImPlot::SetNextLineStyle({ 0, 0, 1, 1 }, ChartLineThickness);

					ImPlot::PlotLine("Органика", chartData_organics, chartValuesToPlot, 1.0f, 0.0f, ImPlotLineFlags_None);
				}

				ImPlot::EndPlot();
			}
		}
		ImGui::End();
	}
}

void Main::DrawInfoWindow()
{
	if (showInfo)
	{
		ImGui::SetNextWindowBgAlpha(1.0f);
		vlsui::PrepareToolWindow(ImVec2(720.0f, 700.0f), ImVec2(560.0f, 500.0f));

		ImGui::Begin("Инфо", &showInfo, ImGuiWindowFlags_NoCollapse);
		{
			ImGui::Text("Горячие клавиши");

			if (ImGui::BeginTable("##Hotkeys", 2,
				ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
				ImGuiTableFlags_Resizable, ImVec2(0.0f, 390.0f)))
			{
				ImGui::TableSetupColumn("Клавиша", ImGuiTableColumnFlags_WidthFixed, 165.0f);
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

				const int numHotkeys = (int)(sizeof(hotkeys) / sizeof(hotkeys[0]));

				for (int i = 0; i < numHotkeys; ++i)
				{
					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex(0);
					ImGui::TextUnformatted(hotkeys[i].key);

					ImGui::TableSetColumnIndex(1);
					ImGui::TextWrapped(hotkeys[i].action);
				}

				ImGui::EndTable();
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();
			ImGui::Text("Мышь");

			if (ImGui::BeginTable("##MouseControlsInfo", 2,
				ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable,
				ImVec2(0.0f, 0.0f)))
			{
				ImGui::TableSetupColumn("Действие", ImGuiTableColumnFlags_WidthFixed, 165.0f);
				ImGui::TableSetupColumn("Описание", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableHeadersRow();

				struct mouse_info
				{
					const char* input;
					const char* action;
				};

				const mouse_info mouseControls[] =
				{
					{ "ЛКМ по клетке", "Выполнить выбранную функцию мыши: выбрать бота, удалить, поставить камень, загрузить объект из файла или мутировать." },
					{ "Колесико", "Приблизить или отдалить поле относительно курсора. Отдаление ограничено размером области поля в окне." },
					{ "Зажать СКМ", "Перетаскивать поле для просмотра увеличенного участка." },
				};

				const int numMouseControls = (int)(sizeof(mouseControls) / sizeof(mouseControls[0]));

				for (int i = 0; i < numMouseControls; ++i)
				{
					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex(0);
					ImGui::TextUnformatted(mouseControls[i].input);

					ImGui::TableSetColumnIndex(1);
					ImGui::TextWrapped(mouseControls[i].action);
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

void Main::DrawModeSwitchConfirmWindow()
{
	if (showModeSwitchConfirm)
	{
		ImGui::OpenPopup("Подтверждение смены режима");
		showModeSwitchConfirm = false;
	}

	if (ImGui::BeginPopupModal("Подтверждение смены режима", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextWrapped("Переключение режима остановит текущую симуляцию и создаст новый мир %s. Текущий мир не конвертируется.",
			SimulationModeName(pendingMode));

		if (ImGui::Button("Переключить", { 120, 30 }))
		{
			SwitchSimulationMode(pendingMode);
			ImGui::CloseCurrentPopup();
		}

		ImGui::SameLine();

		if (ImGui::Button("Отмена", { 90, 30 }))
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
			vlsui::PrepareToolWindow(ImVec2(500.0f, 420.0f), ImVec2(380.0f, 300.0f));

			ImGui::Begin("Данные мозга", &showBrain, ImGuiWindowFlags_NoCollapse);
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
					for (uint i = 0; i < nn_renderer.selectedNeuron->numConnections; ++i)
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
	DrawFieldScrollbars();

	//Below windows that are hidden at startup

	DrawSaveLoadWindow();
	DrawDangerousWindow();
	DrawSummaryWindow();
	DrawAdaptationWindow();
	DrawChartWindow();
	DrawBotBrainWindow();
	DrawInfoWindow();
	DrawExitConfirmWindow();
	DrawModeSwitchConfirmWindow();
}

void Main::HandleFieldNavigation()
{
	if (!IsClassicMode())
	{
		if (cb3Runtime)
		{
			cb3Runtime->HandleFieldNavigation();
			CheckRuntimeRequests();
		}
		return;
	}

	bool middleDown = (mouseState.buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
	bool mouseOverField = field->IsInBoundsScreenCoords(mouseState.mouseX, mouseState.mouseY);

	if ((mouseState.wheel != 0) && mouseOverField && !io->WantCaptureMouse)
	{
		field->ZoomAtScreenPoint(mouseState.mouseX, mouseState.mouseY, mouseState.wheel);
	}

	if (middleDown)
	{
		if (!fieldPanActive)
		{
			if (mouseOverField && !io->WantCaptureMouse)
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

void Main::MouseClick()
{
	if (!IsClassicMode())
	{
		if (cb3Runtime)
		{
			cb3Runtime->HandleMouseClick();
			CheckRuntimeRequests();
		}
		return;
	}

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
						revealSelection = true;
						field->TrackObject(selectedObject);
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
						if (selectedFile->mode != activeMode)
						{
							LogPrint("Файл сохранен в режиме ");
							LogPrint(SimulationModeName(selectedFile->mode));
							LogPrint(". Текущий режим: ");
							LogPrint(SimulationModeName(activeMode));
							LogPrint(". Загрузка отменена.\r\n");
							return;
						}

						obj = saver.LoadObject(selectedFile->pathFull);

						if (obj)
						{
							obj->x = fieldCoords.x;
							obj->y = fieldCoords.y;
							obj->energy = MaxPossibleEnergyForABot;

							if (field->AddObject(obj))
							{
								showFirstPopulationHint = false;
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

bool Main::Render()
{
	if (!IsClassicMode())
	{
		if (cb3Runtime)
		{
			bool rendered = cb3Runtime->Render();
			CheckRuntimeRequests();
			return rendered;
		}
		return false;
	}

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
		return false;
	}
	else
	{
		lastTickFps = currentTickFps;
	}

	if (selectedObject &&
		(!field->ValidateObjectExistance(selectedObject) || selectedObject->type != bot))
	{
		Deselect();
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

	return true;
}


void Main::ClearWorld()
{
	if (!IsClassicMode() || !field)
	{
		return;
	}

	Deselect();

	field->RemoveAllObjects();
	showFirstPopulationHint = true;

	ticknum = 0;
}
