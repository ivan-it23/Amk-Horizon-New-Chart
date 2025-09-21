#include "framework.h"
#include "ChartModule.h"
#include <algorithm>
#include <cfloat>

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
    
    // Отрисовка осей
    DrawChartAxes(hdc, plotRect);
    
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