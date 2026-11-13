#include "GUI.h"
#include "../UITheme.h"
#include "../BrainWindow.h"

namespace cb3
{

using namespace ImGui;


void InitImGUI()
{
	CreateContext();

	io = &GetIO();
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

		uint threads = field->GetNumThreads();
		Text(", потоков: %i", threads);
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

void Main::DrawSidePanelWindow()
{
	RecordChartSample();
	const auto layout = vlsui::CalculateLayout(windowWidth, windowHeight);
	int displayMode = (int)renderType;
	const auto actions =
		vlsui::DrawWorkspaceHeader(layout, simulate, limit_ticks_per_second, GUI_Max_tps, showFirstPopulationHint);
	if (actions.seed)
		SpawnInitialPopulation();
	if (actions.pause)
		SwitchPause();
	if (actions.step)
		MakeStep();
	if (actions.saves)
	{
		LoadFilenames();
		showSaveLoad = true;
	}
	bool chooseBot = false;
	mouseFunc = (MouseFunction)vlsui::DrawWorkspaceTools(layout, (int)mouseFunc, brushSize, 1,
		savedBotPicker.sourceName.c_str(), chooseBot);
	if (chooseBot)
	{
		LoadFilenames();
		savedBotPicker.Open();
	}

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
					if (object->type() == bot)
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
				auto* selectedBot = (Bot*)selectedObject;
				vlsui::DrawFeedingTitle(selectedBot->GetEnergyFromPS(), selectedBot->GetEnergyFromKills(),
					selectedBot->GetEnergyFromOrganics());
				ImGui::Text("Бот  •  X %i  •  Y %i", selectedObject->x, selectedObject->y);
				ImGui::Text("Возраст: %i / %i", selectedObject->GetLifetime(), field->params.botMaxLifetime);
				ImGui::Text("Энергия: %i / %i", selectedObject->energy, field->params.botMaxEnergy);
				ImGui::ProgressBar((float)selectedObject->energy / field->params.botMaxEnergy, ImVec2(-1.0f, 6.0f), "");
				vlsui::DrawFeedingSources(selectedBot->GetEnergyFromPS(), selectedBot->GetEnergyFromKills(),
					selectedBot->GetEnergyFromOrganics());

				ImGui::TextDisabled("Метки родства");
				for (int i = 0; i < NumberOfMutationMarkers; ++i)
				{
					if (i)
						ImGui::SameLine();
					ImGui::Text("%i", ((Bot *)selectedObject)->GetMarkers()[i]);
				}
				Color &color = *((Bot *)selectedObject)->GetColor();
				ImGui::ColorButton("##Cb3BotColor",
								   ImVec4(color.c[0] / 255.0f, color.c[1] / 255.0f, color.c[2] / 255.0f, 1.0f),
								   ImGuiColorEditFlags_NoTooltip, ImVec2(28.0f, 28.0f));
				ImGui::SameLine();
				if (ImGui::Button("Цвет линии", ImVec2(130.0f, 28.0f)))
					field->RepaintBot((Bot *)selectedObject, Color::GetRandomColor(), RepaintTolerance);
				vlsui::ItemTooltip("Перекрасить выбранного бота и его близких родственников в случайный цвет.");
				ImGui::SameLine();
				if (ImGui::Button("Мозг", ImVec2(-1.0f, 28.0f)))
					showBrain = !showBrain;
			}
			else
			{
				if (selectedObject)
					Deselect();
				ImGui::TextDisabled("Ничего не выбрано.");
				ImGui::TextWrapped("Выберите инструмент «Выбор» справа и нажмите на бота в мире.");
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
			ImGui::SliderInt("##InspectorPS", &field->params.PSreward, 0, GUI_Max_food, "%d");
			ImGui::Checkbox("Появление яблок", &field->params.spawnApples);
			if (ImGui::Button("Все условия среды", ImVec2(-1.0f, 34.0f)))
				showAdaptation = true;
			vlsui::ItemTooltip("C · ограничения размножения, отбор, органика и остальные условия");
			vlsui::SectionTitle("Отрисовка");
			ImGui::TextDisabled("Лимит кадров в секунду");
			ImGui::SetNextItemWidth(-1.0f);
			ImGui::SliderInt("##InspectorFPS", &limitFPS, 0, GUI_Max_fps, "%d кадров/с");
			ImGui::TextWrapped("0 — без ограничения. Режимы просмотра и отключение отрисовки находятся на нижней панели.");
			if (ImGui::Button("Автоадаптация", ImVec2(-1.0f, 34.0f)))
				showAutomaticAdaptation = true;

			ImGui::EndChild();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Мир"))
		{
			ImGui::BeginChild("##WorldInspectorContent", ImVec2(0.0f, 0.0f), false);
			vlsui::SectionTitle("Состояние мира");
			if (ImGui::BeginTable("##Cb3Metrics", 2, ImGuiTableFlags_SizingStretchSame))
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

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextDisabled("Средний возраст");
				ImGui::Text("%i", field->GetAverageLifetime());
				ImGui::TableSetColumnIndex(1);
				ImGui::TextDisabled("Размер мира");
				ImGui::Text("%i клеток", FieldCellsWidth);
				ImGui::EndTable();
			}

			if (field->params.useSeasons)
			{
				ImGui::TextDisabled("Сезон: %s  •  %i / %i", SeasonNames[field->GetSeason()], field->GetSeasonCounter(),
									field->params.seasonInterval);
			}

			vlsui::SectionTitle("Быстрые действия");
			float actionWidth = vlsui::HalfWidth();
			if (ImGui::Button("Стена", ImVec2(actionWidth, 34.0f)))
				PlaceWorldWall();
			vlsui::ItemTooltip("Вертикальная стена (F2)");
			ImGui::SameLine();
			if (ImGui::Button("Органика", ImVec2(actionWidth, 34.0f)))
				DropWorldOrganics();
			vlsui::ItemTooltip("Добавить органику (F3)");
			if (ImGui::Button("Камни", ImVec2(actionWidth, 34.0f)))
				SpawnWorldRocks();
			vlsui::ItemTooltip("Случайные камни (F4)");
			ImGui::SameLine();
			if (vlsui::ColoredButton("Мутация F11", ImVec2(actionWidth, 34.0f), vlsui::Warning()))
				MutateWholeWorld();
			vlsui::ItemTooltip("Солнечная вспышка — мутировать ботов (F11)");

			vlsui::SectionTitle("Быстрое сохранение", "отдельный слот CyberBiology3");
			if (ImGui::Button("Сохранить F5", ImVec2(actionWidth, 34.0f)))
				QuickSaveWorld();
			vlsui::ItemTooltip("F5 — перезаписывает быстрый слот CyberBiology3");
			ImGui::SameLine();
			const bool hasCb3QuickSave = std::filesystem::exists(QuicksaveFilename);
			ImGui::BeginDisabled(!hasCb3QuickSave);
			if (ImGui::Button("Загрузить F9", ImVec2(actionWidth, 34.0f)))
				QuickLoadWorld();
			ImGui::EndDisabled();
			vlsui::ItemTooltip(hasCb3QuickSave ? "F9 — заменяет текущий мир быстрым сохранением"
											   : "Быстрое сохранение CyberBiology3 еще не создано");

			if (ImGui::CollapsingHeader("Техническая информация"))
			{
				int logicalProcessors = SDL_GetCPUCount();
				ImGui::Text("Платформа: %s", SDL_GetPlatform());
				ImGui::Text("Процессоры: %d", logicalProcessors);
				ImGui::Text("Память: %.2f ГБ", SDL_GetSystemRAM() / 1024.0f);
				const uint simulationThreads = field->GetNumThreads();
				ImGui::Text("Потоки симуляции: %u", simulationThreads);
				if (simulationThreads > (uint)logicalProcessors)
				{
					ImGui::TextColored(vlsui::Warning(), "Потоков больше, чем процессоров");
				}
				ImGui::TextDisabled("Seed %u  •  ID %u", seed, id);
				ImGui::TextDisabled("Слоев %i  •  нейронов %i", NumNeuronLayers, NumHiddenNeurons);
			}

			ImGui::EndChild();
			ImGui::EndTabItem();
		}
		if (ImGui::BeginTabItem("Ещё"))
		{
			ImGui::BeginChild("##WindowsInspectorContent", ImVec2(0.0f, 0.0f), false);
			vlsui::SectionTitle("Окна");
			if (ImGui::BeginTable("##Cb3Windows", 2, ImGuiTableFlags_SizingStretchSame))
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
				if (ImGui::Button("Автоадаптация", ImVec2(-1.0f, 34.0f)))
					showAutomaticAdaptation = !showAutomaticAdaptation;
				ImGui::TableSetColumnIndex(1);
				if (ImGui::Button("Выход", ImVec2(-1.0f, 34.0f)))
					feedback.Confirm("Выйти из приложения?", "Текущий мир будет закрыт. Сохраните эксперимент перед выходом.", [this] { terminate = true; });
				ImGui::EndTable();
			}

			vlsui::SectionTitle("Режим симуляции");
			if (vlsui::ColoredButton("Перейти в Classic", ImVec2(-1.0f, 36.0f), vlsui::Accent()))
			{
				showModeSwitchConfirm = true;
			}

			if (ImGui::CollapsingHeader("Журнал"))
			{
				ImGui::BeginChild("##Cb3Log", ImVec2(0.0f, 100.0f), true);
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

	vlsui::DrawWorkspaceFooter(layout, "CyberBiology3", simulate, displayedBots, displayedObjects, ticknum, realTPS, realFPS,
		displayMode, showChart, [this](const ImVec2& size) { chart.PlotPreview(size, showChart); });
	renderType = (RenderTypes)displayMode;

	if (showFirstPopulationHint && ticknum == 0 && mouseFunc == mouse_select && renderType != noRender)
	{
		if (vlsui::DrawEmptyWorld(layout.world))
			SpawnInitialPopulation();
	}
}

void Main::DrawSavedBotPicker()
{
	savedBotPicker.Draw(allFilenames, SimulationMode::CyberBiology3, [this](const std::filesystem::path& path)
	{
		std::unique_ptr<Object> object(saver.LoadObject(path));
		if (!object || object->type() != bot) return false;
		mouseFunc = mouse_from_file;
		return true;
	}, [this] { LoadFilenames(); });
}

void Main::DrawSaveLoadWindow()
{
	if (!showSaveLoad)
	{
		saveFileNameInputActive = false;
		return;
	}

	SetNextWindowBgAlpha(1.0f);
	vlsui::PrepareToolWindow(ImVec2(900.0f, 640.0f), ImVec2(700.0f, 520.0f));

	Begin("Сохранения и загрузка", &showSaveLoad, ImGuiWindowFlags_NoCollapse);
	{
		TextColored(vlsui::Accent(), "CYBERBIOLOGY3");
		SameLine();
		TextDisabled("/ SavedObjects/CyberBiology3");
		TextDisabled("Сохранения этого режима отделены от Classic и используют собственный быстрый слот.");

		vlsui::SectionTitle("Новое сохранение", "пустое имя создаст имя по дате и времени");
		bool newNameInputActive = false;
		const bool canSaveBot = selectedObject && field->ValidateObjectExistance(selectedObject);

		if (BeginTable("##Cb3CreateSave", 3, ImGuiTableFlags_SizingStretchProp))
		{
			TableSetupColumn("##Cb3SaveName", ImGuiTableColumnFlags_WidthStretch, 1.7f);
			TableSetupColumn("##Cb3SaveWorld", ImGuiTableColumnFlags_WidthStretch, 1.0f);
			TableSetupColumn("##Cb3SaveBot", ImGuiTableColumnFlags_WidthStretch, 1.0f);
			TableNextRow();
			TableSetColumnIndex(0);
			SetNextItemWidth(-1.0f);
			InputTextWithHint("##Cb3NewSaveName", "Имя нового сохранения", newSaveFileName, sizeof(newSaveFileName));
			newNameInputActive = IsItemActive();

			TableSetColumnIndex(1);
			if (vlsui::ColoredButton("Сохранить мир", ImVec2(-1.0f, 34.0f), vlsui::Accent()))
				SaveWorldToNamedFile();

			TableSetColumnIndex(2);
			BeginDisabled(!canSaveBot);
			if (Button("Сохранить бота", ImVec2(-1.0f, 34.0f)))
				SaveSelectedObjectToNamedFile();
			EndDisabled();
			vlsui::ItemTooltip(canSaveBot ? "Сохранить выбранного бота" : "Сначала выберите бота в мире");
			EndTable();
		}

		vlsui::SectionTitle("Библиотека сохранений");
		if (vlsui::ModeButton("Миры", !showBotSaves, ImVec2(110, 30)))
		{
			if (showBotSaves) SelectFile(-1);
			showBotSaves = false;
		}
		ImGui::SameLine();
		if (vlsui::ModeButton("Боты", showBotSaves, ImVec2(110, 30)))
		{
			if (!showBotSaves) SelectFile(-1);
			showBotSaves = true;
		}
		int visibleFiles = 0;
		for (const auto& file : allFilenames)
			if (showBotSaves ? file.fileType == "бот" : file.isWorld) ++visibleFiles;
		TextDisabled("Файлов: %d", visibleFiles);
		SameLine();
		const float refreshWidth = 105.0f;
		SetCursorPosX(GetWindowContentRegionMax().x - refreshWidth);
		if (Button("Обновить", ImVec2(refreshWidth, 0.0f)))
			LoadFilenames();

		float fileTableHeight = GetContentRegionAvail().y - 190.0f;
		if (fileTableHeight < 100.0f)
			fileTableHeight = 100.0f;

		if (BeginTable("##SaveFilesCB3", 4,
			ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
			ImGuiTableFlags_Resizable, ImVec2(0.0f, fileTableHeight)))
		{
			TableSetupColumn("Имя", ImGuiTableColumnFlags_WidthStretch);
			TableSetupColumn("Размер", ImGuiTableColumnFlags_WidthFixed, 70.0f);
			TableSetupColumn("Режим", ImGuiTableColumnFlags_WidthFixed, 115.0f);
			TableSetupColumn("Дата", ImGuiTableColumnFlags_WidthFixed, 125.0f);
			TableHeadersRow();

			if (visibleFiles == 0)
			{
				TableNextRow();
				TableSetColumnIndex(0);
				TextDisabled(showBotSaves ? "Сохранённых ботов пока нет" : "Сохранённых миров пока нет");
			}

			for (int i = 0; i < allFilenames.size(); ++i)
			{
				if (showBotSaves ? allFilenames[i].fileType != "бот" : !allFilenames[i].isWorld) continue;
				TableNextRow();
				TableSetColumnIndex(0);

				PushID(i);
				if (Selectable(allFilenames[i].nameShort.c_str(), allFilenames[i].isSelected, ImGuiSelectableFlags_SpanAllColumns))
					SelectFile(i);
				PopID();

				TableSetColumnIndex(1);
				TextUnformatted(allFilenames[i].fileSize.c_str());
				TableSetColumnIndex(2);
				TextUnformatted(allFilenames[i].modeText.c_str());
				TableSetColumnIndex(3);
				TextUnformatted(allFilenames[i].modifiedTimeText.c_str());
			}

			EndTable();
		}

		bool renameInputActive = false;
		if (selectedFile)
		{
			if (BeginTable("##Cb3RenameRow", 3, ImGuiTableFlags_SizingStretchProp))
			{
				TableSetupColumn("##Cb3RenameLabel", ImGuiTableColumnFlags_WidthFixed, 85.0f);
				TableSetupColumn("##Cb3RenameInput", ImGuiTableColumnFlags_WidthStretch);
				TableSetupColumn("##Cb3RenameButton", ImGuiTableColumnFlags_WidthFixed, 145.0f);
				TableNextRow();
				TableSetColumnIndex(0);
				AlignTextToFramePadding();
				TextUnformatted("Имя файла");
				TableSetColumnIndex(1);
				SetNextItemWidth(-1.0f);
				InputText("##RenameFileNameCB3", renameFileName, sizeof(renameFileName));
				renameInputActive = IsItemActive();
				TableSetColumnIndex(2);
				if (Button("Переименовать", ImVec2(-1.0f, 0.0f)))
					RenameSelectedFile();
				EndTable();
			}
		}
		else
		{
			TextDisabled("Выберите сохранение, чтобы загрузить, переименовать или удалить его.");
		}

		saveFileNameInputActive = newNameInputActive || renameInputActive;
		const bool canLoadSelected = selectedFile && selectedFile->mode == SimulationMode::CyberBiology3;

		if (selectedFile && !canLoadSelected)
		{
			TextColored(vlsui::Warning(), "Файл создан в другом режиме и не может быть загружен в CyberBiology3.");
		}

		if (BeginTable("##Cb3FileActions", 2, ImGuiTableFlags_SizingStretchSame))
		{
			TableNextRow();
			TableSetColumnIndex(0);
			BeginDisabled(!canLoadSelected);
			const char* loadLabel = showBotSaves
				? (selectedObject ? "Заменить выбранного бота…" : "Загрузить бота") : "Загрузить мир…";
			if (vlsui::ColoredButton(loadLabel, ImVec2(-1.0f, 34.0f), vlsui::Accent()))
			{
				if (selectedFile->isWorld)
				{
					RequestWorldLoad(selectedFile->pathFull);
				}
				else
				{
					const auto botPath = selectedFile->pathFull;
					auto loadBot = [this, botPath] {
						Object* loadedObject = saver.LoadObject(botPath);

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
								LogPrint("Ошибка загрузки объекта: клетка занята или вне поля\r\n", false, true);
							}
						}
						else
							LogPrint("Ошибка загрузки объекта\r\n", false, true);
					};
					if (selectedObject && field->ValidateObjectExistance(selectedObject))
						feedback.Confirm("Заменить выбранного бота?", "Выбранный бот будет удалён и заменён ботом из файла. Для размещения копий на пустых клетках используйте инструмент «Из файла».", loadBot);
					else
						loadBot();
				}
			}
			EndDisabled();

			TableSetColumnIndex(1);
			BeginDisabled(!selectedFile);
			if (Button("Удалить в корзину", ImVec2(-1.0f, 34.0f)))
				DeleteSelectedFile();
			EndDisabled();
			EndTable();
		}

		if (selectedFile && selectedFile->isWorld && canLoadSelected)
		{
			TextDisabled("Частичная загрузка добавляет данные к текущему миру без замены параметров.");
			if (BeginTable("##Cb3PartialLoadActions", 2, ImGuiTableFlags_SizingStretchSame))
			{
				TableNextRow();
				TableSetColumnIndex(0);
				if (Button("Добавить только ландшафт", ImVec2(-1.0f, 34.0f)))
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
						LogPrint("Ошибка загрузки ландшафта\r\n", false, true);
				}

				TableSetColumnIndex(1);
				if (Button("Добавить только ботов", ImVec2(-1.0f, 34.0f)))
				{
					ObjectSaver::WorldParams ret = saver.LoadWorld(field, selectedFile->pathFull,
						false, false, false, true);

					if (ret.id != -1)
					{
						if (ret.width != FieldCellsWidth)
							LogPrint("Боты загружены (ширина не совпадает)\r\n");
						else
							LogPrint("Боты загружены\r\n");
					}
					else
						LogPrint("Ошибка загрузки ботов\r\n", false, true);
				}
				EndTable();
			}
		}
	}
	End();
}

