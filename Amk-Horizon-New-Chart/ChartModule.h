#pragma once

#include "framework.h"
#include "DataModule.h"
#include <vector>

// Класс для отрисовки графиков
class ChartModule {
private:
    // Цвета для разных каналов
    static const COLORREF CHANNEL_COLORS[4];
    static const wchar_t* CHANNEL_NAMES[4]; // Добавляем статическое объявление для имен каналов

public:
    ChartModule();
    ~ChartModule();

    // Основные методы отрисовки
    void DrawChart(HDC hdc, const RECT& rect, const std::vector<std::vector<float>>& chartData, 
                   DataType type, const std::wstring& title, bool hasData, bool showAxes = true) const;
    void DrawChart(HDC hdc, const RECT& rect, const std::vector<std::vector<float>>& chartData,
                   DataType type, const std::wstring& title, bool hasData,
                   const std::vector<bool>& enabledTransmitters, const DataModule* dataModule,
                   double minXData, double maxXData, double minYData, double maxYData, bool showAxes = true) const;

    // Новый метод для отрисовки прямоугольника зума
    void DrawZoomRectangle(HDC hdc, const RECT& zoomRect) const;

    // Вспомогательные методы отрисовки
    void DrawChartBackground(HDC hdc, const RECT& rect) const;
    void DrawChartBorder(HDC hdc, const RECT& rect) const;
    void DrawChartTitle(HDC hdc, const RECT& rect, const std::wstring& title) const;
    void DrawNoDataMessage(HDC hdc, const RECT& rect, const std::wstring& message) const;
    void DrawChartAxes(HDC hdc, const RECT& plotRect, bool showAxes = true) const;
    void DrawGridAndAxes(HDC hdc, const RECT& plotRect, const std::vector<std::vector<float>>& chartData) const;
    void DrawGridAndAxes(HDC hdc, const RECT& plotRect, const std::vector<std::vector<float>>& chartData, const DataModule* dataModule) const;
    void DrawGridAndAxes(HDC hdc, const RECT& plotRect, const std::vector<std::vector<float>>& chartData, 
                         const DataModule* dataModule, double minXData, double maxXData, double minYData, double maxYData) const;
    void DrawChartData(HDC hdc, const RECT& plotRect, const std::vector<std::vector<float>>& chartData) const;
    void DrawChartLegend(HDC hdc, const RECT& rect, const std::vector<std::vector<float>>& chartData) const;
    void DrawChartLegend(HDC hdc, const RECT& rect, const std::vector<std::vector<float>>& chartData,
                         DataType type, const std::vector<bool>& enabledTransmitters) const;

    // Хит-тест для чекбоксов в легенде
    int HitTestLegendCheckbox(const RECT& chartRect, const POINT& pt, DataType type) const;

private:
    // Вспомогательные методы для расчетов
    RECT CalculatePlotRect(const RECT& chartRect) const;
    void FindMinMaxValues(const std::vector<std::vector<float>>& chartData, float& minVal, float& maxVal) const;
    void FindMinMaxValues(const std::vector<std::vector<float>>& chartData, float& minVal, float& maxVal, double minYPlot, double maxYPlot) const;
    
    // Вспомогательные методы для сетки и меток
    double CalculateNiceStep(double range, int targetSteps) const;
    double CalculateNiceMin(double minVal, double step) const;
};