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

const wchar_t* ChartModule::CHANNEL_NAMES[4] = {
    L"T1", L"T2", L"T3", L"T4"
};

ChartModule::ChartModule() {
}

ChartModule::~ChartModule() {
}

void ChartModule::DrawChart(HDC hdc, const RECT& rect, const std::vector<std::vector<float>>& chartData,
                           DataType type, const std::wstring& title, bool hasData) const {
    // По умолчанию все передатчики включены
    std::vector<bool> allTransmittersEnabled = {true, true, true, true};
    DrawChart(hdc, rect, chartData, type, title, hasData, allTransmittersEnabled, nullptr); // Передаем nullptr для dataModule
}

void ChartModule::DrawChart(HDC hdc, const RECT& rect, const std::vector<std::vector<float>>& chartData,
                           DataType type, const std::wstring& title, bool hasData,
                           const std::vector<bool>& enabledTransmitters, const DataModule* dataModule) const {
    if (rect.right <= rect.left || rect.bottom <= rect.top)
        return;

    // Отрисовка фона и рамки
    DrawChartBackground(hdc, rect);
    DrawChartBorder(hdc, rect);
    
    // Отрисовка заголовка
    DrawChartTitle(hdc, rect, title);
    
    // Расчет области для графика
    RECT plotRect = CalculatePlotRect(rect);
    if (plotRect.right <= plotRect.left || plotRect.bottom <= plotRect.top)
        return;
    
    // Отрисовка сетки и осей с метками
    DrawGridAndAxes(hdc, plotRect, chartData, dataModule);
    
    // Отрисовка данных
    if (hasData) {
        DrawChartData(hdc, plotRect, chartData);
    }
    
    // Отрисовка легенды для многоканальных данных
    if (chartData.size() > 1) {
        DrawChartLegend(hdc, rect, chartData, type, enabledTransmitters);
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


void ChartModule::DrawChartAxes(HDC hdc, const RECT& plotRect) const {
    // X-ось
    MoveToEx(hdc, plotRect.left, plotRect.bottom, NULL);
    LineTo(hdc, plotRect.right, plotRect.bottom);
    
    // Y-ось
    MoveToEx(hdc, plotRect.left, plotRect.top, NULL);
    LineTo(hdc, plotRect.left, plotRect.bottom);
}

void ChartModule::DrawChartData(HDC hdc, const RECT& plotRect, const std::vector<std::vector<float>>& chartData) const {
    // Находим минимальные и максимальные значения с padding
    float minVal, maxVal;
    FindMinMaxValues(chartData, minVal, maxVal);
    
    // Рисуем данные для каждого канала
    for (size_t channelIdx = 0; channelIdx < chartData.size(); channelIdx++) {
        const auto& channel = chartData[channelIdx];
        if (channel.empty())
            continue;
            
        HPEN hPen = CreatePen(PS_SOLID, 2, CHANNEL_COLORS[channelIdx % 4]);
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

void ChartModule::DrawChartLegend(HDC hdc, const RECT& rect, const std::vector<std::vector<float>>& chartData,
                                  DataType type, const std::vector<bool>& enabledTransmitters) const {
    // Определяем, является ли тип данных многоканальным
    bool isMultiChannel = false;
    if (type == DataType::PHASE_DIFFERENCE || type == DataType::AMPLITUDE_RATIO ||
        type == DataType::AMPLITUDE_RX1 || type == DataType::AMPLITUDE_RX2 ||
        type == DataType::PHASE_RX1 || type == DataType::PHASE_RX2) {
        isMultiChannel = true;
    }

    if (!isMultiChannel) {
        // Для одноканальных данных легенда отображается без чекбоксов
        // Пока не отрисовываем легенду для одноканальных данных вообще
        return;
    }

    int legendX = rect.right - 200; // Отступ от правого края
    int legendY = rect.top + 10;  // Отступ сверху (ниже заголовка)
    int lineHeight = 20;          // Высота строки легенды (теперь для каждого элемента)
    int elementSpacing = 10;      // Промежуток между элементами легенды (чекбокс+текст)
    int colorBoxSize = 10;        // Размер квадрата цвета
    int checkboxSize = 12;        // Размер чекбокса
    int currentX = legendX;       // Текущая X-позиция для отрисовки элементов

    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    SetBkMode(hdc, TRANSPARENT);

    for (size_t channelIdx = 0; channelIdx < 4; channelIdx++) { // Отрисовываем все 4 чекбокса для T1-T4
        // Проверяем, что это многоканальный тип данных и у нас есть соответствующий передатчик
        if (isMultiChannel && channelIdx < enabledTransmitters.size()) {
            COLORREF color = CHANNEL_COLORS[channelIdx % 4];
            HBRUSH hBrush = CreateSolidBrush(color);
            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
            HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);

            // Отрисовка квадрата цвета
            Rectangle(hdc, currentX,
                      (LONG)(legendY + (lineHeight - colorBoxSize) / 2),
                      currentX + colorBoxSize,
                      (LONG)(legendY + (lineHeight + colorBoxSize) / 2));

            SelectObject(hdc, hOldBrush);
            DeleteObject(hBrush);
            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);

            currentX += colorBoxSize + 2; // Отступ после квадрата цвета

            // Отрисовка чекбокса
            RECT checkboxRect = {currentX,
                                 (LONG)(legendY + (lineHeight - checkboxSize) / 2),
                                 currentX + checkboxSize,
                                 (LONG)(legendY + (lineHeight + checkboxSize) / 2)};
            DrawFrameControl(hdc, &checkboxRect, DFC_BUTTON, DFCS_BUTTONCHECK | (enabledTransmitters[channelIdx] ? DFCS_CHECKED : 0));

            currentX += checkboxSize + 2; // Отступ после чекбокса

            // Отрисовка названия канала
            TextOut(hdc, currentX, legendY, CHANNEL_NAMES[channelIdx], (int)wcslen(CHANNEL_NAMES[channelIdx]));

            currentX += (int)wcslen(CHANNEL_NAMES[channelIdx]) * 8 + elementSpacing; // Примерный размер текста + промежуток
        }
    }

    SelectObject(hdc, hOldFont);
}

// Обновленная DrawChartLegend (для совместимости)
void ChartModule::DrawChartLegend(HDC hdc, const RECT& rect, const std::vector<std::vector<float>>& chartData) const {
    // По умолчанию все передатчики включены
    std::vector<bool> allTransmittersEnabled = {true, true, true, true};
    DrawChartLegend(hdc, rect, chartData, DataType::PHASE_DIFFERENCE, allTransmittersEnabled); // Используем PHASE_DIFFERENCE как заглушку для многоканального типа
}


int ChartModule::HitTestLegendCheckbox(const RECT& chartRect, const POINT& pt, DataType type) const {
    // Определяем, является ли тип данных многоканальным
    bool isMultiChannel = false;
    if (type == DataType::PHASE_DIFFERENCE || type == DataType::AMPLITUDE_RATIO ||
        type == DataType::AMPLITUDE_RX1 || type == DataType::AMPLITUDE_RX2 ||
        type == DataType::PHASE_RX1 || type == DataType::PHASE_RX2) {
        isMultiChannel = true;
    }

    if (!isMultiChannel) {
        return -1; // Для одноканальных данных нет чекбоксов
    }

    int legendX = chartRect.right - 200; // Отступ от правого края
    int legendY = chartRect.top + 10;  // Отступ сверху (ниже заголовка)
    int lineHeight = 20;          // Высота строки легенды
    int elementSpacing = 10;      // Промежуток между элементами легенды (чекбокс+текст)
    int colorBoxSize = 10;        // Размер квадрата цвета
    int checkboxSize = 12;        // Размер чекбокса
    int currentX = legendX;

    for (int channelIdx = 0; channelIdx < 4; ++channelIdx) { // Только для T1-T4
        if (isMultiChannel) {
            currentX += colorBoxSize + 2; // Пропускаем квадрат цвета

            RECT checkboxRect = {currentX,
                                 (LONG)(legendY + (lineHeight - checkboxSize) / 2),
                                 currentX + checkboxSize,
                                 (LONG)(legendY + (lineHeight + checkboxSize) / 2)};

            if (PtInRect(&checkboxRect, pt)) {
                return channelIdx; // Возвращаем индекс передатчика (0-3 для T1-T4)
            }

            currentX += checkboxSize + 2; // Пропускаем чекбокс
            currentX += (int)wcslen(CHANNEL_NAMES[channelIdx]) * 8 + elementSpacing; // Пропускаем текст + промежуток
        }
    }

    return -1; // Не попали ни в один чекбокс
}

void ChartModule::DrawGridAndAxes(HDC hdc, const RECT& chartRect, const std::vector<std::vector<float>>& chartData) const {
    // Вызываем перегруженную версию с nullptr для dataModule, если он не передан.
    // Это означает, что GetDataSize() не будет использоваться в этой перегрузке.
    DrawGridAndAxes(hdc, chartRect, chartData, nullptr);
}

void ChartModule::DrawGridAndAxes(HDC hdc, const RECT& chartRect, const std::vector<std::vector<float>>& chartData, const DataModule* dataModule) const {
    // Отрисовка линий осей (всегда, независимо от наличия данных)
    DrawChartAxes(hdc, chartRect);

    float minVal, maxVal;
    FindMinMaxValues(chartData, minVal, maxVal);

    // Если данных нет, или minVal == maxVal (все значения одинаковы), устанавливаем разумный диапазон по умолчанию
    if (minVal == FLT_MAX || maxVal == -FLT_MAX || minVal == maxVal) {
        minVal = 0.0f;
        maxVal = 100.0f; // Диапазон по умолчанию
    }
    
    // Отрисовка горизонтальных линий сетки и меток Y-оси
    double niceStepY = CalculateNiceStep(maxVal - minVal, (chartRect.bottom - chartRect.top) / 40); // Примерное количество шагов
    double currentYVal = CalculateNiceMin(minVal, niceStepY);

    while (currentYVal <= maxVal) {
        if (currentYVal >= minVal) { // Только если значение находится внутри видимого диапазона
            long screenY = chartRect.bottom - (LONG)((currentYVal - minVal) / (maxVal - minVal) * (chartRect.bottom - chartRect.top));
            HPEN hPen = CreatePen(PS_DOT, 1, RGB(200, 200, 200));
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
            
            MoveToEx(hdc, chartRect.left, screenY, NULL);
            LineTo(hdc, chartRect.right, screenY);
            
            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);

            // Отрисовка метки Y-оси
            std::wstring yLabel = std::to_wstring(static_cast<int>(currentYVal)); // Простой вывод как int
            TextOut(hdc, chartRect.left - 40, screenY - 7, yLabel.c_str(), (int)yLabel.length());
        }
        currentYVal += niceStepY;
    }

    // Отрисовка вертикальных линий сетки и меток X-оси (пока упрощенно)
    // Предполагаем, что X-ось - это индекс данных
    if (dataModule && dataModule->HasData()) {
        int dataPoints = (int)dataModule->GetDataSize(); // Используем размер данных из DataModule
        if (dataPoints > 1) {
            double niceStepX = CalculateNiceStep(static_cast<double>(dataPoints - 1), (chartRect.right - chartRect.left) / 60);
            double currentXVal = CalculateNiceMin(0, niceStepX);

            while (currentXVal < dataPoints) {
                if (currentXVal >= 0) {
                    long screenX = chartRect.left + (LONG)(currentXVal / (dataPoints - 1.0) * (chartRect.right - chartRect.left));
                    HPEN hPen = CreatePen(PS_DOT, 1, RGB(200, 200, 200));
                    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
                    
                    MoveToEx(hdc, screenX, chartRect.top, NULL);
                    LineTo(hdc, screenX, chartRect.bottom);
                    
                    SelectObject(hdc, hOldPen);
                    DeleteObject(hPen);

                    // Отрисовка метки X-оси
                    std::wstring xLabel = std::to_wstring(static_cast<int>(currentXVal));
                    TextOut(hdc, screenX - 10, chartRect.bottom + 5, xLabel.c_str(), (int)xLabel.length());
                }
                currentXVal += niceStepX;
            }
        }
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
    
    // Находим исходные минимум и максимум
    for (const auto& channel : chartData) {
        for (float val : channel) {
            minVal = (std::min)(minVal, val);
            maxVal = (std::max)(maxVal, val);
        }
    }
    
    // Если данных нет, устанавливаем значения по умолчанию
    if (minVal == FLT_MAX || maxVal == -FLT_MAX) {
        minVal = 0.0f;
        maxVal = 1.0f;
        return;
    }
    
    // Рассчитываем диапазон данных
    float range = maxVal - minVal;
    
    // Обрабатываем случай, когда все значения одинаковы или диапазон очень мал
    if (range < 1e-6f) {
        // Создаем искусственный диапазон вокруг значения
        float center = (minVal + maxVal) * 0.5f;
        float artificialRange = (std::max)(2.0f, (std::abs)(center) * 0.1f);
        minVal = center - artificialRange * 0.5f;
        maxVal = center + artificialRange * 0.5f;
    } else {
        // Добавляем 5% padding к диапазону
        float padding = range * 0.05f;
        minVal -= padding;
        maxVal += padding;
    }
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

void ChartModule::DrawZoomRectangle(HDC hdc, const RECT& zoomRect) const {
    // Используем XOR режим для отрисовки прямоугольника выделения
    // Это позволяет рисовать и стирать прямоугольник без повреждения фона
    int oldROP = SetROP2(hdc, R2_XORPEN);
    
    // Создаем перо для прямоугольника выделения
    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 255));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    
    // Устанавливаем прозрачную кисть, чтобы не заливать внутренность
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    
    // Рисуем прямоугольник
    Rectangle(hdc, zoomRect.left, zoomRect.top, zoomRect.right, zoomRect.bottom);
    
    // Восстанавливаем исходные настройки
    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    SetROP2(hdc, oldROP);
    DeleteObject(hPen);
}