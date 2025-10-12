#include "framework.h"
#include "ChartModule.h"
#include "SchematicModule.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <sstream> // Для std::wstringstream
#include <iomanip> // Для std::fixed и std::setprecision

// Определение цветов для каналов
const COLORREF ChartModule::CHANNEL_COLORS[4] = {
    RGB(0, 255, 0),    // Зеленый - T1
    RGB(255, 255, 0),  // Желтый - T2
    RGB(255, 0, 0),    // Красный - T3
    RGB(0, 0, 255)     // Синий - T4
};

const wchar_t* ChartModule::CHANNEL_NAMES[4] = {
    L"T1", L"T2", L"T3", L"T4"
};

ChartModule::ChartModule() {
}

ChartModule::~ChartModule() {
}

void ChartModule::DrawChart(HDC hdc, const RECT& rect, const std::vector<std::vector<float>>& chartData,
                           DataType type, const std::wstring& title, bool hasData, bool showAxes) const {
    // По умолчанию все передатчики включены
    std::vector<bool> allTransmittersEnabled = {true, true, true, true};
    DrawChart(hdc, rect, chartData, type, title, hasData, allTransmittersEnabled, nullptr, 0.0, 0.0, 0.0, 0.0, showAxes);
}

void ChartModule::DrawChart(HDC hdc, const RECT& rect, const std::vector<std::vector<float>>& chartData,
                           DataType type, const std::wstring& title, bool hasData,
                           const std::vector<bool>& enabledTransmitters, const DataModule* dataModule,
                           double minXData, double maxXData, double minYData, double maxYData, bool showAxes) const {
    if (rect.right <= rect.left || rect.bottom <= rect.top)
        return;

    // Отрисовка фона и рамки
    DrawChartBackground(hdc, rect);
    DrawChartBorder(hdc, rect);
    
    // Отрисовка заголовка
    DrawChartTitle(hdc, rect, title);
    
    // Расчет области для графика
    RECT plotRect = CalculatePlotRect(rect);

    // Отрисовка осей (только если не заменены скроллбарами)
    DrawChartAxes(hdc, plotRect, showAxes);

    if (plotRect.right <= plotRect.left || plotRect.bottom <= plotRect.top)
        return;
    
    // Отрисовка сетки и осей с метками
    DrawGridAndAxes(hdc, plotRect, chartData, dataModule, 
                    minXData, maxXData, minYData, maxYData); // Передаем параметры зума
    
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


void ChartModule::DrawChartAxes(HDC hdc, const RECT& plotRect, bool showAxes) const {
    if (!showAxes) return; // Не рисуем оси, если они заменены скроллбарами
    
    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0)); // Сплошная линия для осей
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

    // X-ось
    MoveToEx(hdc, plotRect.left, plotRect.bottom, NULL);
    LineTo(hdc, plotRect.right, plotRect.bottom);
    
    // Y-ось
    MoveToEx(hdc, plotRect.left, plotRect.top, NULL);
    LineTo(hdc, plotRect.left, plotRect.bottom);

    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
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

void ChartModule::DrawGridAndAxes(HDC hdc, const RECT& plotRect, const std::vector<std::vector<float>>& chartData) const {
    // Вызываем перегруженную версию с nullptr для dataModule, если он не передан.
    // Это означает, что GetDataSize() не будет использоваться в этой перегрузке.
    DrawGridAndAxes(hdc, plotRect, chartData, nullptr, 0.0, 0.0, 0.0, 0.0); // Передаем 0.0 для min/maxX/YData
}

void ChartModule::DrawGridAndAxes(HDC hdc, const RECT& plotRect, const std::vector<std::vector<float>>& chartData, const DataModule* dataModule) const {
    // Вызываем новую перегрузку с данными о зуме по умолчанию
    double minXData = 0.0;
    double maxXData = 0.0;
    if (dataModule && dataModule->HasData()) {
        maxXData = static_cast<double>(dataModule->GetDataSize() - 1);
    }
    DrawGridAndAxes(hdc, plotRect, chartData, dataModule, minXData, maxXData, 0.0, 0.0);
}

