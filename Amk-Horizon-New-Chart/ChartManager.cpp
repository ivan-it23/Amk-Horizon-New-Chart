#include "framework.h"
#include "ChartManager.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cfloat>
#include <cmath>

// Реализация класса ChartManager с модульной архитектурой
ChartManager::ChartManager() 
    : dataModule(std::make_unique<DataModule>())
    , chartModule(std::make_unique<ChartModule>())
    , uiModule(std::make_unique<UIModule>())
    , layoutModule(std::make_unique<LayoutModule>())
    , isSelecting(false)
    , activeChart(0)
{
    selectionStart.x = selectionStart.y = 0;
    selectionEnd.x = selectionEnd.y = 0;
}

ChartManager::~ChartManager() {
}

void ChartManager::CreateControls(HWND hWnd, HINSTANCE hInst)
{
    uiModule->CreateControls(hWnd, hInst);
}

void ChartManager::UpdateLayout(HWND hWnd)
{
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    
    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;
    
    bool dualMode = uiModule->IsDualMode();
    
    // Валидация ограничений компоновки
    if (!layoutModule->ValidateLayoutConstraints(width, height, dualMode))
        return;
    
    // Расчет компоновки
    layoutModule->CalculateLayout(width, height, dualMode);
    
    // Управление видимостью и позиционированием элементов управления
    uiModule->UpdateControlsVisibility(dualMode);
    
    if (dualMode) {
        int panel2Y = layoutModule->GetChart2PanelY();
        uiModule->PositionChart2Controls(panel2Y);
    }
    
    // Обновляем скроллбары после изменения компоновки
    UpdateScrollBars(hWnd);
}

void ChartManager::ToggleDualMode()
{
    uiModule->ToggleDualMode();
}

void ChartManager::RefreshChart1()
{
    uiModule->RefreshChart1Settings();
}

void ChartManager::RefreshChart2()
{
    uiModule->RefreshChart2Settings();
}

void ChartManager::DrawChart1(HDC hdc)
{
    const RECT& chart1Rect = layoutModule->GetChart1Rect();
    
    // Получаем данные для отображения
    std::vector<std::vector<float>> chartData;
    DataType dataType = uiModule->GetChart1DataType();
    FrequencyBand frequency = uiModule->GetChart1Frequency();
    const std::vector<bool>& enabledTransmitters = uiModule->GetChart1EnabledTransmitters();
    
    // Используем диапазон масштабирования первого графика
    if (chart1Zoom.isZoomed) {
        dataModule->CalculateChartData(dataType, frequency, chartData, 
                                      enabledTransmitters, chart1Zoom.minX, chart1Zoom.maxX, 
                                      chart1Zoom.minY, chart1Zoom.maxY);
    } else {
        dataModule->CalculateChartData(dataType, frequency, chartData, enabledTransmitters, 
                                      0.0, static_cast<double>(dataModule->GetDataSize() - 1), -1e6, 1e6);
    }
    
    // Формируем заголовок
    std::wstring title = BuildChartTitle(1, dataType, frequency);
    
    // Отрисовка графика (оси скрыты, если применен зум)
    bool showAxes = !chart1Zoom.isZoomed;
    chartModule->DrawChart(hdc, chart1Rect, chartData, dataType, title, dataModule->HasData(), 
                           enabledTransmitters, dataModule.get(),
                           chart1Zoom.isZoomed ? chart1Zoom.minX : 0.0,
                           chart1Zoom.isZoomed ? chart1Zoom.maxX : static_cast<double>(dataModule->GetDataSize() > 0 ? dataModule->GetDataSize() - 1 : 0),
                           chart1Zoom.isZoomed ? chart1Zoom.minY : -1e6, 
                           chart1Zoom.isZoomed ? chart1Zoom.maxY : 1e6, showAxes);
}

