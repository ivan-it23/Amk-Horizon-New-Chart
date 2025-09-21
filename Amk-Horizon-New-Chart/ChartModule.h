#pragma once

#include "framework.h"
#include "DataModule.h"
#include <vector>

// Класс для отрисовки графиков
class ChartModule {
private:
    // Цвета для разных каналов
    static const COLORREF CHANNEL_COLORS[4];

public:
    ChartModule();
    ~ChartModule();

    // Основные методы отрисовки
    void DrawChart(HDC hdc, const RECT& rect, const std::vector<std::vector<float>>& chartData, 
                   DataType type, const std::wstring& title, bool hasData) const;

    // Вспомогательные методы отрисовки
    void DrawChartBackground(HDC hdc, const RECT& rect) const;
    void DrawChartBorder(HDC hdc, const RECT& rect) const;
    void DrawChartTitle(HDC hdc, const RECT& rect, const std::wstring& title) const;
    void DrawNoDataMessage(HDC hdc, const RECT& rect, const std::wstring& message) const;
    void DrawChartAxes(HDC hdc, const RECT& plotRect) const;
    void DrawChartData(HDC hdc, const RECT& plotRect, const std::vector<std::vector<float>>& chartData) const;
    void DrawChartLegend(HDC hdc, const RECT& rect, const std::vector<std::vector<float>>& chartData) const;

private:
    // Вспомогательные методы для расчетов
    RECT CalculatePlotRect(const RECT& chartRect) const;
    void FindMinMaxValues(const std::vector<std::vector<float>>& chartData, float& minVal, float& maxVal) const;
};