#include "framework.h"
#include "ChartModule.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstring>

// Определение цветов для каналов
const COLORREF ChartModule::CHANNEL_COLORS[4] = {
    RGB(255, 0, 0),    // Красный - T1
    RGB(0, 255, 0),    // Зеленый - T2
    RGB(0, 0, 255),    // Синий - T3
    RGB(255, 165, 0)   // Оранжевый - T4
};

ChartModule::ChartModule() {
}

ChartModule::~ChartModule() {
}

void ChartModule::DrawChart(HDC hdc, const RECT& rect, const std::vector<std::vector<float>>& chartData,
                           DataType type, const std::wstring& title, bool hasData) const {
    if (rect.right <= rect.left || rect.bottom <= rect.top)
        return;

    // Отрисовка фона и рамки
    DrawChartBackground(hdc, rect);
    DrawChartBorder(hdc, rect);
    
    // Отрисовка заголовка
    DrawChartTitle(hdc, rect, title);
    
    if (!hasData) {
        DrawNoDataMessage(hdc, rect, L"Нет данных. Загрузите файл через меню Файл -> Открыть файл данных...");
        return;
    }
    
    if (chartData.empty()) {
        DrawNoDataMessage(hdc, rect, L"Нет данных для отображения");
        return;
    }
    
    // Расчет области для графика
    RECT plotRect = CalculatePlotRect(rect);
    if (plotRect.right <= plotRect.left || plotRect.bottom <= plotRect.top)
        return;
    
    // Отрисовка сетки и осей с метками
    DrawGridAndAxes(hdc, plotRect, chartData);
    
    // Отрисовка данных
    DrawChartData(hdc, plotRect, chartData);
    
    // Отрисовка легенды для многоканальных данных
    if (chartData.size() > 1) {
        DrawChartLegend(hdc, rect, chartData);
    }
}

void ChartModule::DrawChartBackground(HDC hdc, const RECT& rect) const {
    HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(hdc, &rect, hBrush);
    DeleteObject(hBrush);
}

void ChartModule::DrawChartBorder(HDC hdc, const RECT& rect) const {
    Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);
}

void ChartModule::DrawChartTitle(HDC hdc, const RECT& rect, const std::wstring& title) const {
    TextOut(hdc, rect.left + 10, rect.top + 10, title.c_str(), (int)title.length());
}

void ChartModule::DrawNoDataMessage(HDC hdc, const RECT& rect, const std::wstring& message) const {
    TextOut(hdc, rect.left + 10, rect.top + 40, message.c_str(), (int)message.length());
}

void ChartModule::DrawChartAxes(HDC hdc, const RECT& plotRect) const {
    // X-ось
    MoveToEx(hdc, plotRect.left, plotRect.bottom, NULL);
    LineTo(hdc, plotRect.right, plotRect.bottom);
    
    // Y-ось
    MoveToEx(hdc, plotRect.left, plotRect.top, NULL);
    LineTo(hdc, plotRect.left, plotRect.bottom);
}

void ChartModule::DrawChartData(HDC hdc, const RECT& plotRect, const std::vector<std::vector<float>>& chartData) const {
    // Находим минимальные и максимальные значения
    float minVal, maxVal;
    FindMinMaxValues(chartData, minVal, maxVal);
    
    if (minVal == maxVal) {
        minVal -= 1.0f;
        maxVal += 1.0f;
    }
    
    // Рисуем данные для каждого канала
    for (size_t channelIdx = 0; channelIdx < chartData.size(); channelIdx++) {
        const auto& channel = chartData[channelIdx];
        if (channel.empty())
            continue;
            
        HPEN hPen = CreatePen(PS_SOLID, 1, CHANNEL_COLORS[channelIdx % 4]);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        
        bool firstPoint = true;
        for (size_t i = 0; i < channel.size(); i++) {
            int x = plotRect.left + (int)((double)i / (channel.size() - 1) * (plotRect.right - plotRect.left));
            int y = plotRect.bottom - (int)((channel[i] - minVal) / (maxVal - minVal) * (plotRect.bottom - plotRect.top));
            
            if (firstPoint) {
                MoveToEx(hdc, x, y, NULL);
                firstPoint = false;
            } else {
                LineTo(hdc, x, y);
            }
        }
        
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
    }
}

void ChartModule::DrawChartLegend(HDC hdc, const RECT& rect, const std::vector<std::vector<float>>& chartData) const {
    const wchar_t* channelNames[] = { L"T1", L"T2", L"T3", L"T4" };
    
    for (size_t i = 0; i < chartData.size() && i < 4; i++) {
        HPEN hPen = CreatePen(PS_SOLID, 2, CHANNEL_COLORS[i]);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        
        int legendY = rect.top + 25 + (int)i * 15;
        MoveToEx(hdc, rect.right - 80, legendY, NULL);
        LineTo(hdc, rect.right - 60, legendY);
        
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
        
        TextOut(hdc, rect.right - 55, legendY - 7, channelNames[i], 2);
    }
}

RECT ChartModule::CalculatePlotRect(const RECT& chartRect) const {
    RECT plotRect = chartRect;
    plotRect.left += 50;   // Отступ для Y-оси
    plotRect.top += 40;    // Отступ для заголовка
    plotRect.right -= 20;  // Отступ справа
    plotRect.bottom -= 30; // Отступ для X-оси
    return plotRect;
}