void ChartManager::DrawChart2(HDC hdc)
{
    if (!uiModule->IsDualMode())
        return;
        
    const RECT& chart2Rect = layoutModule->GetChart2Rect();
    
    // Получаем данные для отображения
    std::vector<std::vector<float>> chartData;
    DataType dataType = uiModule->GetChart2DataType();
    FrequencyBand frequency = uiModule->GetChart2Frequency();
    const std::vector<bool>& enabledTransmitters = uiModule->GetChart2EnabledTransmitters();
    
    // Используем диапазон масштабирования второго графика
    if (chart2Zoom.isZoomed) {
        dataModule->CalculateChartData(dataType, frequency, chartData, 
                                      enabledTransmitters, chart2Zoom.minX, chart2Zoom.maxX, 
                                      chart2Zoom.minY, chart2Zoom.maxY);
    } else {
        dataModule->CalculateChartData(dataType, frequency, chartData, enabledTransmitters,
                                      0.0, static_cast<double>(dataModule->GetDataSize() > 0 ? dataModule->GetDataSize() - 1 : 0), -1e6, 1e6);
    }
    
    // Формируем заголовок
    std::wstring title = BuildChartTitle(2, dataType, frequency);
    
    // Отрисовка графика (оси скрыты, если применен зум)
    bool showAxes = !chart2Zoom.isZoomed;
    chartModule->DrawChart(hdc, chart2Rect, chartData, dataType, title, dataModule->HasData(), 
                           enabledTransmitters, dataModule.get(),
                           chart2Zoom.isZoomed ? chart2Zoom.minX : 0.0,
                           chart2Zoom.isZoomed ? chart2Zoom.maxX : static_cast<double>(dataModule->GetDataSize() > 0 ? dataModule->GetDataSize() - 1 : 0),
                           chart2Zoom.isZoomed ? chart2Zoom.minY : -1e6, 
                           chart2Zoom.isZoomed ? chart2Zoom.maxY : 1e6, showAxes);
}

void ChartManager::LoadDataFromFile(const std::wstring& filename)
{
    dataModule->LoadDataFromFile(filename);
    
    // Сбрасываем масштабирование для обоих графиков при загрузке новых данных
    chart1Zoom.isZoomed = false;
    chart2Zoom.isZoomed = false;
}

bool ChartManager::HandleCommand(WPARAM wParam, LPARAM lParam)
{
    HWND hWnd = GetParent((HWND)lParam);
    bool needsRedraw = uiModule->HandleCommand(wParam, lParam, hWnd);
    
    // Если произошло переключение режима, обновляем компоновку
    int wmId = LOWORD(wParam);
    if (wmId == IDC_DUAL_MODE_CHECK) {
        UpdateLayout(hWnd);
    }
    
    return needsRedraw;
}

bool ChartManager::IsDualMode() const
{
    return uiModule->IsDualMode();
}

std::wstring ChartManager::BuildChartTitle(int chartNumber, DataType dataType, FrequencyBand frequency) const {
    std::wstring title = L"Chart ";
    title += std::to_wstring(chartNumber);
    title += L": ";
    title += dataModule->GetDataTypeName(dataType); // Используем DataModule для получения имени типа данных
    title += L" (";
    title += (frequency == FrequencyBand::FREQ_400_KHZ) ? L"400 kHz" : L"2000 kHz";
    title += L")";
    return title;
}

void ChartManager::HandleLegendCheckboxClick(HWND hWnd, int x, int y, int chartId) {
    DataType dataType;
    const RECT* chartRect;
    if (chartId == 1) {
        dataType = uiModule->GetChart1DataType();
        chartRect = &layoutModule->GetChart1Rect();
    } else if (chartId == 2) {
        dataType = uiModule->GetChart2DataType();
        chartRect = &layoutModule->GetChart2Rect();
    } else {
        return; // Неверный ID графика
    }

    // Проверяем, является ли тип данных многоканальным, иначе нет чекбоксов
    bool isMultiChannel = false;
    if (dataType == DataType::PHASE_DIFFERENCE || dataType == DataType::AMPLITUDE_RATIO ||
        dataType == DataType::AMPLITUDE_RX1 || dataType == DataType::AMPLITUDE_RX2 ||
        dataType == DataType::PHASE_RX1 || dataType == DataType::PHASE_RX2) {
        isMultiChannel = true;
    }

    if (!isMultiChannel) {
        return; 
    }

    // Выполняем хит-тест, чтобы определить, какой чекбокс был нажат
    POINT clickPt = {x, y};
    int clickedTransmitterIdx = chartModule->HitTestLegendCheckbox(*chartRect, clickPt, dataType);

    if (clickedTransmitterIdx != -1) {
        if (chartId == 1) {
            bool currentEnabled = uiModule->GetChart1EnabledTransmitters()[clickedTransmitterIdx];
            uiModule->SetChart1TransmitterEnabled(clickedTransmitterIdx, !currentEnabled);
        } else {
            bool currentEnabled = uiModule->GetChart2EnabledTransmitters()[clickedTransmitterIdx];
            uiModule->SetChart2TransmitterEnabled(clickedTransmitterIdx, !currentEnabled);
        }
        InvalidateRect(hWnd, NULL, FALSE); // Перерисовываем окно
    }
}