void Main::DrawDangerousWindow()
{
    if (!showDangerous) return;
    vlsui::PrepareToolWindow(ImVec2(460, 240), ImVec2(340, 200));
    ImGui::Begin("Инструменты", &showDangerous, ImGuiWindowFlags_NoCollapse);
    ImGui::TextWrapped("Эти действия меняют весь мир. Перед выполнением появится подтверждение.");
    if (vlsui::ColoredButton("Очистить мир…", ImVec2(-1, 36), ImVec4(0.54f, 0.22f, 0.19f, 1)))
        feedback.Confirm("Очистить мир?", "Будут удалены все объекты, обнулено время и очищена история графиков. Параметры среды также вернутся к начальным значениям.", [this] {
            ClearWorld(); field->params.Reset();
            LogPrint("Мир очищен\r\n");
        });
    if (ImGui::Button("Удалить всех ботов…", ImVec2(-1, 36)))
        feedback.Confirm("Удалить всех ботов?", "Все живые боты будут удалены без превращения в органику. Камни и пища останутся.", [this] {
            Deselect();
            for (int x = 0; x < FieldCellsWidth; ++x)
                for (int y = 0; y < FieldCellsHeight; ++y)
                    if (auto* o = field->GetObjectLocalCoords(x, y); o && o->type() == bot)
                        field->RemoveObject(x, y);
            RecordChartSample(true, "Боты удалены вручную");
            LogPrint("Все боты удалены\r\n");
        });
    if (ImGui::Button("Обнулить время…", ImVec2(-1, 36)))
        feedback.Confirm("Обнулить время?", "Счётчик тиков начнётся с нуля, история графиков будет очищена. Объекты и их возраст сохранятся.", [this] {
            ticknum = 0;
            chart.ClearChart();
            RecordChartSample(true, "Время обнулено");
            LogPrint("Время обнулено, начата новая история графиков\r\n");
        });
    ImGui::End();
}

