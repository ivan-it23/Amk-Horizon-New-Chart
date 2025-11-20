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
    
    // Устанавливаем callback для обновления графика после применения настроек (v2.9.4)
    schematicModule->SetOnConfigurationAppliedCallback([this](int chartNumber) {
        // Получаем handle главного окна из родительского окна диалога
        HWND hSchematicDlg = schematicModule->GetSchematicDialogHandle();
        if (hSchematicDlg) {
            HWND hMainWnd = GetParent(hSchematicDlg);
            if (hMainWnd) {
                // Перерисовываем соответствующий график
                InvalidateChart(hMainWnd, chartNumber);
            }
        }
    });
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
    
    // Обновляем элементы управления X Range
    UpdateXRangeUI(hWnd);
    
    // Обновляем размер статусной строки
    uiModule->ResizeStatusBar();
}

void ChartManager::ToggleDualMode()
{
    uiModule->ToggleDualMode();
}

void ChartManager::RefreshChart1()
{
    // Метод оставлен для совместимости API, но больше ничего не делает
    // Настройки управляются через Device Schematic
}

void ChartManager::RefreshChart2()
{
    // Метод оставлен для совместимости API, но больше ничего не делает
    // Настройки управляются через Device Schematic
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
    
    // Обновляем информацию о файле в статусной строке
    UpdateStatusBarFileInfo();
    
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
            
        case IDC_DUAL_MODE_CHECK:
            // Переключение режима двух графиков
            needsRedraw = uiModule->HandleCommand(wParam, lParam, hWnd);
            UpdateLayout(hWnd);
            return needsRedraw;
            
        case IDC_XRANGE_APPLY_BTN:
            // Применение пользовательского диапазона X для Chart 1
            HandleXRangeApply(hWnd, 1);
            return true;
            
        case IDC_XRANGE_RESET_BTN:
            // Сброс диапазона X к полному для Chart 1
            HandleXRangeReset(hWnd, 1);
            return true;
            
        case IDC_XRANGE2_APPLY_BTN:
            // Применение пользовательского диапазона X для Chart 2
            HandleXRangeApply(hWnd, 2);
            return true;
            
        case IDC_XRANGE2_RESET_BTN:
            // Сброс диапазона X к полному для Chart 2
            HandleXRangeReset(hWnd, 2);
            return true;
            
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
    // Метод оставлен для совместимости API, но больше ничего не делает
    // Управление кривыми происходит только через Device Schematic
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
    
    // Стираем предыдущий прямоугольник выделения (используем R2_NOT для стирания)
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

// Метод для отображения схемы прибора (v2.9.4: немодальное окно)
void ChartManager::ShowSchematicDialog(HWND hWnd, int chartNumber) {
    if (!schematicModule || !curveSelectionModule) {
        return;
    }
    
    // Показываем немодальное диалоговое окно схемы прибора
    // Окно остаётся открытым после Apply, поэтому не нужно обрабатывать возвращаемое значение
    schematicModule->ShowSchematicDialog(hWnd, chartNumber);
    
    // Примечание: Применение конфигурации теперь происходит внутри SchematicModule
    // при нажатии кнопки Apply, и окно остаётся открытым для быстрого переключения кривых
}

// Методы для новой системы кривых
void ChartManager::DrawChartWithCurves(HDC hdc, const RECT& chartRect, int chartNumber) {
    if (!curveSelectionModule || !dataModule) return;
    
    // Получаем конфигурацию кривых
    ChartConfiguration config = curveSelectionModule->GetChartConfiguration(chartNumber);
    if (config.curves.empty()) return;
    
    // Получаем параметры zoom
    ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;
    
    // Вычисляем данные для кривых с учетом zoom
    std::vector<std::vector<float>> leftAxisData;
    std::vector<std::vector<float>> rightAxisData;
    std::vector<CurveDefinition> leftAxisCurves;
    std::vector<CurveDefinition> rightAxisCurves;
    
    if (zoom->isZoomed) {
        // Используем zoom параметры (только для X, Y автоматически по данным)
        curveSelectionModule->CalculateCurveData(config, dataModule.get(),
                                               leftAxisData, rightAxisData,
                                               leftAxisCurves, rightAxisCurves,
                                               zoom->minX, zoom->maxX, -1e6, 1e6);
    } else {
        // Используем полный диапазон данных
        curveSelectionModule->CalculateCurveData(config, dataModule.get(),
                                               leftAxisData, rightAxisData,
                                               leftAxisCurves, rightAxisCurves);
    }
    
    // Формируем заголовок с частотой
    std::wstring title = L"Chart " + std::to_wstring(chartNumber);
    
    // Добавляем частоту, если есть кривые
    if (!config.curves.empty()) {
        FrequencyBand frequency = config.curves[0].frequency;
        title += (frequency == FrequencyBand::FREQ_400_KHZ) ? L" (400 kHz)" : L" (2000 kHz)";
    }
    
    // Проверяем, нужны ли двойные оси
    if (config.hasDualAxis && !rightAxisData.empty()) {
        // Используем новый метод для двойных осей с поддержкой zoom
        // Всегда показываем оси (включая ось X)
        bool showAxes = true;
        chartModule->DrawDualAxisChart(hdc, chartRect, config.curves,
                                     leftAxisData, rightAxisData,
                                     title, dataModule->HasData(),
                                     zoom->isZoomed ? zoom->minX : 0.0,
                                     zoom->isZoomed ? zoom->maxX : static_cast<double>(dataModule->GetDataSize() > 0 ? dataModule->GetDataSize() - 1 : 0),
                                     -1e6, 1e6, showAxes); // Y всегда автоматически
    } else {
        // Используем обычный метод для одной оси
        if (!leftAxisData.empty()) {
            // Отрисовываем график с кастомными кривыми
            // Используем метод DrawDualAxisChart даже для одной оси, чтобы правильно отобразить легенду
            bool showAxes = true;
            chartModule->DrawDualAxisChart(hdc, chartRect, leftAxisCurves,
                                         leftAxisData, std::vector<std::vector<float>>(), // Пустая правая ось
                                         title, dataModule->HasData(),
                                         zoom->isZoomed ? zoom->minX : 0.0,
                                         zoom->isZoomed ? zoom->maxX : static_cast<double>(dataModule->GetDataSize() > 0 ? dataModule->GetDataSize() - 1 : 0),
                                         -1e6, 1e6, showAxes);
        }
    }
}

void ChartManager::DrawChartLegacy(HDC hdc, const RECT& chartRect, int chartNumber) {
    // Проверяем, загружены ли данные
    bool hasData = dataModule && dataModule->HasData();
    
    std::wstring title = L"Chart " + std::to_wstring(chartNumber);
    std::vector<std::vector<float>> emptyData;
    std::vector<bool> emptyTransmitters(4, false);
    
    // Отрисовываем фон и рамку графика
    // Если данных нет, не показываем сетку (showAxes = false)
    // Если данные есть, показываем сетку (showAxes = true)
    chartModule->DrawChart(hdc, chartRect, emptyData, DataType::PHASE_DIFFERENCE, 
                          title, false, emptyTransmitters, dataModule.get(),
                          0.0, 1.0, -1.0, 1.0, hasData);
    
    // Рисуем текст с инструкцией
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(100, 100, 100));
    HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    
    // Выбираем сообщение в зависимости от наличия данных
    std::wstring message;
    if (hasData) {
        message = L"Click 'Device Schematic' to configure this chart";
    } else {
        message = L"Please open a data file first";
    }
    
    RECT textRect = chartRect;
    DrawText(hdc, message.c_str(), -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
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
    // Метод оставлен для совместимости API, но больше ничего не делает
    // Все управляется только через Device Schematic
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

// Методы для обработки X Range
void ChartManager::HandleXRangeApply(HWND hWnd, int chartNumber) {
    if (!dataModule || !dataModule->HasData()) {
        wchar_t message[256];
        swprintf_s(message, L"No data loaded for Chart %d", chartNumber);
        MessageBox(hWnd, message, L"X Range", MB_OK | MB_ICONWARNING);
        return;
    }
    
    // Получаем значения из полей ввода для указанного графика
    double fromX, toX;
    uiModule->GetXRangeValues(chartNumber, fromX, toX);
    
    // Валидация входных данных
    if (fromX >= toX) {
        wchar_t message[256];
        swprintf_s(message, L"Chart %d: 'From' value must be less than 'To' value", chartNumber);
        MessageBox(hWnd, message, L"X Range Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    double dataSize = static_cast<double>(dataModule->GetDataSize() - 1);
    if (fromX < 0 || toX > dataSize || fromX > dataSize || toX < 0) {
        wchar_t message[256];
        swprintf_s(message, L"Chart %d: X Range must be between 0 and %.1f", chartNumber, dataSize);
        MessageBox(hWnd, message, L"X Range Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Применяем диапазон к указанному графику
    ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;
    zoom->minX = fromX;
    zoom->maxX = toX;
    zoom->minY = -1e6; // Y автоматически по данным
    zoom->maxY = 1e6;
    zoom->isZoomed = true;
    
    // Обновляем скроллбары
    UpdateScrollBars(hWnd);
    
    // Обновляем UI
    UpdateXRangeUI(hWnd);
    
    // Перерисовываем графики
    InvalidateRect(hWnd, NULL, TRUE);
}

void ChartManager::HandleXRangeReset(HWND hWnd, int chartNumber) {
    // Сбрасываем масштабирование для указанного графика
    ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;
    zoom->isZoomed = false;
    
    // Обновляем скроллбары
    UpdateScrollBars(hWnd);
    
    // Обновляем UI
    UpdateXRangeUI(hWnd);
    
    // Перерисовываем графики
    InvalidateRect(hWnd, NULL, TRUE);
}

void ChartManager::UpdateXRangeUI(HWND hWnd) {
    if (!dataModule || !dataModule->HasData()) {
        uiModule->ResetXRangeControls(1);
        uiModule->ResetXRangeControls(2);
        return;
    }
    
    // Обновляем элементы управления для Chart 1
    double minX1, maxX1;
    bool isCustomRange1 = chart1Zoom.isZoomed;
    
    if (isCustomRange1) {
        minX1 = chart1Zoom.minX;
        maxX1 = chart1Zoom.maxX;
    } else {
        minX1 = 0.0;
        maxX1 = static_cast<double>(dataModule->GetDataSize() - 1);
    }
    
    uiModule->UpdateXRangeControls(1, minX1, maxX1, isCustomRange1);
    
    // Обновляем элементы управления для Chart 2 (если в Dual Mode)
    if (uiModule->IsDualMode()) {
        double minX2, maxX2;
        bool isCustomRange2 = chart2Zoom.isZoomed;
        
        if (isCustomRange2) {
            minX2 = chart2Zoom.minX;
            maxX2 = chart2Zoom.maxX;
        } else {
            minX2 = 0.0;
            maxX2 = static_cast<double>(dataModule->GetDataSize() - 1);
        }
        
        uiModule->UpdateXRangeControls(2, minX2, maxX2, isCustomRange2);
    }
}

void ChartManager::HandleSaveRange(HWND hWnd, int chartNumber) {
    if (!dataModule || !dataModule->HasData()) {
        MessageBox(hWnd, L"No data loaded!", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Получаем текущий диапазон для указанного графика
    ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;
    
    if (!zoom->isZoomed) {
        MessageBox(hWnd, L"Please set X Range first using Apply button!", L"Information", MB_OK | MB_ICONINFORMATION);
        return;
    }
    
    // Получаем индексы начала и конца диапазона
    size_t startIndex = static_cast<size_t>(zoom->minX);
    size_t endIndex = static_cast<size_t>(zoom->maxX);
    
    // Проверяем корректность индексов
    if (startIndex >= dataModule->GetDataSize() || endIndex >= dataModule->GetDataSize()) {
        MessageBox(hWnd, L"Invalid range indices!", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Открываем диалог сохранения файла
    OPENFILENAME ofn;
    wchar_t szFile[260] = {0};
    
    // Формируем предложенное имя файла
    // Формат: [исходное_имя]_range_[начало]_[конец]_[ДД_ММ_ГГГГ].bin
    std::wstring baseFileName = dataModule->GetLoadedFileName();
    
    // Получаем текущую дату
    SYSTEMTIME st;
    GetLocalTime(&st);
    
    // Формируем имя файла с датой в формате ДД_ММ_ГГГГ
    if (!baseFileName.empty()) {
        swprintf_s(szFile, L"%s_range_%zu_%zu_%02d_%02d_%04d.bin", 
                  baseFileName.c_str(), startIndex, endIndex,
                  st.wDay, st.wMonth, st.wYear);
    } else {
        // Если имя исходного файла не сохранено (не должно происходить)
        swprintf_s(szFile, L"data_range_%zu_%zu_%02d_%02d_%04d.bin", 
                  startIndex, endIndex,
                  st.wDay, st.wMonth, st.wYear);
    }
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
    ofn.lpstrFilter = L"Binary Files (*.bin)\0*.bin\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = L"Save Data Range";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    
    if (GetSaveFileName(&ofn) == TRUE) {
        // Сохраняем диапазон данных
        if (dataModule->SaveDataRange(szFile, startIndex, endIndex)) {
            // Формируем сообщение об успехе
            wchar_t message[512];
            size_t recordCount = endIndex - startIndex + 1;
            swprintf_s(message, L"Successfully saved %zu records (indices %zu to %zu) to file:\n%s", 
                      recordCount, startIndex, endIndex, szFile);
            MessageBox(hWnd, message, L"Success", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBox(hWnd, L"Failed to save data range!", L"Error", MB_OK | MB_ICONERROR);
        }
    }
}

// Метод для обработки движения мыши и обновления координат в статусной строке
void ChartManager::HandleMouseMoveForCoordinates(HWND hWnd, int x, int y) {
    if (!dataModule || !dataModule->HasData()) {
        uiModule->ClearStatusBarCoordinates();
        return;
    }

    // Определяем, над каким графиком находится курсор
    int chartNum = GetChartAtPoint(x, y);
    
    if (chartNum == 0) {
        // Курсор вне графиков
        uiModule->ClearStatusBarCoordinates();
        return;
    }

    // Преобразуем экранные координаты в координаты данных
    double dataX, dataY1, dataY2;
    std::wstring y1Unit, y2Unit;
    bool hasDualAxis;
    
    if (ConvertScreenToChartCoords(chartNum, x, y, dataX, dataY1, dataY2, y1Unit, y2Unit, hasDualAxis)) {
        // Обновляем статусную строку с координатами
        uiModule->UpdateStatusBarCoordinates(dataX, dataY1, y1Unit, dataY2, y2Unit, hasDualAxis);
        
        // Обновляем информацию о файле в последней части статусной строки
        std::wstring fileName = dataModule->GetLoadedFileName();
        if (!fileName.empty()) {
            size_t dataSize = dataModule->GetDataSize();
            wchar_t fileInfo[256];
            swprintf_s(fileInfo, L"Points: %zu  |  %s", dataSize, fileName.c_str());
            SendMessage(uiModule->GetStatusBar(), SB_SETTEXT, 3, (LPARAM)fileInfo);
        }
    } else {
        uiModule->ClearStatusBarCoordinates();
    }
}

// Преобразование экранных координат в координаты графика
bool ChartManager::ConvertScreenToChartCoords(int chartNum, int screenX, int screenY,
                                              double& dataX, double& dataY1, double& dataY2,
                                              std::wstring& y1Unit, std::wstring& y2Unit,
                                              bool& hasDualAxis) const {
    if (!dataModule || !dataModule->HasData()) {
        return false;
    }

    // Получаем область графика
    RECT chartRect = (chartNum == 1) ? layoutModule->GetChart1Rect() : layoutModule->GetChart2Rect();
    
    // Проверяем, что курсор внутри области графика
    if (screenX < chartRect.left || screenX > chartRect.right ||
        screenY < chartRect.top || screenY > chartRect.bottom) {
        return false;
    }

    // Определяем, используется ли новая система кривых
    bool usingNewSystem = IsUsingNewCurveSystem(chartNum);
    
    if (usingNewSystem) {
        // Новая система кривых с возможными двойными осями
        ChartConfiguration config = GetChartConfiguration(chartNum);
        
        if (config.curves.empty()) {
            return false;
        }

        // Получаем данные для кривых
        std::vector<std::vector<float>> leftAxisData, rightAxisData;
        std::vector<CurveDefinition> leftAxisCurves, rightAxisCurves;
        curveSelectionModule->CalculateCurveData(config, dataModule.get(), leftAxisData, rightAxisData, leftAxisCurves, rightAxisCurves);
        
        // Определяем диапазоны данных
        double minX, maxX, minY1, maxY1, minY2, maxY2;
        chartModule->GetDataRanges(leftAxisData, rightAxisData, minX, maxX, minY1, maxY1, minY2, maxY2);
        
        // Применяем zoom если есть
        ZoomRange zoom = (chartNum == 1) ? chart1Zoom : chart2Zoom;
        if (zoom.isZoomed) {
            minX = zoom.minX;
            maxX = zoom.maxX;
            minY1 = zoom.minY;
            maxY1 = zoom.maxY;
        }
        
        // Преобразуем экранные координаты в координаты данных
        const int MARGIN_LEFT = 50;
        const int MARGIN_RIGHT = 50;
        const int MARGIN_TOP = 60;
        const int MARGIN_BOTTOM = 40;
        
        int plotWidth = chartRect.right - chartRect.left - MARGIN_LEFT - MARGIN_RIGHT;
        int plotHeight = chartRect.bottom - chartRect.top - MARGIN_TOP - MARGIN_BOTTOM;
        
        int relX = screenX - chartRect.left - MARGIN_LEFT;
        int relY = screenY - chartRect.top - MARGIN_TOP;
        
        // X координата
        dataX = minX + (double)relX / plotWidth * (maxX - minX);
        
        // Y координаты
        hasDualAxis = !rightAxisData.empty();
        
        if (hasDualAxis) {
            // Двойные оси
            dataY1 = maxY1 - (double)relY / plotHeight * (maxY1 - minY1); // Левая ось
            dataY2 = maxY2 - (double)relY / plotHeight * (maxY2 - minY2); // Правая ось (пропорционально)
            
            // Определяем единицы измерения из первых кривых каждой оси
            // Преобразуем std::string в std::wstring
            std::string units1 = config.curves[0].units;
            y1Unit = std::wstring(units1.begin(), units1.end());
            
            for (const auto& curve : config.curves) {
                if (curve.yAxisGroup == 1) {
                    std::string units2 = curve.units;
                    y2Unit = std::wstring(units2.begin(), units2.end());
                    break;
                }
            }
        } else {
            // Одна ось
            dataY1 = maxY1 - (double)relY / plotHeight * (maxY1 - minY1);
            std::string units1 = config.curves[0].units;
            y1Unit = std::wstring(units1.begin(), units1.end());
            y2Unit = L"";
        }
        
        return true;
        
    } else {
        // Старая система (legacy mode)
        // Здесь можно добавить поддержку старой системы, если нужно
        // Пока возвращаем false
        return false;
    }
}

// Получение единиц измерения для типа данных
std::wstring ChartManager::GetUnitForDataType(DataType dataType) const {
    switch (dataType) {
        case DataType::PHASE_DIFFERENCE:
        case DataType::PHASE_RX1:
        case DataType::PHASE_RX2:
            return L"mdeg";
        
        case DataType::AMPLITUDE_RX1:
        case DataType::AMPLITUDE_RX2:
        case DataType::SILENT_AMPLITUDE_RX1:
        case DataType::SILENT_AMPLITUDE_RX2:
            return L"mV";
        
        case DataType::AMPLITUDE_RATIO:
            return L"V/V";
        
        case DataType::TEMPERATURE:
            return L"°C";
        
        default:
            return L"";
    }
}

// Обновление информации о файле в статусной строке
void ChartManager::UpdateStatusBarFileInfo() {
    if (!uiModule) return;
    
    if (dataModule && dataModule->HasData()) {
        std::wstring fileName = dataModule->GetLoadedFileName();
        size_t dataSize = dataModule->GetDataSize();
        
        wchar_t fileInfo[256];
        swprintf_s(fileInfo, L"Points: %zu  |  %s", dataSize, fileName.c_str());
        SendMessage(uiModule->GetStatusBar(), SB_SETTEXT, 3, (LPARAM)fileInfo);
        
        // Устанавливаем начальное сообщение
        uiModule->UpdateStatusBar(L"Ready");
    } else {
        SendMessage(uiModule->GetStatusBar(), SB_SETTEXT, 3, (LPARAM)L"No file loaded");
        uiModule->ClearStatusBarCoordinates();
    }
}