void ChartModule::FindMinMaxValues(const std::vector<std::vector<float>>& chartData, float& minVal, float& maxVal) const {
    minVal = FLT_MAX;
    maxVal = -FLT_MAX;
    
    for (const auto& channel : chartData) {
        for (float val : channel) {
            minVal = (std::min)(minVal, val);
            maxVal = (std::max)(maxVal, val);
        }
    }
}

void ChartModule::DrawZoomRectangle(HDC hdc, const RECT& zoomRect) const {
    // Создаем полупрозрачную кисть для выделения
    HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 255));
    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 255));
    
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    
    // Устанавливаем режим смешивания для полупрозрачности
    int oldROP = SetROP2(hdc, R2_XORPEN);
    
    Rectangle(hdc, zoomRect.left, zoomRect.top, zoomRect.right, zoomRect.bottom);
    
    SetROP2(hdc, oldROP);
    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hBrush);
    DeleteObject(hPen);
}

void ChartModule::DrawGridAndAxes(HDC hdc, const RECT& chartRect, const std::vector<std::vector<float>>& chartData) const {
    if (chartData.empty() || chartData[0].empty())
        return;
    
    // Находим диапазон данных
    float minVal, maxVal;
    FindMinMaxValues(chartData, minVal, maxVal);
    
    if (minVal == maxVal) {
        minVal -= 1.0f;
        maxVal += 1.0f;
    }
    
    // Расчет "красивых" шагов для осей
    double yRange = maxVal - minVal;
    double yStep = CalculateNiceStep(yRange, 8); // Примерно 8 делений по Y
    double yStart = CalculateNiceMin(minVal, yStep);
    
    size_t dataSize = chartData[0].size();
    double xRange = static_cast<double>(dataSize - 1);
    double xStep = CalculateNiceStep(xRange, 10); // Примерно 10 делений по X
    double xStart = 0.0;
    
    // Создаем перо для сетки (светло-серое)
    HPEN hGridPen = CreatePen(PS_SOLID, 1, RGB(220, 220, 220));
    HPEN hAxisPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hGridPen);
    
    // Отрисовка вертикальных линий сетки и меток X
    for (double x = xStart; x <= xRange; x += xStep) {
        int screenX = chartRect.left + (int)(x / xRange * (chartRect.right - chartRect.left));
        
        // Вертикальная линия сетки
        MoveToEx(hdc, screenX, chartRect.top, NULL);
        LineTo(hdc, screenX, chartRect.bottom);
        
        // Метка на оси X
        SelectObject(hdc, hAxisPen);
        wchar_t label[32];
        swprintf_s(label, 32, L"%.0f", x);
        TextOut(hdc, screenX - 10, chartRect.bottom + 5, label, (int)wcslen(label));
        SelectObject(hdc, hGridPen);
    }
    
    // Отрисовка горизонтальных линий сетки и меток Y
    for (double y = yStart; y <= maxVal + yStep * 0.1; y += yStep) {
        int screenY = chartRect.bottom - (int)((y - minVal) / (maxVal - minVal) * (chartRect.bottom - chartRect.top));
        
        if (screenY >= chartRect.top && screenY <= chartRect.bottom) {
            // Горизонтальная линия сетки
            MoveToEx(hdc, chartRect.left, screenY, NULL);
            LineTo(hdc, chartRect.right, screenY);
            
            // Метка на оси Y
            SelectObject(hdc, hAxisPen);
            wchar_t label[32];
            swprintf_s(label, 32, L"%.2f", y);
            TextOut(hdc, chartRect.left - 45, screenY - 7, label, (int)wcslen(label));
            SelectObject(hdc, hGridPen);
        }
    }
    
    // Отрисовка основных осей
    SelectObject(hdc, hAxisPen);
    
    // X-ось
    MoveToEx(hdc, chartRect.left, chartRect.bottom, NULL);
    LineTo(hdc, chartRect.right, chartRect.bottom);
    
    // Y-ось
    MoveToEx(hdc, chartRect.left, chartRect.top, NULL);
    LineTo(hdc, chartRect.left, chartRect.bottom);
    
    SelectObject(hdc, hOldPen);
    DeleteObject(hGridPen);
    DeleteObject(hAxisPen);
}

double ChartModule::CalculateNiceStep(double range, int targetSteps) const {
    if (range <= 0 || targetSteps <= 0)
        return 1.0;
    
    double roughStep = range / targetSteps;
    double magnitude = pow(10.0, floor(log10(roughStep)));
    double normalizedStep = roughStep / magnitude;
    
    // Выбираем "красивый" шаг
    double niceStep;
    if (normalizedStep <= 1.0)
        niceStep = 1.0;
    else if (normalizedStep <= 2.0)
        niceStep = 2.0;
    else if (normalizedStep <= 5.0)
        niceStep = 5.0;
    else
        niceStep = 10.0;
    
    return niceStep * magnitude;
}

double ChartModule::CalculateNiceMin(double minVal, double step) const {
    return floor(minVal / step) * step;
}