// Обработчики событий мыши
void ChartManager::HandleMouseDown(HWND hWnd, int x, int y) {
    if (!dataModule->HasData())
        return;

    RECT chart1Rect = layoutModule->GetChart1Rect();
    RECT chart2Rect = layoutModule->GetChart2Rect();

    int clickedChart = GetChartAtPoint(x, y);

    if (clickedChart != 0) {
        activeChart = clickedChart;
        
        // Проверяем клик на легенду
        DataType dataType = (activeChart == 1) ? uiModule->GetChart1DataType() : uiModule->GetChart2DataType();
        bool isMultiChannel = (dataType == DataType::PHASE_DIFFERENCE || dataType == DataType::AMPLITUDE_RATIO ||
                               dataType == DataType::AMPLITUDE_RX1 || dataType == DataType::AMPLITUDE_RX2 ||
                               dataType == DataType::PHASE_RX1 || dataType == DataType::PHASE_RX2);

        if (isMultiChannel) {
            const RECT* currentChartRect = (activeChart == 1) ? &chart1Rect : &chart2Rect;
            POINT pt = {x, y};
            if (chartModule->HitTestLegendCheckbox(*currentChartRect, pt, dataType) != -1) {
                HandleLegendCheckboxClick(hWnd, x, y, activeChart);
                return;
            }
        }
    }

    isSelecting = true;
    selectionStart.x = x;
    selectionStart.y = y;
    selectionEnd.x = x;
    selectionEnd.y = y;

    SetCapture(hWnd);
}

void ChartManager::HandleMouseMove(HWND hWnd, int x, int y)
{
    if (!isSelecting)
        return;
    
    // Получаем контекст устройства для прямой отрисовки
    HDC hdc = GetDC(hWnd);
    if (!hdc)
        return;
    
    // Стираем предыдущий прямоугольник выделения (используем XOR для стирания)
    if (selectionEnd.x != selectionStart.x || selectionEnd.y != selectionStart.y) {
        RECT oldZoomRect;
        oldZoomRect.left = (std::min)(selectionStart.x, selectionEnd.x);
        oldZoomRect.top = (std::min)(selectionStart.y, selectionEnd.y);
        oldZoomRect.right = (std::max)(selectionStart.x, selectionEnd.x);
        oldZoomRect.bottom = (std::max)(selectionStart.y, selectionEnd.y);
        
        chartModule->DrawZoomRectangle(hdc, oldZoomRect);
    }
    
    // Обновляем конечную точку выделения
    selectionEnd.x = x;
    selectionEnd.y = y;
    
    // Рисуем новый прямоугольник выделения
    RECT newZoomRect;
    newZoomRect.left = (std::min)(selectionStart.x, selectionEnd.x);
    newZoomRect.top = (std::min)(selectionStart.y, selectionEnd.y);
    newZoomRect.right = (std::max)(selectionStart.x, selectionEnd.x);
    newZoomRect.bottom = (std::max)(selectionStart.y, selectionEnd.y);
    
    chartModule->DrawZoomRectangle(hdc, newZoomRect);
    
    ReleaseDC(hWnd, hdc);
}

