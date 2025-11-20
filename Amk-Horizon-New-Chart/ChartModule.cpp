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
    
    // Отрисовка данных с учетом zoom
    if (hasData) {
        DrawChartData(hdc, plotRect, chartData, minXData, maxXData);
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

// Перегрузка с поддержкой zoom для правильного отображения X координат
void ChartModule::DrawChartData(HDC hdc, const RECT& plotRect, const std::vector<std::vector<float>>& chartData,
                                double minXData, double maxXData) const {
    // Находим минимальные и максимальные значения с padding
    float minVal, maxVal;
    FindMinMaxValues(chartData, minVal, maxVal);
    
    if (maxXData <= minXData) {
        maxXData = minXData + 1; // Избегаем деления на ноль
    }
    
    // Полный диапазон данных (от 0 до размера массива)
    double fullXRange = static_cast<double>(chartData.empty() || chartData[0].empty() ? 1 : chartData[0].size() - 1);
    
    // Рисуем данные для каждого канала
    for (size_t channelIdx = 0; channelIdx < chartData.size(); channelIdx++) {
        const auto& channel = chartData[channelIdx];
        if (channel.empty())
            continue;
            
        HPEN hPen = CreatePen(PS_SOLID, 2, CHANNEL_COLORS[channelIdx % 4]);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        
        bool firstPoint = true;
        for (size_t i = 0; i < channel.size(); i++) {
            // Реальная X координата в данных (индекс i соответствует реальной позиции в данных)
            double realX = static_cast<double>(i);
            
            // Проверяем, находится ли точка в видимом диапазоне zoom
            if (realX < minXData || realX > maxXData) {
                continue; // Пропускаем точки вне видимого диапазона
            }
            
            // Вычисляем экранные координаты относительно видимого диапазона
            int x = plotRect.left + (int)((realX - minXData) / (maxXData - minXData) * (plotRect.right - plotRect.left));
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

    // Параметры легенды
    int checkboxSize = 12;        // Размер чекбокса
    int elementSpacing = 15;      // Промежуток между элементами легенды
    int lineHeight = 20;          // Высота строки легенды
    
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    SetBkMode(hdc, TRANSPARENT);
    
    // Вычисляем ширину каждого элемента легенды
    struct LegendElement {
        int width;
        int channelIdx;
    };
    
    std::vector<LegendElement> elements;
    int totalWidth = 0;
    int lineLength = 15; // Длина цветной линии
    
    for (size_t channelIdx = 0; channelIdx < 4; channelIdx++) {
        if (isMultiChannel && channelIdx < enabledTransmitters.size()) {
            // Ширина элемента = чекбокс + отступ + линия + отступ + текст
            SIZE textSize;
            GetTextExtentPoint32(hdc, CHANNEL_NAMES[channelIdx], (int)wcslen(CHANNEL_NAMES[channelIdx]), &textSize);
            int elementWidth = checkboxSize + 5 + lineLength + 5 + textSize.cx;
            
            elements.push_back({elementWidth, (int)channelIdx});
            totalWidth += elementWidth;
            
            if (channelIdx < 3) { // Добавляем промежуток между элементами (кроме последнего)
                totalWidth += elementSpacing;
            }
        }
    }
    
    // Вычисляем начальную X позицию для центрирования
    int chartWidth = rect.right - rect.left;
    int startX = rect.left + (chartWidth - totalWidth) / 2;
    
    // Позиция Y - под заголовком
    int legendY = rect.top + 35;
    
    // Отрисовка элементов легенды
    int currentX = startX;
    
    for (const auto& elem : elements) {
        int channelIdx = elem.channelIdx;
        COLORREF color = CHANNEL_COLORS[channelIdx % 4];
        
        // Отрисовка чекбокса
        RECT checkboxRect = {currentX,
                             legendY + (lineHeight - checkboxSize) / 2,
                             currentX + checkboxSize,
                             legendY + (lineHeight + checkboxSize) / 2};
        DrawFrameControl(hdc, &checkboxRect, DFC_BUTTON, DFCS_BUTTONCHECK | (enabledTransmitters[channelIdx] ? DFCS_CHECKED : 0));
        
        currentX += checkboxSize + 5;
        
        // Отрисовка цветной линии
        HPEN hPen = CreatePen(PS_SOLID, 2, color);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        
        MoveToEx(hdc, currentX, legendY + lineHeight / 2, NULL);
        LineTo(hdc, currentX + lineLength, legendY + lineHeight / 2);
        
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
        
        currentX += lineLength + 5;
        
        // Отрисовка названия канала
        TextOut(hdc, currentX, legendY, CHANNEL_NAMES[channelIdx], (int)wcslen(CHANNEL_NAMES[channelIdx]));
        
        SIZE textSize;
        GetTextExtentPoint32(hdc, CHANNEL_NAMES[channelIdx], (int)wcslen(CHANNEL_NAMES[channelIdx]), &textSize);
        currentX += textSize.cx + elementSpacing;
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

    // Параметры легенды (должны совпадать с DrawChartLegend)
    int checkboxSize = 12;
    int lineLength = 15;
    int elementSpacing = 15;
    int lineHeight = 20;
    
    // Создаем временный DC для вычисления размеров текста
    HDC hdc = GetDC(NULL);
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    
    // Вычисляем ширину каждого элемента и общую ширину
    struct LegendElement {
        int width;
        int channelIdx;
        int checkboxX;
    };
    
    std::vector<LegendElement> elements;
    int totalWidth = 0;
    
    for (int channelIdx = 0; channelIdx < 4; channelIdx++) {
        if (isMultiChannel) {
            SIZE textSize;
            GetTextExtentPoint32(hdc, CHANNEL_NAMES[channelIdx], (int)wcslen(CHANNEL_NAMES[channelIdx]), &textSize);
            int elementWidth = checkboxSize + 5 + lineLength + 5 + textSize.cx;
            
            elements.push_back({elementWidth, channelIdx, 0});
            totalWidth += elementWidth;
            
            if (channelIdx < 3) {
                totalWidth += elementSpacing;
            }
        }
    }
    
    SelectObject(hdc, hOldFont);
    ReleaseDC(NULL, hdc);
    
    // Вычисляем начальную X позицию для центрирования
    int chartWidth = chartRect.right - chartRect.left;
    int startX = chartRect.left + (chartWidth - totalWidth) / 2;
    int legendY = chartRect.top + 35;
    
    // Вычисляем позиции чекбоксов
    int currentX = startX;
    for (auto& elem : elements) {
        elem.checkboxX = currentX; // Чекбокс теперь первый элемент
        currentX += elem.width + elementSpacing;
    }
    
    // Проверяем попадание в чекбоксы
    for (const auto& elem : elements) {
        RECT checkboxRect = {elem.checkboxX,
                             legendY + (lineHeight - checkboxSize) / 2,
                             elem.checkboxX + checkboxSize,
                             legendY + (lineHeight + checkboxSize) / 2};
        
        if (PtInRect(&checkboxRect, pt)) {
            return elem.channelIdx;
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
                
                // Вычисляем ширину текста для правильного выравнивания
                SIZE textSize;
                GetTextExtentPoint32(hdc, yLabel.c_str(), (int)yLabel.length(), &textSize);
                
                // Рисуем текст так, чтобы он заканчивался в 5 пикселях от оси
                // Текст начинается с (plotRect.left - 5 - textSize.cx) и заканчивается в (plotRect.left - 5)
                int textX = plotRect.left - 5 - textSize.cx;
                TextOut(hdc, textX, screenY - 7, yLabel.c_str(), (int)yLabel.length());
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
    plotRect.left += 80;   // Увеличенный отступ для Y-оси (для длинных значений мВ и мдег)
    plotRect.top += 65;    // Увеличенный отступ для заголовка + горизонтальная легенда
    plotRect.right -= 30;  // Увеличенный отступ справа
    plotRect.bottom -= 50; // Увеличенный отступ для X-оси и скроллбара
    return plotRect;
}

RECT ChartModule::CalculateDualAxisPlotRect(const RECT& chartRect) const {
    RECT plotRect = chartRect;
    plotRect.left += 80;   // Увеличенный отступ для левой Y-оси (для длинных значений мВ)
    plotRect.top += 65;    // Увеличенный отступ для заголовка + горизонтальная легенда
    plotRect.right -= 120; // Увеличенный отступ для правой Y-оси (для длинных значений мдег)
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
    // Сохраняем текущий режим отрисовки
    int oldROP = SetROP2(hdc, R2_NOTXORPEN);
    
    // Создаем яркое оранжевое перо с пунктирной линией для рамки
    LOGBRUSH lb;
    lb.lbStyle = BS_SOLID;
    lb.lbColor = RGB(255, 140, 0); // Яркий оранжевый
    lb.lbHatch = 0;
    
    // Создаем расширенное перо с пунктирным стилем для рамки
    DWORD penStyle[2] = {8, 4}; // 8 пикселей линия, 4 пикселя пробел
    HPEN hPen = ExtCreatePen(PS_GEOMETRIC | PS_USERSTYLE, 2, &lb, 2, penStyle);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    
    // Устанавливаем прозрачную кисть (без заливки)
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    
    // Рисуем прямоугольник рамки
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
    
    // Расчет области для графика с двойными осями
    RECT plotRect = CalculateDualAxisPlotRect(rect);
    
    if (plotRect.right <= plotRect.left || plotRect.bottom <= plotRect.top)
        return;
    
    // Отрисовка сетки и осей (без оси X для старой версии)
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

// Перегрузка с поддержкой zoom
void ChartModule::DrawDualAxisChart(HDC hdc, const RECT& rect, 
                                   const std::vector<CurveDefinition>& curves,
                                   const std::vector<std::vector<float>>& leftAxisData,
                                   const std::vector<std::vector<float>>& rightAxisData,
                                   const std::wstring& title, bool hasData,
                                   double minXData, double maxXData, 
                                   double minYData, double maxYData, bool showAxes) const {
    if (rect.right <= rect.left || rect.bottom <= rect.top)
        return;

    // Отрисовка фона и рамки
    DrawDualAxisBackground(hdc, rect);
    DrawChartBorder(hdc, rect);
    
    // Отрисовка заголовка
    DrawChartTitle(hdc, rect, title);
    
    // Расчет области для графика с двойными осями
    RECT plotRect = CalculateDualAxisPlotRect(rect);
    
    if (plotRect.right <= plotRect.left || plotRect.bottom <= plotRect.top)
        return;
    
    // Отрисовка сетки и осей с учетом zoom (включая ось X)
    if (showAxes) {
        DrawDualAxisGrid(hdc, plotRect, leftAxisData, rightAxisData, minXData, maxXData);
    }
    
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

// Перегрузка с поддержкой оси X
void ChartModule::DrawDualAxisGrid(HDC hdc, const RECT& plotRect, 
                                  const std::vector<std::vector<float>>& leftAxisData,
                                  const std::vector<std::vector<float>>& rightAxisData,
                                  double minXData, double maxXData) const {
    // Сначала рисуем обычную сетку для Y осей
    DrawDualAxisGrid(hdc, plotRect, leftAxisData, rightAxisData);
    
    // Теперь добавляем ось X
    if (maxXData <= minXData) {
        maxXData = minXData + 1; // Избегаем деления на ноль
    }
    
    double xRange = maxXData - minXData;
    double niceStepX = CalculateNiceStep(xRange, (plotRect.right - plotRect.left) / 60);
    double currentXVal = CalculateNiceMin(minXData, niceStepX);
    
    SetTextColor(hdc, RGB(0, 0, 0));
    SetBkMode(hdc, TRANSPARENT);
    
    while (currentXVal <= maxXData + niceStepX / 2) {
        if (currentXVal >= minXData - niceStepX / 2 && currentXVal <= maxXData + niceStepX / 2) {
            long screenX = plotRect.left + (LONG)((currentXVal - minXData) / xRange * (plotRect.right - plotRect.left));
            
            if (screenX >= plotRect.left && screenX <= plotRect.right) {
                // Рисуем вертикальную линию сетки
                HPEN hPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
                HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
                
                MoveToEx(hdc, screenX, plotRect.top, NULL);
                LineTo(hdc, screenX, plotRect.bottom);
                
                SelectObject(hdc, hOldPen);
                DeleteObject(hPen);
                
                // Отрисовка метки X-оси с одной цифрой после запятой
                std::wstringstream ssX;
                ssX << std::fixed << std::setprecision(1) << currentXVal;
                std::wstring xLabel = ssX.str();
                TextOut(hdc, screenX - 10, plotRect.bottom + 5, xLabel.c_str(), (int)xLabel.length());
            }
        }
        currentXVal += niceStepX;
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
                
                // Вычисляем ширину текста для правильного выравнивания
                SIZE textSize;
                GetTextExtentPoint32(hdc, label.c_str(), (int)label.length(), &textSize);
                
                // Рисуем текст так, чтобы он заканчивался в 5 пикселях от оси
                // Текст начинается с (plotRect.left - 5 - textSize.cx) и заканчивается в (plotRect.left - 5)
                int textX = plotRect.left - 5 - textSize.cx;
                TextOut(hdc, textX, screenY - 7, label.c_str(), (int)label.length());
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
    
    // Рисуем правую ось с отступом от края
    int rightAxisX = plotRect.right + 40; // Отступ 40 пикселей от области графика
    
    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    
    MoveToEx(hdc, rightAxisX, plotRect.top, NULL);
    LineTo(hdc, rightAxisX, plotRect.bottom);
    
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
                TextOut(hdc, rightAxisX + 5, screenY - 7, label.c_str(), (int)label.length());
            }
        }
        currentYVal += niceStepY;
    }
    
    // Подпись единиц измерения правой оси
    if (!units.empty()) {
        std::wstring wunits(units.begin(), units.end());
        TextOut(hdc, rightAxisX + 5, plotRect.top - 20, wunits.c_str(), (int)wunits.length());
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
    // Параметры легенды
    int lineLength = 15;
    int elementSpacing = 15;
    int lineHeight = 20;
    int separatorSpacing = 25; // Дополнительный отступ для разделителя
    
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
    
    if (leftAxisCurves.empty() && rightAxisCurves.empty()) {
        SelectObject(hdc, hOldFont);
        return;
    }
    
    // Получаем единицы измерения для каждой оси
    std::string leftUnits, rightUnits;
    if (!leftAxisCurves.empty()) {
        leftUnits = leftAxisCurves[0]->units;
    }
    if (!rightAxisCurves.empty()) {
        rightUnits = rightAxisCurves[0]->units;
    }
    
    // Вычисляем ширину каждого элемента легенды
    struct LegendElement {
        int width;
        const CurveDefinition* curve;
        int axisGroup; // 0 = left, 1 = right
    };
    
    std::vector<LegendElement> elements;
    int leftGroupWidth = 0;
    int rightGroupWidth = 0;
    
    // Вычисляем ширину подписи левой оси
    std::wstring leftAxisLabel = L"[Left Axis (" + std::wstring(leftUnits.begin(), leftUnits.end()) + L")]";
    SIZE leftLabelSize;
    GetTextExtentPoint32(hdc, leftAxisLabel.c_str(), (int)leftAxisLabel.length(), &leftLabelSize);
    leftGroupWidth = leftLabelSize.cx + 10; // +10 для отступа после подписи
    
    // Добавляем элементы левой оси
    for (const auto* curve : leftAxisCurves) {
        std::wstring name(curve->displayName.begin(), curve->displayName.end());
        SIZE textSize;
        GetTextExtentPoint32(hdc, name.c_str(), (int)name.length(), &textSize);
        
        int elementWidth = lineLength + 5 + textSize.cx;
        elements.push_back({elementWidth, curve, 0});
        leftGroupWidth += elementWidth + elementSpacing;
    }
    
    // Ширина разделителя " | "
    int separatorWidth = 20;
    
    // Вычисляем ширину подписи правой оси
    std::wstring rightAxisLabel = L"[Right Axis (" + std::wstring(rightUnits.begin(), rightUnits.end()) + L")]";
    SIZE rightLabelSize;
    GetTextExtentPoint32(hdc, rightAxisLabel.c_str(), (int)rightAxisLabel.length(), &rightLabelSize);
    rightGroupWidth = rightLabelSize.cx + 10; // +10 для отступа после подписи
    
    // Добавляем элементы правой оси
    for (const auto* curve : rightAxisCurves) {
        std::wstring name(curve->displayName.begin(), curve->displayName.end());
        SIZE textSize;
        GetTextExtentPoint32(hdc, name.c_str(), (int)name.length(), &textSize);
        
        int elementWidth = lineLength + 5 + textSize.cx;
        elements.push_back({elementWidth, curve, 1});
        rightGroupWidth += elementWidth + elementSpacing;
    }
    
    // Общая ширина легенды
    int totalWidth = leftGroupWidth + separatorSpacing + separatorWidth + separatorSpacing + rightGroupWidth;
    
    // Вычисляем начальную X позицию для центрирования
    int chartWidth = rect.right - rect.left;
    int startX = rect.left + (chartWidth - totalWidth) / 2;
    
    // Позиция Y - под заголовком
    int legendY = rect.top + 35;
    
    // Отрисовка легенды
    int currentX = startX;
    
    // Отрисовка подписи левой оси
    if (!leftAxisCurves.empty()) {
        SetTextColor(hdc, RGB(0, 0, 0));
        TextOut(hdc, currentX, legendY, leftAxisLabel.c_str(), (int)leftAxisLabel.length());
        currentX += leftLabelSize.cx + 10;
        
        // Отрисовка элементов левой оси
        for (const auto& elem : elements) {
            if (elem.axisGroup != 0) continue;
            
            const CurveDefinition* curve = elem.curve;
            
            // Линия (сплошная для левой оси)
            HPEN hPen = CreatePen(PS_SOLID, 2, curve->color);
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
            
            MoveToEx(hdc, currentX, legendY + lineHeight / 2, NULL);
            LineTo(hdc, currentX + lineLength, legendY + lineHeight / 2);
            
            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
            
            currentX += lineLength + 5;
            
            // Название
            std::wstring name(curve->displayName.begin(), curve->displayName.end());
            TextOut(hdc, currentX, legendY, name.c_str(), (int)name.length());
            
            SIZE textSize;
            GetTextExtentPoint32(hdc, name.c_str(), (int)name.length(), &textSize);
            currentX += textSize.cx + elementSpacing;
        }
        
        currentX += separatorSpacing - elementSpacing; // Убираем лишний отступ после последнего элемента
    }
    
    // Отрисовка разделителя " | "
    if (!leftAxisCurves.empty() && !rightAxisCurves.empty()) {
        SetTextColor(hdc, RGB(128, 128, 128)); // Серый цвет для разделителя
        TextOut(hdc, currentX, legendY, L"|", 1);
        currentX += separatorWidth + separatorSpacing;
    }
    
    // Отрисовка подписи правой оси
    if (!rightAxisCurves.empty()) {
        SetTextColor(hdc, RGB(0, 0, 0));
        TextOut(hdc, currentX, legendY, rightAxisLabel.c_str(), (int)rightAxisLabel.length());
        currentX += rightLabelSize.cx + 10;
        
        // Отрисовка элементов правой оси
        for (const auto& elem : elements) {
            if (elem.axisGroup != 1) continue;
            
            const CurveDefinition* curve = elem.curve;
            
            // Линия (пунктирная для правой оси)
            HPEN hPen = CreatePen(PS_DASH, 2, curve->color);
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
            
            MoveToEx(hdc, currentX, legendY + lineHeight / 2, NULL);
            LineTo(hdc, currentX + lineLength, legendY + lineHeight / 2);
            
            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
            
            currentX += lineLength + 5;
            
            // Название
            std::wstring name(curve->displayName.begin(), curve->displayName.end());
            TextOut(hdc, currentX, legendY, name.c_str(), (int)name.length());
            
            SIZE textSize;
            GetTextExtentPoint32(hdc, name.c_str(), (int)name.length(), &textSize);
            currentX += textSize.cx + elementSpacing;
        }
    }
    
    SelectObject(hdc, hOldFont);
}

// Метод для получения диапазонов данных
void ChartModule::GetDataRanges(const std::vector<std::vector<float>>& leftAxisData,
                               const std::vector<std::vector<float>>& rightAxisData,
                               double& minX, double& maxX,
                               double& minY1, double& maxY1,
                               double& minY2, double& maxY2) const {
    minX = 0.0;
    maxX = 0.0;
    minY1 = 0.0;
    maxY1 = 0.0;
    minY2 = 0.0;
    maxY2 = 0.0;

    // Определяем диапазон X (одинаковый для обеих осей)
    if (!leftAxisData.empty() && !leftAxisData[0].empty()) {
        maxX = static_cast<double>(leftAxisData[0].size() - 1);
    } else if (!rightAxisData.empty() && !rightAxisData[0].empty()) {
        maxX = static_cast<double>(rightAxisData[0].size() - 1);
    }

    // Определяем диапазон Y для левой оси
    if (!leftAxisData.empty()) {
        float minVal = FLT_MAX;
        float maxVal = -FLT_MAX;
        FindMinMaxValues(leftAxisData, minVal, maxVal);
        minY1 = static_cast<double>(minVal);
        maxY1 = static_cast<double>(maxVal);
    }

    // Определяем диапазон Y для правой оси
    if (!rightAxisData.empty()) {
        float minVal = FLT_MAX;
        float maxVal = -FLT_MAX;
        FindMinMaxValues(rightAxisData, minVal, maxVal);
        minY2 = static_cast<double>(minVal);
        maxY2 = static_cast<double>(maxVal);
    }
}


// v2.10.0: Перегрузка DrawChart с поддержкой температуры на оси X
void ChartModule::DrawChart(HDC hdc, const RECT& rect, const std::vector<std::vector<float>>& chartData,
                           DataType type, const std::wstring& title, bool hasData,
                           const std::vector<bool>& enabledTransmitters, const DataModule* dataModule,
                           double minXData, double maxXData, double minYData, double maxYData,
                           const std::vector<float>& xAxisData, bool showAxes) const {
    if (rect.right <= rect.left || rect.bottom <= rect.top)
        return;

    // Отрисовка фона и рамки
    DrawChartBackground(hdc, rect);
    DrawChartBorder(hdc, rect);
    
    // Отрисовка заголовка
    DrawChartTitle(hdc, rect, title);
    
    // Расчет области для графика
    RECT plotRect = CalculatePlotRect(rect);

    // Отрисовка осей
    DrawChartAxes(hdc, plotRect, showAxes);

    if (plotRect.right <= plotRect.left || plotRect.bottom <= plotRect.top)
        return;
    
    // Отрисовка сетки и осей с метками (с температурой)
    DrawGridAndAxes(hdc, plotRect, chartData, dataModule, xAxisData,
                    minXData, maxXData, minYData, maxYData);
    
    // Отрисовка данных с температурой на оси X
    if (hasData && !xAxisData.empty()) {
        DrawChartData(hdc, plotRect, chartData, xAxisData, minXData, maxXData);
    }
    
    // Отрисовка легенды
    if (chartData.size() > 1) {
        DrawChartLegend(hdc, rect, chartData, type, enabledTransmitters);
    }
}

// v2.10.0: Отрисовка данных с температурой на оси X
void ChartModule::DrawChartData(HDC hdc, const RECT& plotRect, const std::vector<std::vector<float>>& chartData,
                                const std::vector<float>& xAxisData, double minXData, double maxXData) const {
    if (xAxisData.empty() || chartData.empty()) return;
    
    // Находим минимальные и максимальные значения Y
    float minVal, maxVal;
    FindMinMaxValues(chartData, minVal, maxVal);
    
    // Находим минимальные и максимальные значения температуры для отображаемого диапазона
    size_t startIdx = static_cast<size_t>(std::max(0.0, minXData));
    size_t endIdx = static_cast<size_t>(std::min(static_cast<double>(xAxisData.size()), maxXData + 1));
    
    if (startIdx >= endIdx || endIdx > xAxisData.size()) return;
    
    float minTemp = *std::min_element(xAxisData.begin() + startIdx, xAxisData.begin() + endIdx);
    float maxTemp = *std::max_element(xAxisData.begin() + startIdx, xAxisData.begin() + endIdx);
    
    if (maxTemp <= minTemp) {
        maxTemp = minTemp + 1.0f; // Избегаем деления на ноль
    }
    
    // Рисуем данные для каждого канала
    for (size_t channelIdx = 0; channelIdx < chartData.size(); channelIdx++) {
        const auto& channel = chartData[channelIdx];
        if (channel.empty()) continue;
        
        HPEN hPen = CreatePen(PS_SOLID, 2, CHANNEL_COLORS[channelIdx % 4]);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        
        bool firstPoint = true;
        for (size_t i = startIdx; i < endIdx && i < channel.size() && i < xAxisData.size(); i++) {
            // X координата основана на температуре
            float temp = xAxisData[i];
            int x = plotRect.left + (int)((temp - minTemp) / (maxTemp - minTemp) * (plotRect.right - plotRect.left));
            
            // Y координата основана на значении данных
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

// v2.10.0: Отрисовка сетки и осей с температурой на оси X
void ChartModule::DrawGridAndAxes(HDC hdc, const RECT& plotRect, const std::vector<std::vector<float>>& chartData,
                                  const DataModule* dataModule, const std::vector<float>& xAxisData,
                                  double minXData, double maxXData, double minYData, double maxYData) const {
    if (xAxisData.empty()) return;
    
    // Находим диапазон температур для отображаемого диапазона
    size_t startIdx = static_cast<size_t>(std::max(0.0, minXData));
    size_t endIdx = static_cast<size_t>(std::min(static_cast<double>(xAxisData.size()), maxXData + 1));
    
    if (startIdx >= endIdx || endIdx > xAxisData.size()) return;
    
    float minTemp = *std::min_element(xAxisData.begin() + startIdx, xAxisData.begin() + endIdx);
    float maxTemp = *std::max_element(xAxisData.begin() + startIdx, xAxisData.begin() + endIdx);
    
    if (maxTemp <= minTemp) {
        maxTemp = minTemp + 1.0f;
    }
    
    // Находим диапазон Y
    float minVal, maxVal;
    FindMinMaxValues(chartData, minVal, maxVal, minYData, maxYData);
    
    // Рисуем сетку
    HPEN hGridPen = CreatePen(PS_DOT, 1, RGB(200, 200, 200));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hGridPen);
    
    // Вертикальные линии сетки (по температуре)
    int numVerticalLines = 10;
    for (int i = 0; i <= numVerticalLines; i++) {
        int x = plotRect.left + (plotRect.right - plotRect.left) * i / numVerticalLines;
        MoveToEx(hdc, x, plotRect.top, NULL);
        LineTo(hdc, x, plotRect.bottom);
        
        // Подпись температуры
        float temp = minTemp + (maxTemp - minTemp) * i / numVerticalLines;
        wchar_t label[32];
        swprintf_s(label, L"%.1f°C", temp);
        
        SIZE textSize;
        GetTextExtentPoint32(hdc, label, (int)wcslen(label), &textSize);
        TextOut(hdc, x - textSize.cx / 2, plotRect.bottom + 5, label, (int)wcslen(label));
    }
    
    // Горизонтальные линии сетки (по Y)
    int numHorizontalLines = 10;
    for (int i = 0; i <= numHorizontalLines; i++) {
        int y = plotRect.top + (plotRect.bottom - plotRect.top) * i / numHorizontalLines;
        MoveToEx(hdc, plotRect.left, y, NULL);
        LineTo(hdc, plotRect.right, y);
        
        // Подпись Y
        float value = maxVal - (maxVal - minVal) * i / numHorizontalLines;
        wchar_t label[32];
        swprintf_s(label, L"%.2f", value);
        
        SIZE textSize;
        GetTextExtentPoint32(hdc, label, (int)wcslen(label), &textSize);
        TextOut(hdc, plotRect.left - textSize.cx - 5, y - textSize.cy / 2, label, (int)wcslen(label));
    }
    
    SelectObject(hdc, hOldPen);
    DeleteObject(hGridPen);
}
