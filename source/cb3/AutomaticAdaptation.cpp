#include "AutomaticAdaptation.h"


namespace cb3
{

void AutomaticAdaptation::NextPhase(const char* desc)
{
	if (phase == 0)
	{
		NewEpoch();
	}

	++phase;
	phaseDesc = desc;

	string s = "Фаза " + to_string(phase);
	s += "/" + to_string(numPhases);
	s += " тиков: " + to_string(currentTick - phaseBeginTick);

	PrintInLog(s);
}

void AutomaticAdaptation::Finish()
{
	phaseDesc = NULL;
	PrintInLog("Готово!");

	if (iteration == numIterations)
	{
		PrintInLog("Все эпохи завершены");

		if(numIterations > 1)
			CalculateHistoryAverage();

		inProgress = false;
		sim->Pause();
	}
	else
	{
		(*this.*Reset_func)();
	}
}

void ParameterSweep::SetTick(uint tick)
{
	currentTick = tick;
}

void ParameterSweep::WaitPopulation(int pop, int count)
{
	waitForPopulation = pop;
	sweep_counter = count;
	beginTick = currentTick;
}

void ParameterSweep::BeginSweep(int* p, int begin, int inc, int goal, int pop, int count, int addToChartEvery)
{
	toSweep = p;
	sweepIncrement = inc;
	sweepGoal = goal;
	waitForPopulation = pop;
	chartValPrev = 0;
	addToChartRate = addToChartEvery;

	*toSweep = begin;
	sweep_rate = count;
	sweep_counter = sweep_rate;

	beginTick = currentTick;
}

bool ParameterSweep::CheckSweep(uint numBots)
{
	if (toSweep)
	{
		if ((sweep_counter <= 0) or ((numBots > waitForPopulation * 3) and (sweep_counter <= sweep_rate/2)))
		{
			if (numBots >= waitForPopulation)
			{
				if (*toSweep == sweepGoal)
				{
					if (addToChartRate == 0)
					{
						AddToChart();
					}

					toSweep = NULL;

					return true;
				}

				*toSweep += sweepIncrement;

				if (addToChartRate > 0)
				{
					if (chartValPrev == addToChartRate)
					{
						chartValPrev = 0;
						AddToChart();
					}
					else
					{
						++chartValPrev;
					}
				}

				sweep_counter = sweep_rate;
			}
		}
		else
		{
			--sweep_counter;
		}

		return false;
	}
	else
	{
		if (numBots >= waitForPopulation)
		{
			AddToChart();
			return true;
		}
	}

	return false;
}

void ParameterSweep::Clear()
{
	currentTick = 0u;
	beginTick = 0u;
	toSweep = NULL;
}

void ParameterSweep::AddToChart()
{
	history->push_back(currentTick);
}

void AutomaticAdaptation::AutoAdaptDivers()
{
	switch (phase)
	{
	case 1:
		params->noPredators = true;
		params->botMaxEnergy = 300;

		sweep.BeginSweep(&params->adaptation_forceBotMovementsY, 
			0, 100, 1000, 10000, 30);

		NextPhase("движение по Y");
		break;

	case 2:
		params->adaptation_landBirthBlock = 1000;

		sweep.BeginSweep(&params->adaptation_botShouldDoPSOnLandOnceToMultiply,
			0, 100, 1000, 25000, 30);

		NextPhase("ФС на суше хотя бы раз");
		break;

	case 3:
		params->adaptation_seaBirthBlock = 1000;
		params->botMaxLifetime = 300;

		sweep.BeginSweep(&params->mudLevel, params->oceanLevel - 1,
			-1, 24, 20000, 200);

		NextPhase("уровень ила");
		break;

	case 4:
		sweep.BeginSweep(&params->adaptation_PSInMudBlock,
			0, 20, 1000, 30000, 150);

		NextPhase("без ФС в иле");
		break;

	case 5:
		params->botMaxLifetime = 400;

		sweep.BeginSweep(&params->adaptation_PSInOceanBlock,
			0, 10, 900, 30000, 150);

		NextPhase("без ФС в воде");
		break;

	case 6:
		sweep.BeginSweep(&params->adaptation_forceBotMovementsY,
			1000, -50, 0, 30000, 200);

		NextPhase("отключение движения по Y");
		break;

	case 7:
		params->botMaxLifetime = 600;

		sweep.BeginSweep(&params->adaptation_PSInOceanBlock,
			900, 2, 996, 30000, 200);

		NextPhase("без ФС в воде");
		break;	

	case 8:
		params->botMaxLifetime = 800;

		sweep.BeginSweep(&params->adaptation_PSInOceanBlock,
			996, 1, 1000, 30000, 400);

		NextPhase("без ФС в воде");
		break;

	case 9:
		sweep.BeginSweep(&params->adaptation_botShouldDoPSOnLandOnceToMultiply,
			1000, -10, 0, 30000, 200);

		NextPhase("снять требование ФС на суше");
		break;

	case 10:
		sweep.BeginSweep(&params->PSreward,
			25, -1, 6, 26000, 400);

		NextPhase("вернуть энергию ФС к 6");
		break;

	default:
		params->noPredators = false;
		params->botMaxLifetime = 1600;
		params->botMaxEnergy = 500;

		Finish();
	}
}

void AutomaticAdaptation::AutoAdaptWinds()
{
	switch (phase)
	{
	case 1:
		sweep.BeginSweep(&params->adaptation_forceBotMovementsX,
				0, 100, 1000, 24000,100);

		NextPhase("движение по X");
		break;

	case 2:
		sweep.BeginSweep(&params->adaptation_StepsNumToDivide_Winds,
			0, 1, 5, 15000, 60);

		NextPhase("дистанция X до деления");
		break;

	case 3:
		sweep.BeginSweep(&params->adaptation_StepsNumToDivide_Winds,
				5, 5, 145, 7000, 150, 2);

		NextPhase("дистанция X до деления");
		break;

	case 4:
		sweep.BeginSweep(&params->adaptation_StepsNumToDivide_Winds,
			145, 2, 180, 7000, 200, 4);

		NextPhase("дистанция X до деления");
		break;


	case 5:
		params->adaptation_forceBotMovementsX = 0;		

		sweep.WaitPopulation(5000, 200);
		NextPhase();
		break;

	default:
		Finish();
	}
}

void AutomaticAdaptation::NewEpoch()
{
	++iteration;

	vector<uint> v;

	history.push_back(v);
	sweep.history = &history.back();

	PrintInLog("Эпоха: " + to_string(iteration) + "/" + to_string(numIterations));
}

void AutomaticAdaptation::CalculateHistoryAverage()
{
	avg_history.clear();

	std::sort(history.begin(), history.end(), [](vector<uint> v1, vector<uint> v2) {return v1.back() < v2.back(); });
	
	for(uint b = 0; b < history[0].size(); ++b)
	{
		uint val = 0;

		for (uint i = 0; i < history.size() / 2; ++i)
		{
			val += history[i][b];			
		}

		val /= static_cast<uint>(history.size()) / 2;
		avg_history.push_back(val);
	}
}

void AutomaticAdaptation::PrintInLog(string s)
{
	s += "\r\n";

	AALog.append(s.c_str());
	sim->Print(s);
}

void AutomaticAdaptation::Plot()
{
	using namespace ImPlot;

	if (BeginPlot("Результаты", { 680, 400 }))
	{
		//Axes
		SetupAxisLimits(ImAxis_X1, 0.0, 1.0f * history[0].size(), ImPlotCond_Always);
		SetupAxisLimits(ImAxis_Y1, 0.0, 7000.0f);

		SetupAxis(ImAxis_X1, "Шаг");
		SetupAxis(ImAxis_Y1, "Время");

		SetAxis(ImAxis_Y1);		

		for(auto& h : history)
		{
			SetNextLineStyle({ 1, 1, 1, 0.3f }, 2.0f);

			PlotLine("##", h.data(), static_cast<int>(h.size()),
				1.0f, 0.0f, ImPlotLineFlags_None);
		}

		if (avg_history.size())
		{
			SetNextLineStyle({ 1, 0, 0, 1 }, 4.0f);

			PlotLine("##", avg_history.data(), static_cast<int>(avg_history.size()),
				1.0f, 0.0f, ImPlotLineFlags_None);
		}

		EndPlot();
	}
}

void AutomaticAdaptation::DrawWindow(bool* open)
{
	using namespace ImGui;

	static bool visualize = true;
	static int iterations = 1;

	SetNextWindowBgAlpha(1.0f);
	SetNextWindowSize({ 700.0f, 770.0f });
	SetNextWindowPos({ 700.0f, 250.0f }, ImGuiCond_Once);


	Begin("Автоадаптация", open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
	{
		PushStyleColor(ImGuiCol_ChildBg, ImVec4(LogBackgroundColor));

		BeginChild("Журнал автоадаптации", { 565, 200 });
		{
			TextUnformatted(AALog.Buf.Data);

			if (GetScrollY() >= GetScrollMaxY())
				SetScrollHereY(1.0f);
		}
		EndChild();

		PopStyleColor();

		NewLine();		

		if (inProgress)
		{
			Text("Прогресс шага: %i / %i", (sweep.sweep_rate - sweep.sweep_counter), sweep.sweep_rate);
			if (sweep.sweep_counter == 0)
			{
				SameLine();
				Text(" [ожидание популяции: %i / %i]", field->GetNumBots(), sweep.waitForPopulation);
			}

			if (phaseDesc)
			{
				Text("Текущая фаза: %s", phaseDesc);
			}
			else
			{
				NewLine();
			}

			NewLine();

			if (Button("Стоп", { 100, 30 }))
			{
				Stop();
			}
		}
		else
		{
			NewLine();

			if (Button("Создать ветры", { 100, 30 }))
			{	
				numIterations = iterations;
				iteration = 0u;
				history.clear();

				BeginWinds();

				if (visualize)
				{
					sim->RunWithMinimumRender();
				}
				else
				{
					sim->RunWithNoRender();
				}
			}

			SameLine();

			if (Button("Создать ныряльщиков", { 100, 30 }))
			{
				numIterations = iterations;
				iteration = 0u;
				history.clear();

				BeginDivers();				

				if (visualize)
				{
					sim->RunWithMinimumRender();
				}
				else
				{
					sim->RunWithNoRender();
				}
			}

			SameLine();

			if (Button("Очистить журнал", { 100, 30 }))
			{
				AALog.clear();
			}

			SetNextItemWidth(60);
			SameLine();
			DragInt("итерации", &iterations, 1.0f, 1, 20);

			SameLine();
			Checkbox("Визуализировать", &visualize);
		}

		NewLine();

		if(history.size() > 0)
		{
			Plot();
		}
	}
	End();
}

void AutomaticAdaptation::AdaptationStep(uint frame)
{
	currentTick = frame;

	if(inProgress)
	{
		sweep.SetTick(frame);

		if (field->GetNumBots() == 0)
		{
			PrintInLog("Попытка не удалась. Начинаю заново...");

			--iteration;
			history.pop_back();
			(*this.*Reset_func)();
		}
		else
		{
			if(sweep.CheckSweep(field->GetNumBots()))
			{
				(*this.*AA_func)();
			}
		}
	}
}

void AutomaticAdaptation::BeginDivers()
{
	Reset();

	if (BotMaxEnergyInitial < 300)
	{
		PrintInLog("Начальная энергия бота должна быть не меньше 300!");

		return;
	}

	if (FieldCellsWidth < 4000)
	{
		PrintInLog("Ширина поля должна быть больше 4000!");

		return;
	}

	PrintInLog("[Ныряльщики] Запущено...");

	inProgress = true;
	numPhases = 11u;
	AA_func = &AutomaticAdaptation::AutoAdaptDivers;
	Reset_func = &AutomaticAdaptation::BeginDivers;
	params->PSreward = 25;

	field->placeWall();

	sweep.WaitPopulation(10000, 100);

	NextPhase();
}

void AutomaticAdaptation::BeginWinds()
{
	if (FieldCellsWidth < 350)
	{
		PrintInLog("Ширина поля должна быть больше 350!");

		return;
	}

	Reset();

	PrintInLog("[Ветры] Запущено...");

	params->mudLevel = 0;
	params->oceanLevel = FieldCellsHeight;
	params->noPredators = true;
	params->PSreward = 12;

	inProgress = true;
	numPhases = 6u;
	AA_func = &AutomaticAdaptation::AutoAdaptWinds;
	Reset_func = &AutomaticAdaptation::BeginWinds;

	NextPhase();
}

void AutomaticAdaptation::Stop()
{
	inProgress = false;
}

void AutomaticAdaptation::Reset()
{		
	sweep.Clear();
	sim->ClearWorld();
	params->Reset();
	field->RemoveAllObjects();
	field->SpawnControlGroup();

	phase = 0u;
	phaseBeginTick = 0u;
	currentTick = 0u;
	adaptationBeginTick = 0u;
}

AutomaticAdaptation::AutomaticAdaptation(Field* f, Main* m)
{
	field = f;
	params = &f->params;
	sim = m;
}


}