void Main::DrawAdaptationWindow()
{
    if (!showAdaptation) return;
    vlsui::PrepareToolWindow(ImVec2(620, 720), ImVec2(460, 460));
    ImGui::Begin("Условия среды", &showAdaptation, ImGuiWindowFlags_NoCollapse);
    ImGui::TextWrapped("Изменения действуют сразу. Вероятности: 0 %% — правило отключено, 100 %% — срабатывает при каждой проверке.");
    ImGui::TextWrapped("Наведите на настройку для пояснения. Ctrl + щелчок по шкале — точный ввод.");
    auto& p = field->params;
    if (ImGui::CollapsingHeader("Отбор по движению"))
    {
        vlsui::IntegerSetting("Удаление от места рождения перед делением в воде", &p.adaptation_StepsNumToDivide_Winds,
            0, 200, "%d клеток по X", "Для размножения в воде бот должен отдалиться от места рождения по X на это расстояние. Считается расстояние, а не число сделанных шагов. 0 — ограничение отключено.");
        vlsui::ProbabilitySlider("Гибель при недостатке движений по X", &p.adaptation_forceBotMovementsX,
            "После первых 3 тиков: если число движений по X меньше (возраст − 3) / 3, бот может погибнуть с этой вероятностью в каждом тике.");
        vlsui::ProbabilitySlider("Гибель при недостатке движений по Y", &p.adaptation_forceBotMovementsY,
            "После первых 3 тиков: если число движений по Y меньше (возраст − 3) / 3, бот может погибнуть с этой вероятностью в каждом тике.");
    }
    if (ImGui::CollapsingHeader("Вода, суша и размножение", ImGuiTreeNodeFlags_DefaultOpen))
    {
        vlsui::ProbabilitySlider("Вероятность запрета рождения на суше", &p.adaptation_landBirthBlock,
            "Отменяет попытку размножения на суше с указанной вероятностью.");
        vlsui::ProbabilitySlider("Вероятность запрета рождения в воде", &p.adaptation_seaBirthBlock,
            "Отменяет попытку размножения в воде над слоем ила. На ил это правило не распространяется.");
        vlsui::ProbabilitySlider("Вероятность блокировки фотосинтеза в воде", &p.adaptation_PSInOceanBlock,
            "Бот не получит энергию при попытке фотосинтеза в воде над илом.");
        vlsui::ProbabilitySlider("Вероятность блокировки фотосинтеза в иле", &p.adaptation_PSInMudBlock,
            "Бот не получит энергию при попытке фотосинтеза в слое ила.");

        vlsui::ProbabilitySlider("Запрет рождения без 4 фотосинтезов на суше", &p.adaptation_botShouldDoPSOnLandOnceToMultiply,
            "Пока бот не выполнил фотосинтез на суше хотя бы 4 раза, попытка размножения блокируется с указанной вероятностью.");
        vlsui::IntegerSetting("Глубина океана", &p.oceanLevel, 0, FieldCellsHeight, "%d клеток",
            "Число нижних рядов мира, занятых водой. 0 — воды нет.");
        vlsui::IntegerSetting("Толщина слоя ила", &p.mudLevel, 0, FieldCellsHeight, "%d клеток",
            "Число нижних рядов, считающихся илом. Ил виден внутри водной области.");
    }
    if (ImGui::CollapsingHeader("Пища"))
    {
        vlsui::ProbabilitySlider("Вероятность органики после гибели бота", &p.adaptation_organicSpawnRate,
            "При естественной гибели бот может оставить органику с оставшейся положительной энергией. Это не частота случайного появления пищи.");
        vlsui::IntegerSetting("Энергия нового яблока", &p.appleEnergy, 1, 200, "%d ед.",
            "Запас энергии яблок, которые появятся после изменения настройки.");
        ImGui::Checkbox("Создавать яблоки", &p.spawnApples);
        vlsui::ItemTooltip("Разрешить появление новых яблок на суше. Уже существующие яблоки сохраняются.");
    }
    if (ImGui::CollapsingHeader("Жизнь и наследование"))
    {
        vlsui::IntegerSetting("Предельный возраст бота", &p.botMaxLifetime, MaxBotLifetime_Min, MaxBotLifetime_Max, "%d тиков",
            "При достижении этого возраста бот погибает.");
        vlsui::IntegerSetting("Предельный запас энергии", &p.botMaxEnergy, 100, 2000, "%d ед.",
            "Верхняя граница энергии бота.");
        vlsui::IntegerSetting("Задержка размножения", &p.fertility_delay, 0, p.botMaxLifetime, "%d тиков",
            "Минимальный возраст размножения для новых ботов. Между рождениями эта задержка не повторяется; уже живущие боты сохраняют прежний порог.");
        ImGui::Checkbox("Без мутаций при рождении", &p.noMutations);
        vlsui::ItemTooltip("Отключает наследственные мутации при рождении. Ручные изменения мозга и мутаген остаются доступны.");
        ImGui::Checkbox("Гибель при попытке атаки", &p.noPredators);
        vlsui::ItemTooltip("Бот погибает, когда мозг выбирает атаку, даже если перед ним нет жертвы. Это жёсткий отбор против атакующего поведения.");
    }
    if (ImGui::CollapsingHeader("Сезоны"))
    {
        ImGui::Checkbox("Использовать сезоны", &p.useSeasons);
        vlsui::ItemTooltip("Периодически меняет награду за фотосинтез на суше. Под водой сезонный множитель не применяется.");
        vlsui::IntegerSetting("Интервал смены сезона", &p.seasonInterval, 200, 4000, "%d тиков",
            "Число тиков между сменами сезона при включённых сезонах.");
    }
    ImGui::Spacing();
    if (ImGui::Button("Вернуть начальные условия…", ImVec2(-1, 34)))
        feedback.Confirm("Сбросить условия среды?", "Параметры среды вернутся к начальным значениям режима. Объекты и время мира сохранятся.", [this] {
            field->params.Reset();
            RecordChartSample(true, "Условия среды сброшены");
            LogPrint("Начальные условия среды восстановлены\r\n");
        });
    ImGui::End();
}

