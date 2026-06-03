
#include "Chart.h"



namespace cb3
{

bool Chart::Tick()
{
	if (timeBeforeNextDataToChart-- == 0)
	{
		timeBeforeNextDataToChart = AddToChartEvery;

		return true;
	}
	else
	{
		return false;
	}
}

void Chart::ClearChart()
{
	memset(chartData_bots, 0, sizeof(chartData_bots));
	memset(chartData_organics, 0, sizeof(chartData_organics));
	memset(chartData_apples, 0, sizeof(chartData_apples));
	memset(chartData_predators, 0, sizeof(chartData_predators));
	memset(chartData_avg_lifetime, 0, sizeof(chartData_avg_lifetime));

	chart_numValues = 0;
	chart_currentPosition = 0;
}

void Chart::AddToChart(float newVal_bots, float newVal_apples, float newVal_organics, float newVal_predators, float newVal_avg_lifetime)
{
	chartData_bots[chart_currentPosition] = newVal_bots;
	chartData_apples[chart_currentPosition] = newVal_apples;
	chartData_organics[chart_currentPosition] = newVal_organics;
	chartData_predators[chart_currentPosition] = newVal_predators;
	chartData_avg_lifetime[chart_currentPosition] = newVal_avg_lifetime;

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

void Chart::Plot()
{
	using namespace ImPlot;

	if (BeginPlot("Объекты", { 800, 550 }))
	{
		//Axes
		SetupAxisLimits(ImAxis_X1, 0.0, 250.0, ImPlotCond_Always);
		SetupAxisLimits(ImAxis_Y1, 0.0, 26000.0);
		SetupAxisLimits(ImAxis_Y2, 0.0, 1000.0);

		SetupAxis(ImAxis_X1, "Время");
		SetupAxis(ImAxis_Y1);
		SetupAxis(ImAxis_Y2, NULL, ImPlotAxisFlags_Opposite | ImPlotAxisFlags_NoGridLines);

		SetAxis(ImAxis_Y1);

		//Bots
		SetNextLineStyle({ ChartBotsColor }, ChartLineThickness);

		PlotLine("Боты", chartData_bots, chart_numValues - 1,
			1.0f, 0.0f, ImPlotLineFlags_None);

		//Apples
		if (chartShow_apples)
		{
			SetNextLineStyle({ ChartApplesColor }, ChartLineThickness);

			PlotLine("Яблоки", chartData_apples, chart_numValues - 1,
				1.0f, 0.0f, ImPlotLineFlags_None);
		}

		//Organics
		if (chartShow_organics)
		{
			SetNextLineStyle({ ChartOrganicsColor }, ChartLineThickness);

			PlotLine("Органика", chartData_organics, chart_numValues - 1,
				1.0f, 0.0f, ImPlotLineFlags_None);
		}

		SetAxis(ImAxis_Y2);

		//Predators
		if (chartShow_predators)
		{
			SetNextLineStyle({ ChartPredatorsColor }, ChartLineThickness);

			PlotLine("Хищники", chartData_predators, chart_numValues - 1,
				1.0f, 0.0f, ImPlotLineFlags_None);
		}

		//AVG lifetime
		if (chartShow_avg_lifetime)
		{
			SetNextLineStyle({ ChartAVGLifetimeColor }, ChartLineThickness);

			PlotLine("Средний возраст", chartData_avg_lifetime, chart_numValues - 1,
				1.0f, 0.0f, ImPlotLineFlags_None);
		}

		EndPlot();
	}

	ImGui::SameLine();

	ImGui::BeginGroup();
	{
		if (ImGui::Button("Очистить", { 100.0f, 30.0f }))
			ClearChart();

		ImGui::Text("Левая ось:");

		ImGui::Checkbox("Яблоки", &chartShow_apples);
		ImGui::Checkbox("Органика", &chartShow_organics);

		ImGui::NewLine();

		ImGui::Text("Правая ось:");

		ImGui::Checkbox("Хищники", &chartShow_predators);
		ImGui::Checkbox("Средний возраст", &chartShow_avg_lifetime);
	}
	ImGui::EndGroup();
}


}
