#include "framework.h"
#include "ChartManager.h"
#include "SchematicModule.h"
#include "CurveSelectionModule.h"
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
    , schematicModule(std::make_unique<SchematicModule>())
    , curveSelectionModule(std::make_unique<CurveSelectionModule>())
    , isSelecting(false)
    , activeChart(0)
{
    selectionStart.x = selectionStart.y = 0;
    selectionEnd.x = selectionEnd.y = 0;
    
    // Устанавливаем связь между модулями
    schematicModule->SetCurveSelectionModule(curveSelectionModule.get());
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
    
    // Проверяем, есть ли конфигурация кривых для первого графика
    if (IsUsingNewCurveSystem(1)) {
        // Используем новую систему кривых
        DrawChartWithCurves(hdc, chart1Rect, 1);
    } else {
        // Используем старую систему (для обратной совместимости)
        DrawChartLegacy(hdc, chart1Rect, 1);
    }
}

void ChartManager::DrawChart2(HDC hdc)
{
    if (!uiModule->IsDualMode())
        return;
        
    const RECT& chart2Rect = layoutModule->GetChart2Rect();
    
    // Проверяем, есть ли конфигурация кривых для второго графика
    if (IsUsingNewCurveSystem(2)) {
        // Используем новую систему кривых
        DrawChartWithCurves(hdc, chart2Rect, 2);
    } else {
        // Используем старую систему (для обратной совместимости)
        DrawChartLegacy(hdc, chart2Rect, 2);
    }
}

void ChartManager::LoadDataFromFile(const std::wstring& filename)
{
    dataModule->LoadDataFromFile(filename);
    
    // Сбрасываем масштабирование для обоих графиков при загрузке новых данных
    chart1Zoom.isZoomed = false;
    chart2Zoom.isZoomed = false;
    
    // Сохраняем конфигурации кривых при загрузке новых данных
    // Новая система должна продолжать работать с теми же настройками
    // но на новых данных
}