void ChartManager::HandleMouseUp(HWND hWnd, int x, int y)
{
    if (!isSelecting)
        return;
    
    // Стираем последний прямоугольник выделения перед завершением
    HDC hdc = GetDC(hWnd);
    if (hdc) {
        RECT lastZoomRect;
        lastZoomRect.left = (std::min)(selectionStart.x, selectionEnd.x);
        lastZoomRect.top = (std::min)(selectionStart.y, selectionEnd.y);
        lastZoomRect.right = (std::max)(selectionStart.x, selectionEnd.x);
        lastZoomRect.bottom = (std::max)(selectionStart.y, selectionEnd.y);
        
        chartModule->DrawZoomRectangle(hdc, lastZoomRect);
        ReleaseDC(hWnd, hdc);
    }
    
    isSelecting = false;
    ReleaseCapture();
    
    selectionEnd.x = x;
    selectionEnd.y = y;
    
    // Проверяем, что выделена достаточно большая область
    int deltaX = abs(selectionEnd.x - selectionStart.x);
    int deltaY = abs(selectionEnd.y - selectionStart.y);
    
    if (deltaX > 10 && deltaY > 10) {
        // Преобразуем экранные координаты в координаты данных
        double minDataX, minDataY, maxDataX, maxDataY;
        
        ConvertScreenToDataCoords(activeChart, 
            (std::min)(selectionStart.x, selectionEnd.x), 
            (std::max)(selectionStart.y, selectionEnd.y), 
            minDataX, minDataY);
            
        ConvertScreenToDataCoords(activeChart, 
            (std::max)(selectionStart.x, selectionEnd.x), 
            (std::min)(selectionStart.y, selectionEnd.y), 
            maxDataX, maxDataY);
        
        // Устанавливаем новый видимый диапазон для соответствующего графика
        if (activeChart == 1) {
            chart1Zoom.minX = minDataX;
            chart1Zoom.maxX = maxDataX;
            chart1Zoom.minY = minDataY;
            chart1Zoom.maxY = maxDataY;
            chart1Zoom.isZoomed = true;
        } else if (activeChart == 2) {
            chart2Zoom.minX = minDataX;
            chart2Zoom.maxX = maxDataX;
            chart2Zoom.minY = minDataY;
            chart2Zoom.maxY = maxDataY;
            chart2Zoom.isZoomed = true;
        }
    }
    
    // Обновляем скроллбары после изменения зума
    UpdateScrollBars(hWnd);
    
    // Перерисовываем только измененный график (избегаем мигания)
    if (activeChart > 0) {
        InvalidateChart(hWnd, activeChart);
    } else {
        InvalidateRect(hWnd, NULL, TRUE); // Если не определен график, перерисовываем всё
    }
}

void ChartManager::HandleRightClick(HWND hWnd, int x, int y)
{
    // Определяем, на каком графике произошел клик
    int chartAtPoint = GetChartAtPoint(x, y);
    
    // Сбрасываем масштаб только для соответствующего графика
    if (chartAtPoint == 1) {
        chart1Zoom.isZoomed = false;
    } else if (chartAtPoint == 2) {
        chart2Zoom.isZoomed = false;
    }
    
    // Обновляем скроллбары после сброса зума
    UpdateScrollBars(hWnd);
    
    // Перерисовываем только измененный график (избегаем мигания)
    if (chartAtPoint > 0) {
        InvalidateChart(hWnd, chartAtPoint);
    } else {
        InvalidateRect(hWnd, NULL, TRUE); // Если не определен график, перерисовываем всё
    }
}

int ChartManager::GetChartAtPoint(int x, int y) const {
    RECT chart1Rect = layoutModule->GetChart1Rect();
    RECT chart2Rect = layoutModule->GetChart2Rect();

    if (PtInRect(&chart1Rect, {x, y}))
        return 1;
    if (uiModule->IsDualMode() && PtInRect(&chart2Rect, {x, y}))
        return 2;
    return 0;
}

void ChartManager::ConvertScreenToDataCoords(int chartNum, int screenX, int screenY, double& dataX, double& dataY) const {
    const RECT* chartRect = nullptr;
    const ZoomRange* zoom = nullptr;

    if (chartNum == 1) {
        chartRect = &layoutModule->GetChart1Rect();
        zoom = &chart1Zoom;
    } else if (chartNum == 2) {
        chartRect = &layoutModule->GetChart2Rect();
        zoom = &chart2Zoom;
    }

    if (!chartRect || !zoom || (chartRect->right - chartRect->left == 0) || (chartRect->bottom - chartRect->top == 0)) {
        dataX = 0.0;
        dataY = 0.0;
        return;
    }

    double plotWidth = static_cast<double>(chartRect->right - chartRect->left);
    double plotHeight = static_cast<double>(chartRect->bottom - chartRect->top);

    double xRatio = static_cast<double>(screenX - chartRect->left) / plotWidth;
    double yRatio = static_cast<double>(chartRect->bottom - screenY) / plotHeight; // Y-axis is inverted in screen coords

    if (zoom->isZoomed) {
        dataX = zoom->minX + xRatio * (zoom->maxX - zoom->minX);
        dataY = zoom->minY + yRatio * (zoom->maxY - zoom->minY);
    } else {
        // Если не зазумлено, используем полный диапазон данных (0 до data.size()-1)
        dataX = xRatio * (dataModule->GetDataSize() - 1);
        // Для Y-координаты без зума требуется определить глобальный мин/макс диапазон данных
        // В этой реализации это не предусмотрено, поэтому оставим заглушку
        dataY = yRatio; 
    }
}