void ChartModule::DrawGridAndAxes(HDC hdc, const RECT& plotRect, const std::vector<std::vector<float>>& chartData, 
                                const DataModule* dataModule, double minXData, double maxXData, double minYData, double maxYData) const {
    // Если данных нет, то не рисуем сетку и метки
    if (!dataModule || !dataModule->HasData()) {
        return;
    }

    float minVal, maxVal;
    FindMinMaxValues(chartData, minVal, maxVal, minYData, maxYData); // Передаем minYData и maxYData

    // Если minVal == maxVal (все значения одинаковы), устанавливаем разумный диапазон по умолчанию
    if (minVal == FLT_MAX || maxVal == -FLT_MAX || minVal == maxVal) {
        minVal = static_cast<float>(minYData);
        maxVal = static_cast<float>(maxYData);
        if (minYData == 0.0 && maxYData == 0.0) { // Если не было зума, устанавливаем диапазон по умолчанию
            minVal = 0.0f;
            maxVal = 100.0f;
        }
    }
    
    // Отрисовка горизонтальных линий сетки и меток Y-оси
    double niceStepY = CalculateNiceStep(maxVal - minVal, (plotRect.bottom - plotRect.top) / 40); // Примерное количество шагов
    double currentYVal = CalculateNiceMin(minVal, niceStepY);

    while (currentYVal <= maxVal + niceStepY / 2) { // Добавляем niceStepY / 2 для учета ошибок округления
        if (currentYVal >= minVal - niceStepY / 2 && currentYVal <= maxVal + niceStepY / 2) { // Проверка на видимый диапазон с запасом
            long screenY = plotRect.bottom - (LONG)((currentYVal - minVal) / (maxVal - minVal) * (plotRect.bottom - plotRect.top));
            
            // Проверяем, что линия сетки находится в пределах области графика
            if (screenY >= plotRect.top && screenY <= plotRect.bottom) {
                HPEN hPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
                HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
                
                // Рисуем горизонтальную линию сетки только в пределах plotRect
                MoveToEx(hdc, plotRect.left, screenY, NULL);
                LineTo(hdc, plotRect.right, screenY);
                
                SelectObject(hdc, hOldPen);
                DeleteObject(hPen);

                // Отрисовка метки Y-оси (в области отступа слева от plotRect)
                std::wstringstream ssY;
                ssY << std::fixed << std::setprecision(1) << currentYVal;
                std::wstring yLabel = ssY.str();
                TextOut(hdc, plotRect.left - 40, screenY - 7, yLabel.c_str(), (int)yLabel.length());
            }
        }
        currentYVal += niceStepY;
    }

    // Отрисовка вертикальных линий сетки и меток X-оси
    if (dataModule && dataModule->HasData()) {
        double currentMinX = minXData; // Используем переданный диапазон
        double currentMaxX = maxXData;

        if (currentMaxX == 0.0 && dataModule->GetDataSize() > 0) { // Если не был передан зум, используем полный диапазон
            currentMaxX = static_cast<double>(dataModule->GetDataSize() - 1);
        }

        if (currentMaxX <= currentMinX) currentMaxX = currentMinX + 1; // Избегаем деления на ноль

        double xRange = currentMaxX - currentMinX;
        double niceStepX = CalculateNiceStep(xRange, (plotRect.right - plotRect.left) / 60);
        double currentXVal = CalculateNiceMin(currentMinX, niceStepX);

        while (currentXVal <= currentMaxX + niceStepX / 2) { // Добавляем niceStepX / 2 для учета ошибок округления
            if (currentXVal >= currentMinX - niceStepX / 2 && currentXVal <= currentMaxX + niceStepX / 2) { // Проверка на видимый диапазон с запасом
                long screenX = plotRect.left + (LONG)((currentXVal - currentMinX) / xRange * (plotRect.right - plotRect.left));
                
                // Проверяем, что линия сетки находится в пределах области графика
                if (screenX >= plotRect.left && screenX <= plotRect.right) {
                    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
                    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
                    
                    // Рисуем вертикальную линию сетки только в пределах plotRect
                    MoveToEx(hdc, screenX, plotRect.top, NULL);
                    LineTo(hdc, screenX, plotRect.bottom);
                    
                    SelectObject(hdc, hOldPen);
                    DeleteObject(hPen);

                    // Отрисовка метки X-оси (в области отступа снизу от plotRect)
                    std::wstringstream ssX;
                    ssX << std::fixed << std::setprecision(1) << currentXVal;
                    std::wstring xLabel = ssX.str();
                    TextOut(hdc, screenX - 10, plotRect.bottom + 5, xLabel.c_str(), (int)xLabel.length());
                }
            }
            currentXVal += niceStepX;
        }
    }
}