void Main::DrawChartWindow()
{
	chart.DrawWindow(showChart);
}


void Main::DrawBotBrainWindow()
{
    // The selected bot keeps its last neuron values for the window (the
    // network values themselves are only a temporary buffer during think()).
    const bool selectedBot = selectedObject && selectedObject->type() == bot &&
        field->ValidateObjectExistance(selectedObject);
    Bot::SetObservedBot(selectedBot ? (Bot*)selectedObject : nullptr);

    if (!showBrain)
    {
        nn_renderer.history.Clear();
        nn_renderer.ResetSelection();
        if (selectedBot) ((Bot*)selectedObject)->CompactBrain();
        return;
    }
    if (!selectedBot)
    {
        Deselect();
        return;
    }
    auto bot = (Bot*)selectedObject;
    nn_renderer.history.SetOwner(selectedObject);
    // Views give the bot its own genome copy: edits do not touch relatives.
    auto current = bot->GetActiveBrain();
    auto initial = bot->GetInitialBrain();
    vlsui::DrawBrainWindow(showBrain, brainToShow, &current, &initial,
        nn_renderer, bot->GetNeuronSummary(), BiasMultiplier, WeightMultiplier, true);
    bot->RecheckBrain();
    if (!showBrain)
    {
        nn_renderer.history.Clear();
        nn_renderer.ResetSelection();
        bot->CompactBrain();
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
	DrawAdaptationWindow();
	DrawChartWindow();
	DrawBotBrainWindow();
	DrawAAWindow();
	DrawModeSwitchConfirmWindow();
	DrawSavedBotPicker();
	feedback.Draw();
}

void Main::MouseClick()
{
	if (!feedback.HasConfirmation() && !io->WantCaptureMouse)
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
						if (selectedObject != obj)
						{
							const bool reopenBrain = showBrain;
							Deselect();
							showBrain = reopenBrain;
						}
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
				if (!obj)
				{
					std::unique_ptr<Object> loaded(saver.LoadObject(savedBotPicker.sourcePath));
					if (!loaded || loaded->type() != bot)
					{
						savedBotPicker.ClearSource();
						LoadFilenames();
						savedBotPicker.Open();
						return;
					}
					loaded->x = fieldCoords.x;
					loaded->y = fieldCoords.y;
					loaded->energy = field->params.botMaxEnergy;
					if (field->AddObject(loaded.get()))
					{
						loaded.release();
						showFirstPopulationHint = false;
						LogPrint("Объект загружен\r\n");
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

bool Main::Render()
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
		return false;
	}
	else
	{
		lastTickFps = currentTickFps;
	}

	if (selectedObject &&
		(!field->ValidateObjectExistance(selectedObject) || selectedObject->type() != bot))
	{
		Deselect();
	}

	//Begin frame

	//Clear background
	UpdateWindowSize();
	SDLBeginScene();

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

	return true;
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
	showFirstPopulationHint = true;

	ticknum = 0;
	chart.ClearChart();
	RecordChartSample(true, "Мир очищен");
}


}