// Методы для обработки скроллинга
void ChartManager::HandleScrollEvent(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    HWND hScrollBar = (HWND)lParam;
    int scrollCode = LOWORD(wParam);
    
    // Определяем, какой скроллбар был использован (только горизонтальные)
    int chartNumber = 0;
    
    if (hScrollBar == uiModule->GetHScrollBar(1)) {
        chartNumber = 1;
    } else if (hScrollBar == uiModule->GetHScrollBar(2)) {
        chartNumber = 2;
    } else {
        return; // Неизвестный скроллбар
    }
    
    // Получаем текущую информацию о скроллбаре
    SCROLLINFO si = {};
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_ALL;
    GetScrollInfo(hScrollBar, SB_CTL, &si);
    
    int newPos = si.nPos;
    
    // Обрабатываем различные команды скроллинга
    switch (scrollCode) {
        case SB_LINEUP:
            newPos = (std::max)(si.nMin, si.nPos - 1);
            break;
        case SB_LINEDOWN:
            newPos = (std::min)(si.nMax - (int)si.nPage + 1, si.nPos + 1);
            break;
        case SB_PAGEUP:
            newPos = (std::max)(si.nMin, si.nPos - (int)si.nPage);
            break;
        case SB_PAGEDOWN:
            newPos = (std::min)(si.nMax - (int)si.nPage + 1, si.nPos + (int)si.nPage);
            break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            newPos = si.nTrackPos;
            break;
        default:
            return;
    }
    
    // Обновляем позицию скроллбара
    si.nPos = newPos;
    SetScrollInfo(hScrollBar, SB_CTL, &si, TRUE);
    
    // Обновляем диапазон зума на основе новой позиции скроллбара (только горизонтальный)
    ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;
    
    // Горизонтальный скроллинг (ось X)
    double totalRange = static_cast<double>(dataModule->GetDataSize() - 1);
    double viewRange = zoom->maxX - zoom->minX;
    double newMinX = (newPos / 100.0) * (totalRange - viewRange);
    zoom->minX = newMinX;
    zoom->maxX = newMinX + viewRange;
    
    // Перерисовываем только нужный график (избегаем мигания)
    InvalidateChart(hWnd, chartNumber);
}

void ChartManager::UpdateScrollBars(HWND hWnd) {
    // Обновляем скроллбар для первого графика
    if (chart1Zoom.isZoomed) {
        uiModule->ShowScrollBars(1, true);
        uiModule->UpdateScrollBarPositions(1, layoutModule->GetChart1Rect());
        
        // Настраиваем диапазон горизонтального скроллбара
        double totalDataRange = static_cast<double>(dataModule->GetDataSize() - 1);
        double viewRangeX = chart1Zoom.maxX - chart1Zoom.minX;
        
        int hScrollPos = static_cast<int>((chart1Zoom.minX / (totalDataRange - viewRangeX)) * 100);
        uiModule->SetScrollBarRange(1, 0, 100, static_cast<int>((viewRangeX / totalDataRange) * 100), hScrollPos);
    } else {
        uiModule->ShowScrollBars(1, false);
    }
    
    // Обновляем скроллбар для второго графика (если в режиме двух графиков)
    if (uiModule->IsDualMode()) {
        if (chart2Zoom.isZoomed) {
            uiModule->ShowScrollBars(2, true);
            uiModule->UpdateScrollBarPositions(2, layoutModule->GetChart2Rect());
            
            // Настраиваем диапазон горизонтального скроллбара
            double totalDataRange = static_cast<double>(dataModule->GetDataSize() - 1);
            double viewRangeX = chart2Zoom.maxX - chart2Zoom.minX;
            
            int hScrollPos = static_cast<int>((chart2Zoom.minX / (totalDataRange - viewRangeX)) * 100);
            uiModule->SetScrollBarRange(2, 0, 100, static_cast<int>((viewRangeX / totalDataRange) * 100), hScrollPos);
        } else {
            uiModule->ShowScrollBars(2, false);
        }
    } else {
        uiModule->ShowScrollBars(2, false);
    }
}

// Метод для частичной перерисовки конкретного графика
void ChartManager::InvalidateChart(HWND hWnd, int chartNumber) {
    RECT chartRect;
    
    if (chartNumber == 1) {
        chartRect = layoutModule->GetChart1Rect();
    } else if (chartNumber == 2) {
        chartRect = layoutModule->GetChart2Rect();
    } else {
        return; // Неверный номер графика
    }
    
    // Перерисовываем только область конкретного графика
    InvalidateRect(hWnd, &chartRect, TRUE);
}