#pragma once

#include "framework.h"
#include "DataModule.h"
#include <vector>

// Предварительное объявление
struct CurveDefinition;

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
    
    // v2.10.0: Перегрузка с поддержкой температуры на оси X
    void DrawChart(HDC hdc, const RECT& rect, const std::vector<std::vector<float>>& chartData,
                   DataType type, const std::wstring& title, bool hasData,
                   const std::vector<bool>& enabledTransmitters, const DataModule* dataModule,
                   double minXData, double maxXData, double minYData, double maxYData,
                   const std::vector<float>& xAxisData, bool showAxes = true) const;

    // Новый метод для отрисовки прямоугольника зума
    void DrawZoomRectangle(HDC hdc, const RECT& zoomRect) const;
    
    // Методы для поддержки двойных осей
    void DrawDualAxisChart(HDC hdc, const RECT& rect, 
                          const std::vector<CurveDefinition>& curves,
                          const std::vector<std::vector<float>>& leftAxisData,
                          const std::vector<std::vector<float>>& rightAxisData,
                          const std::wstring& title, bool hasData) const;
    
    // Перегрузка с поддержкой zoom
    void DrawDualAxisChart(HDC hdc, const RECT& rect, 
                          const std::vector<CurveDefinition>& curves,
                          const std::vector<std::vector<float>>& leftAxisData,
                          const std::vector<std::vector<float>>& rightAxisData,
                          const std::wstring& title, bool hasData,
                          double minXData, double maxXData, 
                          double minYData, double maxYData, bool showAxes = true) const;
    
    // v2.10.0: Перегрузка с поддержкой температуры на оси X
    void DrawDualAxisChart(HDC hdc, const RECT& rect,
                          const std::vector<CurveDefinition>& curves,
                          const std::vector<std::vector<float>>& leftAxisData,
                          const std::vector<std::vector<float>>& rightAxisData,
                          const std::wstring& title, bool hasData,
                          const std::vector<float>& xAxisData,
                          double minXData, double maxXData,
                          double minYData, double maxYData, bool showAxes = true) const;

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
    
    // Метод для отрисовки только сетки без подписей осей (для состояния "кривые не выбраны")
    void DrawGridOnly(HDC hdc, const RECT& plotRect) const;
    
    // v2.10.0: Перегрузка с поддержкой температуры на оси X
    void DrawGridAndAxes(HDC hdc, const RECT& plotRect, const std::vector<std::vector<float>>& chartData,
                         const DataModule* dataModule, const std::vector<float>& xAxisData,
                         double minXData, double maxXData, double minYData, double maxYData) const;
    void DrawChartData(HDC hdc, const RECT& plotRect, const std::vector<std::vector<float>>& chartData) const;
    
    // Перегрузка с поддержкой zoom для правильного отображения X координат
    void DrawChartData(HDC hdc, const RECT& plotRect, const std::vector<std::vector<float>>& chartData,
                      double minXData, double maxXData) const;
    
    // v2.10.0: Перегрузка с поддержкой температуры на оси X
    void DrawChartData(HDC hdc, const RECT& plotRect, const std::vector<std::vector<float>>& chartData,
                      const std::vector<float>& xAxisData, double minXData, double maxXData) const;
    void DrawChartLegend(HDC hdc, const RECT& rect, const std::vector<std::vector<float>>& chartData) const;
    void DrawChartLegend(HDC hdc, const RECT& rect, const std::vector<std::vector<float>>& chartData,
                         DataType type, const std::vector<bool>& enabledTransmitters) const;
    
    // Новый метод для отрисовки легенды с кастомными кривыми
    void DrawChartLegend(HDC hdc, const RECT& rect, const std::vector<std::vector<float>>& chartData,
                         const std::vector<CurveDefinition>& curves) const;

    // Хит-тест для чекбоксов в легенде
    int HitTestLegendCheckbox(const RECT& chartRect, const POINT& pt, DataType type) const;
    
    // Методы для двойных осей
    void DrawDualAxisBackground(HDC hdc, const RECT& rect) const;
    void DrawDualAxisGrid(HDC hdc, const RECT& plotRect, 
                         const std::vector<std::vector<float>>& leftAxisData,
                         const std::vector<std::vector<float>>& rightAxisData) const;
    
    // Перегрузка с поддержкой оси X
    // v2.12.0: Перегрузка с поддержкой zoom по X и Y
    void DrawDualAxisGrid(HDC hdc, const RECT& plotRect, 
                         const std::vector<std::vector<float>>& leftAxisData,
                         const std::vector<std::vector<float>>& rightAxisData,
                         double minXData, double maxXData,
                         double minYData, double maxYData) const;
    
    // v2.10.0: Перегрузка с поддержкой температуры на оси X
    void DrawDualAxisGrid(HDC hdc, const RECT& plotRect,
                         const std::vector<std::vector<float>>& leftAxisData,
                         const std::vector<std::vector<float>>& rightAxisData,
                         const std::vector<float>& xAxisData,
                         double minXData, double maxXData) const;
    void DrawLeftAxis(HDC hdc, const RECT& plotRect, 
                     const std::vector<std::vector<float>>& leftAxisData,
                     const std::string& units) const;
    void DrawRightAxis(HDC hdc, const RECT& plotRect, 
                      const std::vector<std::vector<float>>& rightAxisData,
                      const std::string& units) const;
    void DrawDualAxisData(HDC hdc, const RECT& plotRect,
                         const std::vector<CurveDefinition>& curves,
                         const std::vector<std::vector<float>>& leftAxisData,
                         const std::vector<std::vector<float>>& rightAxisData) const;
    
    // v2.10.0: Перегрузка с поддержкой температуры на оси X
    void DrawDualAxisData(HDC hdc, const RECT& plotRect,
                         const std::vector<CurveDefinition>& curves,
                         const std::vector<std::vector<float>>& leftAxisData,
                         const std::vector<std::vector<float>>& rightAxisData,
                         const std::vector<float>& xAxisData,
                         double minXData, double maxXData) const;
    
    // v2.12.0: Новая функция для отрисовки с учетом zoom по Y
    void DrawDualAxisDataWithZoom(HDC hdc, const RECT& plotRect,
                                  const std::vector<CurveDefinition>& curves,
                                  const std::vector<std::vector<float>>& leftAxisData,
                                  const std::vector<std::vector<float>>& rightAxisData,
                                  double leftMinY, double leftMaxY,
                                  double rightMinY, double rightMaxY) const;
    
    // v2.12.0: Вспомогательные функции для температурного режима с zoom по Y
    void DrawDualAxisGridWithTemperature(HDC hdc, const RECT& plotRect,
                                        const std::vector<std::vector<float>>& leftAxisData,
                                        const std::vector<std::vector<float>>& rightAxisData,
                                        const std::vector<float>& xAxisData,
                                        double minXData, double maxXData,
                                        double minYData, double maxYData,
                                        const std::string& leftUnits) const;
    
    void DrawLeftAxisWithZoom(HDC hdc, const RECT& plotRect,
                             const std::vector<std::vector<float>>& leftAxisData,
                             const std::string& units,
                             double minYData, double maxYData) const;
    
    void DrawRightAxisWithZoom(HDC hdc, const RECT& plotRect,
                              const std::vector<std::vector<float>>& rightAxisData,
                              const std::string& units,
                              double minYData, double maxYData) const;
    
    void DrawDualAxisDataWithTemperatureAndZoom(HDC hdc, const RECT& plotRect,
                                               const std::vector<CurveDefinition>& curves,
                                               const std::vector<std::vector<float>>& leftAxisData,
                                               const std::vector<std::vector<float>>& rightAxisData,
                                               const std::vector<float>& xAxisData,
                                               double minXData, double maxXData,
                                               double minYData, double maxYData) const;
    
    void DrawDualAxisLegend(HDC hdc, const RECT& rect,
                           const std::vector<CurveDefinition>& curves) const;

private:
    // Вспомогательные методы для расчетов
    RECT CalculatePlotRect(const RECT& chartRect) const;
    RECT CalculateDualAxisPlotRect(const RECT& chartRect) const;
    void FindMinMaxValues(const std::vector<std::vector<float>>& chartData, float& minVal, float& maxVal) const;
    void FindMinMaxValues(const std::vector<std::vector<float>>& chartData, float& minVal, float& maxVal, double minYPlot, double maxYPlot) const;
    
    // Вспомогательные методы для сетки и меток
    double CalculateNiceStep(double range, int targetSteps) const;
    double CalculateNiceMin(double minVal, double step) const;

public:
    // Метод для получения диапазонов данных (для координат курсора)
    void GetDataRanges(const std::vector<std::vector<float>>& leftAxisData,
                      const std::vector<std::vector<float>>& rightAxisData,
                      double& minX, double& maxX,
                      double& minY1, double& maxY1,
                      double& minY2, double& maxY2) const;
};