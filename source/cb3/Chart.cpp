
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

	if (ImGui::BeginTable("##Cb3ChartControls", 4, ImGuiTableFlags_SizingStretchProp))
	{
		ImGui::TableSetupColumn("##Cb3ChartAction", ImGuiTableColumnFlags_WidthFixed, 110.0f);
		ImGui::TableSetupColumn("##Cb3ChartAxis", ImGuiTableColumnFlags_WidthFixed, 90.0f);
		ImGui::TableSetupColumn("##Cb3ChartSeriesA", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("##Cb3ChartSeriesB", ImGuiTableColumnFlags_WidthStretch);

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(0);
		if (ImGui::Button("Очистить", ImVec2(-1.0f, 32.0f)))
			ClearChart();
		ImGui::TableSetColumnIndex(1);
		ImGui::AlignTextToFramePadding();
		ImGui::TextDisabled("Левая ось");
		ImGui::TableSetColumnIndex(2);
		ImGui::Checkbox("Яблоки", &chartShow_apples);
		ImGui::TableSetColumnIndex(3);
		ImGui::Checkbox("Органика", &chartShow_organics);

		ImGui::TableNextRow();
		ImGui::TableSetColumnIndex(1);
		ImGui::AlignTextToFramePadding();
		ImGui::TextDisabled("Правая ось");
		ImGui::TableSetColumnIndex(2);
		ImGui::Checkbox("Хищники", &chartShow_predators);
		ImGui::TableSetColumnIndex(3);
		ImGui::Checkbox("Средний возраст", &chartShow_avg_lifetime);
		ImGui::EndTable();
	}

	if (BeginPlot("Объекты", ImVec2(-1.0f, -1.0f)))
	{
		const int chartValuesToPlot = chart_numValues;

		//Axes
		SetupAxis(ImAxis_X1, "Время");
		SetupAxis(ImAxis_Y1);
		SetupAxis(ImAxis_Y2, NULL, ImPlotAxisFlags_Opposite | ImPlotAxisFlags_NoGridLines);

		SetupAxisLimits(ImAxis_X1, 0.0, 250.0, ImPlotCond_Always);
		SetupAxisLimits(ImAxis_Y1, 0.0, 26000.0);
		SetupAxisLimits(ImAxis_Y2, 0.0, 1000.0);

		SetAxis(ImAxis_Y1);

		//Bots
		SetNextLineStyle({ ChartBotsColor }, ChartLineThickness);

		PlotLine("Боты", chartData_bots, chartValuesToPlot,
			1.0f, 0.0f, ImPlotLineFlags_None);

		//Apples
		if (chartShow_apples)
		{
			SetNextLineStyle({ ChartApplesColor }, ChartLineThickness);

			PlotLine("Яблоки", chartData_apples, chartValuesToPlot,
				1.0f, 0.0f, ImPlotLineFlags_None);
		}

		//Organics
		if (chartShow_organics)
		{
			SetNextLineStyle({ ChartOrganicsColor }, ChartLineThickness);

			PlotLine("Органика", chartData_organics, chartValuesToPlot,
				1.0f, 0.0f, ImPlotLineFlags_None);
		}

		SetAxis(ImAxis_Y2);

		//Predators
		if (chartShow_predators)
		{
			SetNextLineStyle({ ChartPredatorsColor }, ChartLineThickness);

			PlotLine("Хищники", chartData_predators, chartValuesToPlot,
				1.0f, 0.0f, ImPlotLineFlags_None);
		}

		//AVG lifetime
		if (chartShow_avg_lifetime)
		{
			SetNextLineStyle({ ChartAVGLifetimeColor }, ChartLineThickness);

			PlotLine("Средний возраст", chartData_avg_lifetime, chartValuesToPlot,
				1.0f, 0.0f, ImPlotLineFlags_None);
		}

		EndPlot();
	}
}


}