RECT ChartModule::CalculatePlotRect(const RECT& chartRect) const {
    RECT plotRect = chartRect;
    plotRect.left += 50;   // Отступ для Y-оси
    plotRect.top += 40;    // Отступ для заголовка
    plotRect.right -= 20;  // Отступ справа
    plotRect.bottom -= 50; // Увеличенный отступ для X-оси и скроллбара
    return plotRect;
}

void ChartModule::FindMinMaxValues(const std::vector<std::vector<float>>& chartData, float& minVal, float& maxVal) const {
    FindMinMaxValues(chartData, minVal, maxVal, 0.0, 100.0); // По умолчанию используем 0-100 для Y
}

void ChartModule::FindMinMaxValues(const std::vector<std::vector<float>>& chartData, float& minVal, float& maxVal, double minYPlot, double maxYPlot) const {
    minVal = FLT_MAX;
    maxVal = -FLT_MAX;
    
    // Находим исходные минимум и максимум
    for (const auto& channel : chartData) {
        for (float val : channel) {
            minVal = (std::min)(minVal, val);
            maxVal = (std::max)(maxVal, val);
        }
    }
    
    // Если данных нет, устанавливаем значения по умолчанию из minYPlot и maxYPlot
    if (minVal == FLT_MAX || maxVal == -FLT_MAX) {
        minVal = static_cast<float>(minYPlot);
        maxVal = static_cast<float>(maxYPlot);
        // Если minYPlot и maxYPlot тоже 0, то устанавливаем глобальный дефолт
        if (minYPlot == 0.0 && maxYPlot == 0.0) {
            minVal = 0.0f;
            maxVal = 100.0f;
        }
        return;
    }
    
    // Если minVal == maxVal (все значения одинаковы), устанавливаем разумный диапазон по умолчанию
    if (minVal == maxVal) {
        minVal = static_cast<float>(minYPlot); 
        maxVal = static_cast<float>(maxYPlot); 
        if (minYPlot == 0.0 && maxYPlot == 0.0) {
            minVal = 0.0f;
            maxVal = 100.0f;
        }
        return;
    }
    
    // Рассчитываем диапазон данных
    float range = maxVal - minVal;
    // Добавляем небольшой padding для лучшей визуализации, если диапазон достаточно большой
    if (range > 0.001f) { // Избегаем деления на ноль для очень маленьких диапазонов
        float padding = range * 0.05f; // 5% от диапазона
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

// Методы для поддержки двойных осей ординат
void ChartModule::DrawDualAxisChart(HDC hdc, const RECT& rect, 
                                   const std::vector<CurveDefinition>& curves,
                                   const std::vector<std::vector<float>>& leftAxisData,
                                   const std::vector<std::vector<float>>& rightAxisData,
                                   const std::wstring& title, bool hasData) const {
    if (rect.right <= rect.left || rect.bottom <= rect.top)
        return;

    // Отрисовка фона и рамки
    DrawDualAxisBackground(hdc, rect);
    DrawChartBorder(hdc, rect);
    
    // Отрисовка заголовка
    DrawChartTitle(hdc, rect, title);
    
    // Расчет области для графика
    RECT plotRect = CalculatePlotRect(rect);
    
    if (plotRect.right <= plotRect.left || plotRect.bottom <= plotRect.top)
        return;
    
    // Отрисовка сетки и осей
    DrawDualAxisGrid(hdc, plotRect, leftAxisData, rightAxisData);
    
    // Отрисовка левой и правой осей с подписями
    std::string leftUnits, rightUnits;
    for (const auto& curve : curves) {
        if (!curve.enabled) continue;
        
        if (curve.yAxisGroup == 0 && leftUnits.empty()) {
            leftUnits = curve.units;
        } else if (curve.yAxisGroup == 1 && rightUnits.empty()) {
            rightUnits = curve.units;
        }
    }
    
    if (!leftAxisData.empty()) {
        DrawLeftAxis(hdc, plotRect, leftAxisData, leftUnits);
    }
    if (!rightAxisData.empty()) {
        DrawRightAxis(hdc, plotRect, rightAxisData, rightUnits);
    }
    
    // Отрисовка данных
    if (hasData) {
        DrawDualAxisData(hdc, plotRect, curves, leftAxisData, rightAxisData);
    }
    
    // Отрисовка легенды
    if (!curves.empty()) {
        DrawDualAxisLegend(hdc, rect, curves);
    }
}

void ChartModule::DrawDualAxisBackground(HDC hdc, const RECT& rect) const {
    // Используем тот же фон, что и для обычных графиков
    DrawChartBackground(hdc, rect);
}

void ChartModule::DrawDualAxisGrid(HDC hdc, const RECT& plotRect, 
                                  const std::vector<std::vector<float>>& leftAxisData,
                                  const std::vector<std::vector<float>>& rightAxisData) const {
    // Находим диапазоны для обеих осей
    float leftMin = FLT_MAX, leftMax = -FLT_MAX;
    float rightMin = FLT_MAX, rightMax = -FLT_MAX;
    
    // Диапазон левой оси
    for (const auto& channel : leftAxisData) {
        for (float val : channel) {
            leftMin = (std::min)(leftMin, val);
            leftMax = (std::max)(leftMax, val);
        }
    }
    
    // Диапазон правой оси
    for (const auto& channel : rightAxisData) {
        for (float val : channel) {
            rightMin = (std::min)(rightMin, val);
            rightMax = (std::max)(rightMax, val);
        }
    }
    
    // Добавляем padding
    if (leftMin != FLT_MAX && leftMax != -FLT_MAX && leftMin != leftMax) {
        float leftRange = leftMax - leftMin;
        float leftPadding = leftRange * 0.05f;
        leftMin -= leftPadding;
        leftMax += leftPadding;
    }
    
    if (rightMin != FLT_MAX && rightMax != -FLT_MAX && rightMin != rightMax) {
        float rightRange = rightMax - rightMin;
        float rightPadding = rightRange * 0.05f;
        rightMin -= rightPadding;
        rightMax += rightPadding;
    }
    
    // Отрисовка горизонтальных линий сетки (основываемся на левой оси)
    if (leftMin != FLT_MAX && leftMax != -FLT_MAX) {
        double niceStepY = CalculateNiceStep(leftMax - leftMin, (plotRect.bottom - plotRect.top) / 40);
        double currentYVal = CalculateNiceMin(leftMin, niceStepY);

        while (currentYVal <= leftMax + niceStepY / 2) {
            if (currentYVal >= leftMin - niceStepY / 2 && currentYVal <= leftMax + niceStepY / 2) {
                long screenY = plotRect.bottom - (LONG)((currentYVal - leftMin) / (leftMax - leftMin) * (plotRect.bottom - plotRect.top));
                
                if (screenY >= plotRect.top && screenY <= plotRect.bottom) {
                    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(230, 230, 230));
                    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
                    
                    MoveToEx(hdc, plotRect.left, screenY, NULL);
                    LineTo(hdc, plotRect.right, screenY);
                    
                    SelectObject(hdc, hOldPen);
                    DeleteObject(hPen);
                }
            }
            currentYVal += niceStepY;
        }
    }
}

void ChartModule::DrawLeftAxis(HDC hdc, const RECT& plotRect, 
                              const std::vector<std::vector<float>>& leftAxisData,
                              const std::string& units) const {
    if (leftAxisData.empty()) return;
    
    // Находим диапазон левой оси
    float minVal = FLT_MAX, maxVal = -FLT_MAX;
    for (const auto& channel : leftAxisData) {
        for (float val : channel) {
            minVal = (std::min)(minVal, val);
            maxVal = (std::max)(maxVal, val);
        }
    }
    
    if (minVal == FLT_MAX || maxVal == -FLT_MAX || minVal == maxVal) {
        minVal = 0.0f;
        maxVal = 100.0f;
    } else {
        float range = maxVal - minVal;
        float padding = range * 0.05f;
        minVal -= padding;
        maxVal += padding;
    }
    
    // Рисуем левую ось
    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    
    MoveToEx(hdc, plotRect.left, plotRect.top, NULL);
    LineTo(hdc, plotRect.left, plotRect.bottom);
    
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
    
    // Рисуем метки левой оси
    double niceStepY = CalculateNiceStep(maxVal - minVal, (plotRect.bottom - plotRect.top) / 40);
    double currentYVal = CalculateNiceMin(minVal, niceStepY);
    
    SetTextColor(hdc, RGB(0, 0, 0));
    SetBkMode(hdc, TRANSPARENT);
    
    while (currentYVal <= maxVal + niceStepY / 2) {
        if (currentYVal >= minVal - niceStepY / 2 && currentYVal <= maxVal + niceStepY / 2) {
            long screenY = plotRect.bottom - (LONG)((currentYVal - minVal) / (maxVal - minVal) * (plotRect.bottom - plotRect.top));
            
            if (screenY >= plotRect.top && screenY <= plotRect.bottom) {
                std::wstringstream ss;
                ss << std::fixed << std::setprecision(1) << currentYVal;
                std::wstring label = ss.str();
                TextOut(hdc, plotRect.left - 45, screenY - 7, label.c_str(), (int)label.length());
            }
        }
        currentYVal += niceStepY;
    }
    
    // Подпись единиц измерения левой оси
    if (!units.empty()) {
        std::wstring wunits(units.begin(), units.end());
        TextOut(hdc, plotRect.left - 45, plotRect.top - 20, wunits.c_str(), (int)wunits.length());
    }
}

void ChartModule::DrawRightAxis(HDC hdc, const RECT& plotRect, 
                               const std::vector<std::vector<float>>& rightAxisData,
                               const std::string& units) const {
    if (rightAxisData.empty()) return;
    
    // Находим диапазон правой оси
    float minVal = FLT_MAX, maxVal = -FLT_MAX;
    for (const auto& channel : rightAxisData) {
        for (float val : channel) {
            minVal = (std::min)(minVal, val);
            maxVal = (std::max)(maxVal, val);
        }
    }
    
    if (minVal == FLT_MAX || maxVal == -FLT_MAX || minVal == maxVal) {
        minVal = 0.0f;
        maxVal = 100.0f;
    } else {
        float range = maxVal - minVal;
        float padding = range * 0.05f;
        minVal -= padding;
        maxVal += padding;
    }
    
    // Рисуем правую ось
    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    
    MoveToEx(hdc, plotRect.right, plotRect.top, NULL);
    LineTo(hdc, plotRect.right, plotRect.bottom);
    
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
    
    // Рисуем метки правой оси
    double niceStepY = CalculateNiceStep(maxVal - minVal, (plotRect.bottom - plotRect.top) / 40);
    double currentYVal = CalculateNiceMin(minVal, niceStepY);
    
    SetTextColor(hdc, RGB(0, 0, 0));
    SetBkMode(hdc, TRANSPARENT);
    
    while (currentYVal <= maxVal + niceStepY / 2) {
        if (currentYVal >= minVal - niceStepY / 2 && currentYVal <= maxVal + niceStepY / 2) {
            long screenY = plotRect.bottom - (LONG)((currentYVal - minVal) / (maxVal - minVal) * (plotRect.bottom - plotRect.top));
            
            if (screenY >= plotRect.top && screenY <= plotRect.bottom) {
                std::wstringstream ss;
                ss << std::fixed << std::setprecision(1) << currentYVal;
                std::wstring label = ss.str();
                TextOut(hdc, plotRect.right + 5, screenY - 7, label.c_str(), (int)label.length());
            }
        }
        currentYVal += niceStepY;
    }
    
    // Подпись единиц измерения правой оси
    if (!units.empty()) {
        std::wstring wunits(units.begin(), units.end());
        TextOut(hdc, plotRect.right + 5, plotRect.top - 20, wunits.c_str(), (int)wunits.length());
    }
}

void ChartModule::DrawDualAxisData(HDC hdc, const RECT& plotRect,
                                  const std::vector<CurveDefinition>& curves,
                                  const std::vector<std::vector<float>>& leftAxisData,
                                  const std::vector<std::vector<float>>& rightAxisData) const {
    // Находим диапазоны для обеих осей
    float leftMin = FLT_MAX, leftMax = -FLT_MAX;
    float rightMin = FLT_MAX, rightMax = -FLT_MAX;
    
    // Диапазон левой оси
    for (const auto& channel : leftAxisData) {
        for (float val : channel) {
            leftMin = (std::min)(leftMin, val);
            leftMax = (std::max)(leftMax, val);
        }
    }
    
    // Диапазон правой оси
    for (const auto& channel : rightAxisData) {
        for (float val : channel) {
            rightMin = (std::min)(rightMin, val);
            rightMax = (std::max)(rightMax, val);
        }
    }
    
    // Добавляем padding и проверяем корректность диапазонов
    if (leftMin != FLT_MAX && leftMax != -FLT_MAX && leftMin != leftMax) {
        float leftRange = leftMax - leftMin;
        float leftPadding = leftRange * 0.05f;
        leftMin -= leftPadding;
        leftMax += leftPadding;
    } else {
        leftMin = 0.0f;
        leftMax = 100.0f;
    }
    
    if (rightMin != FLT_MAX && rightMax != -FLT_MAX && rightMin != rightMax) {
        float rightRange = rightMax - rightMin;
        float rightPadding = rightRange * 0.05f;
        rightMin -= rightPadding;
        rightMax += rightPadding;
    } else {
        rightMin = 0.0f;
        rightMax = 100.0f;
    }
    
    // Отрисовка кривых левой оси (сплошные линии)
    size_t leftIndex = 0;
    for (size_t i = 0; i < curves.size() && leftIndex < leftAxisData.size(); i++) {
        const auto& curve = curves[i];
        if (!curve.enabled || curve.yAxisGroup != 0) continue;
        
        const auto& channel = leftAxisData[leftIndex];
        if (channel.empty()) {
            leftIndex++;
            continue;
        }
        
        HPEN hPen = CreatePen(PS_SOLID, 2, curve.color);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        
        bool firstPoint = true;
        for (size_t j = 0; j < channel.size(); j++) {
            int x = plotRect.left + (int)((double)j / (channel.size() - 1) * (plotRect.right - plotRect.left));
            int y = plotRect.bottom - (int)((channel[j] - leftMin) / (leftMax - leftMin) * (plotRect.bottom - plotRect.top));
            
            if (firstPoint) {
                MoveToEx(hdc, x, y, NULL);
                firstPoint = false;
            } else {
                LineTo(hdc, x, y);
            }
        }
        
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
        leftIndex++;
    }
    
    // Отрисовка кривых правой оси (пунктирные линии)
    size_t rightIndex = 0;
    for (size_t i = 0; i < curves.size() && rightIndex < rightAxisData.size(); i++) {
        const auto& curve = curves[i];
        if (!curve.enabled || curve.yAxisGroup != 1) continue;
        
        const auto& channel = rightAxisData[rightIndex];
        if (channel.empty()) {
            rightIndex++;
            continue;
        }
        
        HPEN hPen = CreatePen(PS_DASH, 2, curve.color);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        
        bool firstPoint = true;
        for (size_t j = 0; j < channel.size(); j++) {
            int x = plotRect.left + (int)((double)j / (channel.size() - 1) * (plotRect.right - plotRect.left));
            int y = plotRect.bottom - (int)((channel[j] - rightMin) / (rightMax - rightMin) * (plotRect.bottom - plotRect.top));
            
            if (firstPoint) {
                MoveToEx(hdc, x, y, NULL);
                firstPoint = false;
            } else {
                LineTo(hdc, x, y);
            }
        }
        
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
        rightIndex++;
    }
}

void ChartModule::DrawDualAxisLegend(HDC hdc, const RECT& rect,
                                    const std::vector<CurveDefinition>& curves) const {
    int legendX = rect.right - 250; // Больше места для двойной легенды
    int legendY = rect.top + 10;
    int lineHeight = 20;
    int currentY = legendY;
    
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    SetBkMode(hdc, TRANSPARENT);
    
    // Группируем кривые по осям
    std::vector<const CurveDefinition*> leftAxisCurves;
    std::vector<const CurveDefinition*> rightAxisCurves;
    
    for (const auto& curve : curves) {
        if (!curve.enabled) continue;
        
        if (curve.yAxisGroup == 0) {
            leftAxisCurves.push_back(&curve);
        } else {
            rightAxisCurves.push_back(&curve);
        }
    }
    
    // Отрисовка легенды левой оси
    if (!leftAxisCurves.empty()) {
        SetTextColor(hdc, RGB(0, 0, 0));
        TextOut(hdc, legendX, currentY, L"Left Axis:", 10);
        currentY += lineHeight;
        
        for (const auto* curve : leftAxisCurves) {
            // Цветной квадрат
            HBRUSH hBrush = CreateSolidBrush(curve->color);
            HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
            Rectangle(hdc, legendX + 10, currentY + 5, legendX + 20, currentY + 15);
            SelectObject(hdc, hOldBrush);
            DeleteObject(hBrush);
            
            // Линия (сплошная)
            HPEN hPen = CreatePen(PS_SOLID, 2, curve->color);
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
            MoveToEx(hdc, legendX + 25, currentY + 10, NULL);
            LineTo(hdc, legendX + 40, currentY + 10);
            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
            
            // Название
            std::wstring name(curve->displayName.begin(), curve->displayName.end());
            TextOut(hdc, legendX + 45, currentY + 2, name.c_str(), (int)name.length());
            
            currentY += lineHeight;
        }
    }
    
    // Отрисовка легенды правой оси
    if (!rightAxisCurves.empty()) {
        currentY += 5; // Дополнительный отступ
        SetTextColor(hdc, RGB(0, 0, 0));
        TextOut(hdc, legendX, currentY, L"Right Axis:", 11);
        currentY += lineHeight;
        
        for (const auto* curve : rightAxisCurves) {
            // Цветной квадрат
            HBRUSH hBrush = CreateSolidBrush(curve->color);
            HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
            Rectangle(hdc, legendX + 10, currentY + 5, legendX + 20, currentY + 15);
            SelectObject(hdc, hOldBrush);
            DeleteObject(hBrush);
            
            // Линия (пунктирная)
            HPEN hPen = CreatePen(PS_DASH, 2, curve->color);
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
            MoveToEx(hdc, legendX + 25, currentY + 10, NULL);
            LineTo(hdc, legendX + 40, currentY + 10);
            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
            
            // Название
            std::wstring name(curve->displayName.begin(), curve->displayName.end());
            TextOut(hdc, legendX + 45, currentY + 2, name.c_str(), (int)name.length());
            
            currentY += lineHeight;
        }
    }
    
    SelectObject(hdc, hOldFont);
}