bool ChartManager::HandleCommand(WPARAM wParam, LPARAM lParam)
{
    HWND hWnd = GetParent((HWND)lParam);
    int wmId = LOWORD(wParam);
    bool needsRedraw = false;
    
    // Обработка команд от диалогового окна схемы прибора
    switch (wmId) {
        case IDC_CHART1_SCHEMATIC_BTN:
            ShowSchematicDialog(hWnd, 1);
            return true;
            
        case IDC_CHART2_SCHEMATIC_BTN:
            ShowSchematicDialog(hWnd, 2);
            return true;
            
        case IDC_APPLY_SETTINGS:
            // Команда применения настроек из диалогового окна схемы
            // Обработка происходит в самом диалоговом окне
            return true;
            
        case IDC_PRESET_ALL_AMP_R1:
        case IDC_PRESET_ALL_AMP_R2:
        case IDC_PRESET_ALL_PHASE_R1:
        case IDC_PRESET_ALL_PHASE_R2:
        case IDC_PRESET_RATIOS:
        case IDC_PRESET_DIFFERENCES:
            // Команды пресетов обрабатываются в диалоговом окне схемы
            return true;
            
        case IDC_COPY_FROM_CHART:
            // Команда копирования настроек обрабатывается в диалоговом окне схемы
            return true;
            
        case IDC_DUAL_MODE_CHECK:
            // Переключение режима двух графиков
            needsRedraw = uiModule->HandleCommand(wParam, lParam, hWnd);
            UpdateLayout(hWnd);
            return needsRedraw;
            
        case IDC_CHART1_DATA_COMBO:
        case IDC_CHART1_FREQ_COMBO:
        case IDC_CHART1_REFRESH_BTN:
            // Изменения в элементах управления первого графика
            needsRedraw = uiModule->HandleCommand(wParam, lParam, hWnd);
            HandleLegacyUIChange(hWnd, 1);
            break;
            
        case IDC_CHART2_DATA_COMBO:
        case IDC_CHART2_FREQ_COMBO:
        case IDC_CHART2_REFRESH_BTN:
            // Изменения в элементах управления второго графика
            needsRedraw = uiModule->HandleCommand(wParam, lParam, hWnd);
            HandleLegacyUIChange(hWnd, 2);
            break;
            
        default:
            // Передаем остальные команды в UIModule
            needsRedraw = uiModule->HandleCommand(wParam, lParam, hWnd);
            break;
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

// Метод для отображения схемы прибора
void ChartManager::ShowSchematicDialog(HWND hWnd, int chartNumber) {
    if (!schematicModule || !curveSelectionModule) {
        return;
    }
    
    // Получаем текущую конфигурацию графика
    ChartConfiguration config;
    if (curveSelectionModule->HasConfiguration(chartNumber)) {
        config = curveSelectionModule->GetChartConfiguration(chartNumber);
    }
    
    // Показываем диалоговое окно схемы прибора
    if (schematicModule->ShowSchematicDialog(hWnd, chartNumber, config)) {
        // Пользователь применил настройки - получаем новую конфигурацию
        ChartConfiguration newConfig = curveSelectionModule->GetChartConfiguration(chartNumber);
        
        // Применяем новую конфигурацию
        ApplyCurveConfiguration(chartNumber, newConfig);
        
        // Обновляем график с учетом изменений
        UpdateChartAfterConfigurationChange(hWnd, chartNumber);
    }
}

// Методы для новой системы кривых
void ChartManager::DrawChartWithCurves(HDC hdc, const RECT& chartRect, int chartNumber) {
    if (!curveSelectionModule || !dataModule) return;
    
    // Получаем конфигурацию кривых
    ChartConfiguration config = curveSelectionModule->GetChartConfiguration(chartNumber);
    if (config.curves.empty()) return;
    
    // Вычисляем данные для кривых
    std::vector<std::vector<float>> leftAxisData;
    std::vector<std::vector<float>> rightAxisData;
    std::vector<CurveDefinition> leftAxisCurves;
    std::vector<CurveDefinition> rightAxisCurves;
    
    curveSelectionModule->CalculateCurveData(config, dataModule.get(),
                                           leftAxisData, rightAxisData,
                                           leftAxisCurves, rightAxisCurves);
    
    // Формируем заголовок
    std::wstring title = L"Chart " + std::to_wstring(chartNumber) + L" (Custom Curves)";
    
    // Проверяем, нужны ли двойные оси
    if (config.hasDualAxis && !rightAxisData.empty()) {
        // Используем новый метод для двойных осей
        chartModule->DrawDualAxisChart(hdc, chartRect, config.curves,
                                     leftAxisData, rightAxisData,
                                     title, dataModule->HasData());
    } else {
        // Используем обычный метод для одной оси
        if (!leftAxisData.empty()) {
            // Преобразуем кривые в старый формат для совместимости
            DataType legacyType = DataType::PHASE_DIFFERENCE; // Заглушка
            std::vector<bool> enabledTransmitters = {true, true, true, true};
            
            // Применяем масштабирование если есть
            ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;
            bool showAxes = !zoom->isZoomed;
            
            chartModule->DrawChart(hdc, chartRect, leftAxisData, legacyType, title, 
                                 dataModule->HasData(), enabledTransmitters, dataModule.get(),
                                 zoom->isZoomed ? zoom->minX : 0.0,
                                 zoom->isZoomed ? zoom->maxX : static_cast<double>(dataModule->GetDataSize() > 0 ? dataModule->GetDataSize() - 1 : 0),
                                 zoom->isZoomed ? zoom->minY : -1e6, 
                                 zoom->isZoomed ? zoom->maxY : 1e6, showAxes);
        }
    }
}

void ChartManager::DrawChartLegacy(HDC hdc, const RECT& chartRect, int chartNumber) {
    // Старая система отрисовки для обратной совместимости
    std::vector<std::vector<float>> chartData;
    DataType dataType;
    FrequencyBand frequency;
    std::vector<bool> enabledTransmitters;
    ZoomRange* zoom;
    
    if (chartNumber == 1) {
        dataType = uiModule->GetChart1DataType();
        frequency = uiModule->GetChart1Frequency();
        enabledTransmitters = uiModule->GetChart1EnabledTransmitters();
        zoom = &chart1Zoom;
    } else {
        dataType = uiModule->GetChart2DataType();
        frequency = uiModule->GetChart2Frequency();
        enabledTransmitters = uiModule->GetChart2EnabledTransmitters();
        zoom = &chart2Zoom;
    }
    
    // Вычисляем данные
    if (zoom->isZoomed) {
        dataModule->CalculateChartData(dataType, frequency, chartData, 
                                      enabledTransmitters, zoom->minX, zoom->maxX, 
                                      zoom->minY, zoom->maxY);
    } else {
        dataModule->CalculateChartData(dataType, frequency, chartData, enabledTransmitters, 
                                      0.0, static_cast<double>(dataModule->GetDataSize() - 1), -1e6, 1e6);
    }
    
    // Формируем заголовок
    std::wstring title = BuildChartTitle(chartNumber, dataType, frequency);
    
    // Отрисовка графика
    bool showAxes = !zoom->isZoomed;
    chartModule->DrawChart(hdc, chartRect, chartData, dataType, title, dataModule->HasData(), 
                           enabledTransmitters, dataModule.get(),
                           zoom->isZoomed ? zoom->minX : 0.0,
                           zoom->isZoomed ? zoom->maxX : static_cast<double>(dataModule->GetDataSize() > 0 ? dataModule->GetDataSize() - 1 : 0),
                           zoom->isZoomed ? zoom->minY : -1e6, 
                           zoom->isZoomed ? zoom->maxY : 1e6, showAxes);
}

// Методы для работы с конфигурациями кривых
void ChartManager::ApplyCurveConfiguration(int chartNumber, const ChartConfiguration& config) {
    if (!curveSelectionModule) return;
    
    // Валидируем конфигурацию
    if (!ValidateChartConfiguration(config)) {
        return;
    }
    
    // Применяем конфигурацию
    curveSelectionModule->ApplyCurveConfiguration(chartNumber, config);
    
    // Синхронизируем UI с новой конфигурацией
    SynchronizeUIWithConfiguration(chartNumber);
}

ChartConfiguration ChartManager::GetChartConfiguration(int chartNumber) const {
    if (!curveSelectionModule) {
        return ChartConfiguration();
    }
    
    return curveSelectionModule->GetChartConfiguration(chartNumber);
}

bool ChartManager::HasCurveConfiguration(int chartNumber) const {
    if (!curveSelectionModule) {
        return false;
    }
    
    return curveSelectionModule->HasConfiguration(chartNumber);
}

// Методы для копирования настроек между графиками
void ChartManager::CopyChartSettings(int fromChart, int toChart) {
    if (!curveSelectionModule) return;
    
    // Проверяем валидность номеров графиков
    if (fromChart < 1 || fromChart > 2 || toChart < 1 || toChart > 2 || fromChart == toChart) {
        return;
    }
    
    // Проверяем, что исходный график имеет конфигурацию
    if (!curveSelectionModule->HasConfiguration(fromChart)) {
        return;
    }
    
    // Копируем настройки
    curveSelectionModule->CopySettings(fromChart, toChart);
    
    // Синхронизируем UI с новой конфигурацией
    SynchronizeUIWithConfiguration(toChart);
}

// Методы для применения пресетов
void ChartManager::ApplyPreset(int chartNumber, PresetType preset, FrequencyBand frequency) {
    if (!curveSelectionModule) return;
    
    // Применяем пресет
    curveSelectionModule->ApplyPreset(preset, chartNumber, frequency);
    
    // Синхронизируем UI с новой конфигурацией
    SynchronizeUIWithConfiguration(chartNumber);
}

// Вспомогательные методы для интеграции
void ChartManager::UpdateChartAfterConfigurationChange(HWND hWnd, int chartNumber) {
    // Сбрасываем масштабирование при изменении конфигурации
    if (chartNumber == 1) {
        chart1Zoom.isZoomed = false;
    } else if (chartNumber == 2) {
        chart2Zoom.isZoomed = false;
    }
    
    // Обновляем скроллбары
    UpdateScrollBars(hWnd);
    
    // Перерисовываем график
    InvalidateChart(hWnd, chartNumber);
}

void ChartManager::SynchronizeUIWithConfiguration(int chartNumber) {
    if (!curveSelectionModule) return;
    
    // Получаем конфигурацию
    ChartConfiguration config = curveSelectionModule->GetChartConfiguration(chartNumber);
    
    // Если конфигурация пуста, возвращаемся к стандартному UI
    if (config.curves.empty()) {
        return;
    }
    
    // Синхронизируем старые элементы UI с новой системой, если возможно
    SynchronizeLegacyUIWithNewSystem(chartNumber);
    
    // Новая система работает независимо от старых элементов UI,
    // но мы стараемся поддерживать их в синхронизированном состоянии
    // для лучшего пользовательского опыта
}

bool ChartManager::ValidateChartConfiguration(const ChartConfiguration& config) const {
    if (!curveSelectionModule) return false;
    
    // Используем валидацию из CurveSelectionModule
    return curveSelectionModule->ValidateConfiguration(config);
}

// Методы для обработки событий от диалогового окна схемы
void ChartManager::HandleSchematicDialogResult(HWND hWnd, int chartNumber, bool applied) {
    if (!applied) {
        return; // Пользователь отменил изменения
    }
    
    // Обновляем график после применения настроек
    UpdateChartAfterConfigurationChange(hWnd, chartNumber);
    
    // Отправляем пользовательское сообщение главному окну для обновления
    PostMessage(hWnd, WM_USER + 100, static_cast<WPARAM>(chartNumber), 0);
}

void ChartManager::HandlePresetCommand(HWND hWnd, int chartNumber, int presetId, FrequencyBand frequency) {
    PresetType preset;
    
    // Определяем тип пресета по ID команды
    switch (presetId) {
        case IDC_PRESET_ALL_AMP_R1:
            preset = PresetType::ALL_AMPLITUDES_R1;
            break;
        case IDC_PRESET_ALL_AMP_R2:
            preset = PresetType::ALL_AMPLITUDES_R2;
            break;
        case IDC_PRESET_ALL_PHASE_R1:
            preset = PresetType::ALL_PHASES_R1;
            break;
        case IDC_PRESET_ALL_PHASE_R2:
            preset = PresetType::ALL_PHASES_R2;
            break;
        case IDC_PRESET_RATIOS:
            preset = PresetType::AMPLITUDE_RATIOS;
            break;
        case IDC_PRESET_DIFFERENCES:
            preset = PresetType::PHASE_DIFFERENCES;
            break;
        default:
            return; // Неизвестный пресет
    }
    
    // Применяем пресет
    ApplyPreset(chartNumber, preset, frequency);
    
    // Обновляем график
    UpdateChartAfterConfigurationChange(hWnd, chartNumber);
}

void ChartManager::HandleCopySettingsCommand(HWND hWnd, int fromChart, int toChart) {
    // Копируем настройки между графиками
    CopyChartSettings(fromChart, toChart);
    
    // Обновляем целевой график
    UpdateChartAfterConfigurationChange(hWnd, toChart);
}

// Методы для обеспечения совместимости
void ChartManager::ResetToLegacyMode(int chartNumber) {
    if (!curveSelectionModule) return;
    
    // Очищаем конфигурацию кривых для возврата к старой системе
    ChartConfiguration emptyConfig;
    curveSelectionModule->ApplyCurveConfiguration(chartNumber, emptyConfig);
    
    // Синхронизируем UI
    SynchronizeUIWithConfiguration(chartNumber);
}

bool ChartManager::IsUsingNewCurveSystem(int chartNumber) const {
    if (!curveSelectionModule) return false;
    
    return curveSelectionModule->HasConfiguration(chartNumber);
}

void ChartManager::SynchronizeLegacyUIWithNewSystem(int chartNumber) {
    if (!IsUsingNewCurveSystem(chartNumber) || !uiModule) {
        return;
    }
    
    // Получаем конфигурацию новой системы
    ChartConfiguration config = curveSelectionModule->GetChartConfiguration(chartNumber);
    
    if (config.curves.empty()) {
        return;
    }
    
    // Пытаемся найти соответствие в старой системе
    // Если все кривые одного типа, можем синхронизировать комбобоксы
    DataType legacyType = DataType::PHASE_DIFFERENCE;
    FrequencyBand legacyFreq = FrequencyBand::FREQ_400_KHZ;
    bool canSync = true;
    
    if (!config.curves.empty()) {
        // Берем тип и частоту от первой кривой
        legacyType = config.curves[0].dataType;
        legacyFreq = config.curves[0].frequency;
        
        // Проверяем, что все кривые имеют одинаковый тип и частоту
        for (const auto& curve : config.curves) {
            if (curve.dataType != legacyType || curve.frequency != legacyFreq) {
                canSync = false;
                break;
            }
        }
    }
    
    // Если можем синхронизировать, обновляем старые элементы UI
    if (canSync) {
        if (chartNumber == 1) {
            uiModule->SetChart1DataType(legacyType);
            uiModule->SetChart1Frequency(legacyFreq);
        } else if (chartNumber == 2) {
            uiModule->SetChart2DataType(legacyType);
            uiModule->SetChart2Frequency(legacyFreq);
        }
    }
}

void ChartManager::HandleLegacyUIChange(HWND hWnd, int chartNumber) {
    // Если пользователь изменил настройки через старые элементы UI,
    // сбрасываем новую систему кривых для этого графика
    if (IsUsingNewCurveSystem(chartNumber)) {
        ResetToLegacyMode(chartNumber);
        
        // Обновляем график
        InvalidateChart(hWnd, chartNumber);
    }
}