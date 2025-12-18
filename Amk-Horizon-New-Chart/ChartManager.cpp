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
    , serialModule(std::make_unique<SerialModule>())  // v2.13.0
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
                // Сбрасываем X Range при смене функции/пресета
                if (dataModule && dataModule->HasData()) {
                    double maxX = static_cast<double>(dataModule->GetDataSize() - 1);
                    uiModule->UpdateXRangeControls(chartNumber, 0.0, maxX, false);
                    
                    // Сбрасываем zoom для соответствующего графика
                    if (chartNumber == 1) {
                        chart1Zoom.isZoomed = false;
                        chart1ZoomHistory.clear(); // v2.12.5: Очищаем историю zoom
                    } else if (chartNumber == 2) {
                        chart2Zoom.isZoomed = false;
                        chart2ZoomHistory.clear(); // v2.12.5: Очищаем историю zoom
                    }
                }
                
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
    
    // v2.13.0: Устанавливаем callback для получения данных с прибора
    // Элементы управления COM-портом теперь в меню "Прибор" и диалоге IDD_SERIAL_STATUS
    serialModule->SetDataCallback([this, hWnd](const GP_TOOL_DATA& data) {
        // Добавляем данные в DataModule
        dataModule->AddData(data);
        
        // v2.14.0: Обновляем диапазон для режима Surf
        if (dataModule->HasData()) {
            size_t dataSize = dataModule->GetDataSize();
            double maxX = static_cast<double>(dataSize - 1);
            
            // Обработка Surf для Chart 1
            if (uiModule->IsSurfEnabled(1)) {
                // Используем UpdateSurfRange для корректного вычисления Y диапазона
                UpdateSurfRange(1);
            } else {
                uiModule->UpdateXRangeControls(1, 0.0, maxX, false);
            }
            
            // Обработка Surf для Chart 2 (если Dual Mode)
            if (uiModule->IsDualMode()) {
                if (uiModule->IsSurfEnabled(2)) {
                    // Используем UpdateSurfRange для корректного вычисления Y диапазона
                    UpdateSurfRange(2);
                } else {
                    uiModule->UpdateXRangeControls(2, 0.0, maxX, false);
                }
            }
            
            // Обновляем статусную строку с информацией о Surf
            UpdateSurfStatusBar();
        }
        
        // Перерисовываем графики
        InvalidateChart(hWnd, 1);
        if (uiModule->IsDualMode()) {
            InvalidateChart(hWnd, 2);
        }
    });
    
    // Устанавливаем callback для изменения состояния подключения
    serialModule->SetStateCallback([this, hWnd](ConnectionState state, const std::wstring& error) {
        if (state == ConnectionState::CONNECTION_ERROR && !error.empty()) {
            MessageBox(hWnd, error.c_str(), L"Ошибка COM-порта", MB_OK | MB_ICONERROR);
        }
    });
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
        // Позиционируем элементы координат к правому краю для обоих графиков
        uiModule->PositionCursorControls(width, panel2Y);
    } else {
        // Позиционируем элементы координат к правому краю только для Chart 1
        uiModule->PositionCursorControls(width, 0);
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
    
    // Отрисовываем перекрестие поверх графика
    DrawCrosshair(hdc, chart1Rect, 1);
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
    
    // Отрисовываем перекрестие поверх графика
    DrawCrosshair(hdc, chart2Rect, 2);
}

void ChartManager::LoadDataFromFile(const std::wstring& filename)
{
    // v2.14.1: При открытии файла останавливаем опрос COM-порта
    // Режимы работы с файлом и COM-портом взаимоисключающие
    if (serialModule && serialModule->IsPolling()) {
        serialModule->StopPolling();
    }
    if (serialModule && serialModule->IsConnected()) {
        serialModule->Disconnect();
    }
    
    dataModule->LoadDataFromFile(filename);
    
    // Обновляем информацию о файле в статусной строке
    UpdateStatusBarFileInfo();
    
    // Сбрасываем масштабирование для обоих графиков при загрузке новых данных
    chart1Zoom.isZoomed = false;
    chart2Zoom.isZoomed = false;
    
    // v2.12.5: Очищаем историю zoom при загрузке нового файла
    chart1ZoomHistory.clear();
    chart2ZoomHistory.clear();
    
    // v2.14.0: Обработка режима Surf при загрузке файла
    if (dataModule->HasData()) {
        double maxX = static_cast<double>(dataModule->GetDataSize() - 1);
        
        // Если Surf включён для Chart 1 - показываем последние N точек
        if (uiModule->IsSurfEnabled(1)) {
            UpdateSurfRange(1);
        } else {
            uiModule->UpdateXRangeControls(1, 0.0, maxX, false);
        }
        
        // Если Surf включён для Chart 2 - показываем последние N точек
        if (uiModule->IsSurfEnabled(2)) {
            UpdateSurfRange(2);
        } else {
            uiModule->UpdateXRangeControls(2, 0.0, maxX, false);
        }
        
        // Обновляем статусную строку с информацией о Surf
        UpdateSurfStatusBar();
    }
    
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
            
        case IDC_RESET_ZOOM1_BTN:
            // Сброс zoom для Chart 1 (аналогично правому клику мыши)
            HandleResetZoom(hWnd, 1);
            return true;
            
        case IDC_RESET_ZOOM2_BTN:
            // Сброс zoom для Chart 2 (аналогично правому клику мыши)
            HandleResetZoom(hWnd, 2);
            return true;
            
        // === Команды COM-порта (v2.13.0) ===
        case IDC_SERIAL_CONNECT_BTN:
            if (serialModule->IsConnected()) {
                HandleSerialDisconnect(hWnd);
            } else {
                HandleSerialConnect(hWnd);
            }
            return true;
            
        case IDC_SERIAL_START_BTN:
            HandleSerialStart(hWnd);
            return true;
            
        case IDC_SERIAL_STOP_BTN:
            HandleSerialStop(hWnd);
            return true;
            
        case IDC_SERIAL_CLEAR_BTN:
            HandleSerialClear(hWnd);
            return true;
            
        // === Команды режима Surf (v2.14.0) ===
        case IDC_CHART1_SURF_CHECK:
            HandleSurfToggle(hWnd, 1);
            return true;
            
        case IDC_CHART2_SURF_CHECK:
            HandleSurfToggle(hWnd, 2);
            return true;
            
        case IDC_CHART1_SURF_EDIT:
            // При изменении размера окна Surf для Chart 1
            if (HIWORD(wParam) == EN_CHANGE && uiModule->IsSurfEnabled(1)) {
                UpdateSurfRange(1);
                UpdateSurfStatusBar();
                InvalidateChart(hWnd, 1);
            }
            return true;
            
        case IDC_CHART2_SURF_EDIT:
            // При изменении размера окна Surf для Chart 2
            if (HIWORD(wParam) == EN_CHANGE && uiModule->IsSurfEnabled(2)) {
                UpdateSurfRange(2);
                UpdateSurfStatusBar();
                InvalidateChart(hWnd, 2);
            }
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
        
        // Обновляем координаты курсора при клике на график (v2.15.1 - поддержка двойных осей)
        double dataX, dataY1, dataY2;
        std::wstring y1Unit, y2Unit;
        bool hasDualAxis;
        
        if (ConvertScreenToChartCoords(clickedChart, x, y, dataX, dataY1, dataY2, y1Unit, y2Unit, hasDualAxis)) {
            uiModule->UpdateClickCoordinates(clickedChart, dataX, dataY1, y1Unit, dataY2, y2Unit, hasDualAxis);
        }
        
        // Сохраняем позицию перекрестия для отображения
        CrosshairPosition* crosshair = (clickedChart == 1) ? &chart1Crosshair : &chart2Crosshair;
        crosshair->screenX = x;
        crosshair->screenY = y;
        crosshair->dataX = dataX;
        crosshair->dataY = dataY1;
        crosshair->isVisible = true;
        crosshair->chartNumber = clickedChart;
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
        // Это zoom — скрываем перекрестие
        if (activeChart == 1) {
            chart1Crosshair.isVisible = false;
        } else if (activeChart == 2) {
            chart2Crosshair.isVisible = false;
        }
        
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
        
        // Проверяем минимальный диапазон для X: подписи должны быть целыми и отличаться минимум на 10
        const double MIN_X_RANGE = 10.0;
        double xRange = maxDataX - minDataX;
        
        if (xRange < MIN_X_RANGE) {
            // Диапазон X слишком мал
            MessageBox(hWnd, 
                L"Cannot zoom further: X-axis labels must be integers with minimum spacing of 1.\nMinimum range is 10 points.",
                L"Zoom Limit Reached", 
                MB_OK | MB_ICONINFORMATION);
        } else {
            // v2.14.0: Ограничиваем zoom областью Surf, если режим Surf активен
            if (uiModule->IsSurfEnabled(activeChart)) {
                size_t dataSize = dataModule->GetDataSize();
                int surfWindow = uiModule->GetSurfWindowSize(activeChart);
                double surfMinX = (dataSize > static_cast<size_t>(surfWindow)) 
                    ? static_cast<double>(dataSize - surfWindow) : 0.0;
                double surfMaxX = static_cast<double>(dataSize - 1);
                
                // Ограничиваем zoom областью Surf
                minDataX = (std::max)(minDataX, surfMinX);
                maxDataX = (std::min)(maxDataX, surfMaxX);
                
                // Проверяем, что диапазон всё ещё достаточен
                if (maxDataX - minDataX < MIN_X_RANGE) {
                    MessageBox(hWnd, 
                        L"Cannot zoom further within Surf area.\nMinimum range is 10 points.",
                        L"Zoom Limit Reached", 
                        MB_OK | MB_ICONINFORMATION);
                    return;
                }
            }
            
            // Используем Y диапазон из выделенной области мышью
            // Это позволяет пользователю точно выбрать интересующий участок по Y
            // и затем использовать вертикальный scrollbar для навигации
            
            // v2.12.5: Сохраняем текущее состояние в историю перед применением нового zoom
            // Устанавливаем новый видимый диапазон для соответствующего графика
            if (activeChart == 1) {
                // Сохраняем текущее состояние в историю
                chart1ZoomHistory.push_back(chart1Zoom);
                
                chart1Zoom.minX = minDataX;
                chart1Zoom.maxX = maxDataX;
                chart1Zoom.minY = minDataY;
                chart1Zoom.maxY = maxDataY;
                chart1Zoom.isZoomed = true;
            } else if (activeChart == 2) {
                // Сохраняем текущее состояние в историю
                chart2ZoomHistory.push_back(chart2Zoom);
                
                chart2Zoom.minX = minDataX;
                chart2Zoom.maxX = maxDataX;
                chart2Zoom.minY = minDataY;
                chart2Zoom.maxY = maxDataY;
                chart2Zoom.isZoomed = true;
            }
        }
    }
    
    // Обновляем скроллбары после изменения зума
    UpdateScrollBars(hWnd);
    
    // Перерисовываем только измененный график (избегаем мигания)
    if (activeChart > 0) {
        InvalidateChart(hWnd, activeChart);
    } else {
        InvalidateRect(hWnd, NULL, FALSE); // Если не определен график, перерисовываем всё (FALSE для double buffering)
    }
}

void ChartManager::HandleRightClick(HWND hWnd, int x, int y)
{
    // Определяем, на каком графике произошел клик
    int chartAtPoint = GetChartAtPoint(x, y);
    
    // v2.12.5: Пошаговый откат zoom из истории
    // Если есть история — откатываемся на один шаг назад
    // Если история пуста — сбрасываем к полному виду
    if (chartAtPoint == 1) {
        chart1Crosshair.isVisible = false;
        
        if (!chart1ZoomHistory.empty()) {
            // Откатываемся к предыдущему состоянию из истории
            chart1Zoom = chart1ZoomHistory.back();
            chart1ZoomHistory.pop_back();
        } else {
            // История пуста — полный сброс zoom
            chart1Zoom.isZoomed = false;
        }
    } else if (chartAtPoint == 2) {
        chart2Crosshair.isVisible = false;
        
        if (!chart2ZoomHistory.empty()) {
            // Откатываемся к предыдущему состоянию из истории
            chart2Zoom = chart2ZoomHistory.back();
            chart2ZoomHistory.pop_back();
        } else {
            // История пуста — полный сброс zoom
            chart2Zoom.isZoomed = false;
        }
    }
    
    // Обновляем скроллбары после изменения зума
    UpdateScrollBars(hWnd);
    
    // Перерисовываем только измененный график (избегаем мигания)
    if (chartAtPoint > 0) {
        InvalidateChart(hWnd, chartAtPoint);
    } else {
        InvalidateRect(hWnd, NULL, FALSE); // Если не определен график, перерисовываем всё (FALSE для double buffering)
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

    // Получаем конфигурацию графика для определения типа оси X
    ChartConfiguration config = curveSelectionModule->GetChartConfiguration(chartNum);
    
    // Вычисляем область графика (plotRect) с учётом отступов
    // ВАЖНО: Всегда используем отступы для двойных осей, потому что DrawDualAxisChart
    // вызывается для всех графиков (даже с одной осью) и использует CalculateDualAxisPlotRect
    RECT plotRect;
    plotRect.left = chartRect->left + 60;
    plotRect.top = chartRect->top + 45;
    plotRect.right = chartRect->right - 140;
    plotRect.bottom = chartRect->bottom - 50;

    double plotWidth = static_cast<double>(plotRect.right - plotRect.left);
    double plotHeight = static_cast<double>(plotRect.bottom - plotRect.top);

    double xRatio = static_cast<double>(screenX - plotRect.left) / plotWidth;
    double yRatio = static_cast<double>(plotRect.bottom - screenY) / plotHeight; // Y-axis is inverted in screen coords

    // v2.10.0: Для кроссплота (температура на оси X) нужно преобразовать координаты по-другому
    if (config.xAxisType == XAxisType::TEMPERATURE) {
        // В режиме кроссплота экранная X координата соответствует температуре
        // Но zoom всё равно работает с индексами кадров
        
        // Получаем температурные данные
        std::vector<float> temperatureData;
        if (zoom->isZoomed) {
            dataModule->GetTemperatureData(temperatureData, zoom->minX, zoom->maxX);
        } else {
            dataModule->GetTemperatureData(temperatureData);
        }
        
        if (!temperatureData.empty()) {
            // Находим диапазон температур
            size_t startIdx = zoom->isZoomed ? static_cast<size_t>(zoom->minX) : 0;
            size_t endIdx = zoom->isZoomed ? static_cast<size_t>(zoom->maxX + 1) : temperatureData.size();
            
            if (startIdx < endIdx && endIdx <= temperatureData.size()) {
                float minTemp = *std::min_element(temperatureData.begin() + startIdx, temperatureData.begin() + endIdx);
                float maxTemp = *std::max_element(temperatureData.begin() + startIdx, temperatureData.begin() + endIdx);
                
                // Температура, на которую кликнули
                float clickedTemp = minTemp + static_cast<float>(xRatio) * (maxTemp - minTemp);
                
                // Находим ближайший индекс кадра с этой температурой
                float minDiff = FLT_MAX;
                size_t closestIdx = startIdx;
                for (size_t i = startIdx; i < endIdx; i++) {
                    float diff = fabs(temperatureData[i] - clickedTemp);
                    if (diff < minDiff) {
                        minDiff = diff;
                        closestIdx = i;
                    }
                }
                
                // Возвращаем индекс кадра (не температуру!)
                dataX = static_cast<double>(closestIdx);
            } else {
                dataX = xRatio * (dataModule->GetDataSize() - 1);
            }
        } else {
            dataX = xRatio * (dataModule->GetDataSize() - 1);
        }
    } else {
        // Обычный режим: номер кадра на оси X
        if (zoom->isZoomed) {
            dataX = zoom->minX + xRatio * (zoom->maxX - zoom->minX);
        } else {
            dataX = xRatio * (dataModule->GetDataSize() - 1);
        }
    }
    
    // Y координата обрабатывается одинаково для обоих режимов
    // Получаем реальный диапазон Y из данных
    ChartConfiguration config2 = curveSelectionModule->GetChartConfiguration(chartNum);
    std::vector<std::vector<float>> leftAxisData, rightAxisData;
    std::vector<CurveDefinition> leftAxisCurves, rightAxisCurves;
    curveSelectionModule->CalculateCurveData(config2, dataModule.get(), leftAxisData, rightAxisData, leftAxisCurves, rightAxisCurves);
    
    double minYFull, maxYFull, minXFull, maxXFull, minY2, maxY2;
    chartModule->GetDataRanges(leftAxisData, rightAxisData, minXFull, maxXFull, minYFull, maxYFull, minY2, maxY2);
    
    if (zoom->isZoomed) {
        dataY = zoom->minY + yRatio * (zoom->maxY - zoom->minY);
    } else {
        dataY = minYFull + yRatio * (maxYFull - minYFull);
    }
}


// Методы для обработки скроллинга
void ChartManager::HandleScrollEvent(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    HWND hScrollBar = (HWND)lParam;
    int scrollCode = LOWORD(wParam);
    
    // Определяем, какой скроллбар был использован
    int chartNumber = 0;
    bool isVertical = false;
    
    if (hScrollBar == uiModule->GetHScrollBar(1)) {
        chartNumber = 1;
        isVertical = false;
    } else if (hScrollBar == uiModule->GetHScrollBar(2)) {
        chartNumber = 2;
        isVertical = false;
    } else if (hScrollBar == uiModule->GetVScrollBar(1)) {
        chartNumber = 1;
        isVertical = true;
    } else if (hScrollBar == uiModule->GetVScrollBar(2)) {
        chartNumber = 2;
        isVertical = true;
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
    
    // Обновляем диапазон зума на основе новой позиции скроллбара
    ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;
    
    if (isVertical) {
        // Вертикальный скроллинг (ось Y)
        if (!zoom->isZoomed) {
            // Если график не в режиме zoom, вертикальный скроллинг не работает
            return;
        }
        
        // Получаем Y диапазон для ТЕКУЩЕГО X участка (не всех данных)
        // Это позволяет скроллить в пределах данных текущего видимого X диапазона
        ChartConfiguration config = curveSelectionModule->GetChartConfiguration(chartNumber);
        std::vector<std::vector<float>> leftAxisData, rightAxisData;
        std::vector<CurveDefinition> leftAxisCurves, rightAxisCurves;
        
        // Получаем данные для текущего X диапазона
        curveSelectionModule->CalculateCurveData(config, dataModule.get(), leftAxisData, rightAxisData,
                                                 leftAxisCurves, rightAxisCurves,
                                                 zoom->minX, zoom->maxX, -1e10, 1e10);
        
        double minYFull, maxYFull, minX, maxX, minY2, maxY2;
        chartModule->GetDataRanges(leftAxisData, rightAxisData, minX, maxX, minYFull, maxYFull, minY2, maxY2);
        
        // Добавляем padding 5% к диапазону данных для визуального комфорта
        double dataYRange = maxYFull - minYFull;
        double padding = dataYRange * 0.05;
        minYFull -= padding;
        maxYFull += padding;
        
        double totalYRange = maxYFull - minYFull;
        double viewYRange = zoom->maxY - zoom->minY;
        
        // Вертикальный скроллбар инвертирован (верх = 0, низ = 100)
        // newPos = 0 означает верх (максимальный Y)
        // newPos = maxScrollPos означает низ (минимальный Y)
        // Учитываем, что максимальная позиция скроллбара = nMax - nPage + 1
        if (totalYRange > viewYRange && totalYRange > 0) {
            int maxScrollPos = si.nMax - (int)si.nPage + 1;
            double scrollRatio = (maxScrollPos > 0) ? (double)(maxScrollPos - newPos) / (double)maxScrollPos : 0.0;
            double newMinY = minYFull + scrollRatio * (totalYRange - viewYRange);
            zoom->minY = newMinY;
            zoom->maxY = newMinY + viewYRange;
        }
    } else {
        // Горизонтальный скроллинг (ось X)
        double totalRange = static_cast<double>(dataModule->GetDataSize() - 1);
        double viewRange = zoom->maxX - zoom->minX;
        
        // Учитываем, что максимальная позиция скроллбара = nMax - nPage + 1
        // nMax = 100, nPage = (viewRange / totalRange) * 100
        // Поэтому реальный максимум = 100 - page + 1
        int maxScrollPos = si.nMax - (int)si.nPage + 1;
        double scrollRatio = (maxScrollPos > 0) ? (double)newPos / (double)maxScrollPos : 0.0;
        double newMinX = scrollRatio * (totalRange - viewRange);
        zoom->minX = newMinX;
        zoom->maxX = newMinX + viewRange;
        
        // Полный автомасштаб Y для нового X диапазона
        // Это обеспечивает оптимальный масштаб Y для видимых данных
        ChartConfiguration config = curveSelectionModule->GetChartConfiguration(chartNumber);
        std::vector<std::vector<float>> leftAxisData, rightAxisData;
        std::vector<CurveDefinition> leftAxisCurves, rightAxisCurves;
        
        // Получаем данные только для нового X диапазона
        curveSelectionModule->CalculateCurveData(config, dataModule.get(), 
                                                 leftAxisData, rightAxisData, 
                                                 leftAxisCurves, rightAxisCurves,
                                                 zoom->minX, zoom->maxX, -1e10, 1e10);
        
        // Находим min/max Y для данных в этом диапазоне
        double minYData, maxYData, minXData, maxXData, minY2, maxY2;
        chartModule->GetDataRanges(leftAxisData, rightAxisData, minXData, maxXData, minYData, maxYData, minY2, maxY2);
        
        // Добавляем padding 5% для визуального комфорта
        double yDataRange = maxYData - minYData;
        double padding = yDataRange * 0.05;
        zoom->minY = minYData - padding;
        zoom->maxY = maxYData + padding;
    }
    
    // Перерисовываем только нужный график (избегаем мигания)
    InvalidateChart(hWnd, chartNumber);
}

void ChartManager::UpdateScrollBars(HWND hWnd) {
    // Обновляем скроллбары для первого графика
    if (chart1Zoom.isZoomed) {
        uiModule->ShowScrollBars(1, true);
        uiModule->UpdateScrollBarPositions(1, layoutModule->GetChart1Rect());
        
        // Настраиваем диапазон горизонтального скроллбара (ось X)
        double totalDataRange = static_cast<double>(dataModule->GetDataSize() - 1);
        double viewRangeX = chart1Zoom.maxX - chart1Zoom.minX;
        
        // Вычисляем page size и максимальную позицию скроллбара
        int pageSize = static_cast<int>((viewRangeX / totalDataRange) * 100);
        int maxScrollPos = 100 - pageSize + 1;
        
        // Вычисляем позицию скроллбара пропорционально
        double scrollableRange = totalDataRange - viewRangeX;
        int hScrollPos = (scrollableRange > 0) ? static_cast<int>((chart1Zoom.minX / scrollableRange) * maxScrollPos) : 0;
        uiModule->SetScrollBarRange(1, 0, 100, pageSize, hScrollPos);
        
        // Настраиваем диапазон вертикального скроллбара (ось Y)
        // Получаем Y диапазон для ТЕКУЩЕГО X участка (не всех данных)
        ChartConfiguration config = curveSelectionModule->GetChartConfiguration(1);
        std::vector<std::vector<float>> leftAxisData, rightAxisData;
        std::vector<CurveDefinition> leftAxisCurves, rightAxisCurves;
        
        // Получаем данные для текущего X диапазона
        curveSelectionModule->CalculateCurveData(config, dataModule.get(), leftAxisData, rightAxisData, 
                                                 leftAxisCurves, rightAxisCurves,
                                                 chart1Zoom.minX, chart1Zoom.maxX, -1e10, 1e10);
        
        double minYFull, maxYFull, minX, maxX, minY2, maxY2;
        chartModule->GetDataRanges(leftAxisData, rightAxisData, minX, maxX, minYFull, maxYFull, minY2, maxY2);
        
        // Добавляем padding 5% к диапазону данных для согласованности с HandleScrollEvent
        double dataYRange = maxYFull - minYFull;
        double padding = dataYRange * 0.05;
        minYFull -= padding;
        maxYFull += padding;
        
        double totalYRange = maxYFull - minYFull;
        double viewRangeY = chart1Zoom.maxY - chart1Zoom.minY;
        
        // Вертикальный скроллбар инвертирован (верх = 0, низ = 100)
        // Защита от деления на ноль
        int vScrollPos = 50;
        int vScrollPage = 100;
        if (totalYRange > viewRangeY && totalYRange > 0) {
            vScrollPage = static_cast<int>((viewRangeY / totalYRange) * 100);
            int maxScrollPos = 100 - vScrollPage + 1;
            
            // Вычисляем позицию: 0 = верх (maxY), maxScrollPos = низ (minY)
            double scrollableRange = totalYRange - viewRangeY;
            double scrollRatio = (chart1Zoom.minY - minYFull) / scrollableRange;
            vScrollPos = static_cast<int>((1.0 - scrollRatio) * maxScrollPos);
        }
        uiModule->SetVScrollBarRange(1, 0, 100, vScrollPage, vScrollPos);
    } else {
        uiModule->ShowScrollBars(1, false);
    }
    
    // Обновляем скроллбары для второго графика (если в режиме двух графиков)
    if (uiModule->IsDualMode()) {
        if (chart2Zoom.isZoomed) {
            uiModule->ShowScrollBars(2, true);
            uiModule->UpdateScrollBarPositions(2, layoutModule->GetChart2Rect());
            
            // Настраиваем диапазон горизонтального скроллбара (ось X)
            double totalDataRange = static_cast<double>(dataModule->GetDataSize() - 1);
            double viewRangeX = chart2Zoom.maxX - chart2Zoom.minX;
            
            // Вычисляем page size и максимальную позицию скроллбара
            int pageSize = static_cast<int>((viewRangeX / totalDataRange) * 100);
            int maxScrollPos = 100 - pageSize + 1;
            
            // Вычисляем позицию скроллбара пропорционально
            double scrollableRange = totalDataRange - viewRangeX;
            int hScrollPos = (scrollableRange > 0) ? static_cast<int>((chart2Zoom.minX / scrollableRange) * maxScrollPos) : 0;
            uiModule->SetScrollBarRange(2, 0, 100, pageSize, hScrollPos);
            
            // Настраиваем диапазон вертикального скроллбара (ось Y)
            // Получаем Y диапазон для ТЕКУЩЕГО X участка (не всех данных)
            ChartConfiguration config = curveSelectionModule->GetChartConfiguration(2);
            std::vector<std::vector<float>> leftAxisData, rightAxisData;
            std::vector<CurveDefinition> leftAxisCurves, rightAxisCurves;
            
            // Получаем данные для текущего X диапазона
            curveSelectionModule->CalculateCurveData(config, dataModule.get(), leftAxisData, rightAxisData,
                                                     leftAxisCurves, rightAxisCurves,
                                                     chart2Zoom.minX, chart2Zoom.maxX, -1e10, 1e10);
            
            double minYFull, maxYFull, minX, maxX, minY2, maxY2;
            chartModule->GetDataRanges(leftAxisData, rightAxisData, minX, maxX, minYFull, maxYFull, minY2, maxY2);
            
            // Добавляем padding 5% к диапазону данных для согласованности с HandleScrollEvent
            double dataYRange = maxYFull - minYFull;
            double padding = dataYRange * 0.05;
            minYFull -= padding;
            maxYFull += padding;
            
            double totalYRange = maxYFull - minYFull;
            double viewRangeY = chart2Zoom.maxY - chart2Zoom.minY;
            
            // Вертикальный скроллбар инвертирован (верх = 0, низ = 100)
            // Защита от деления на ноль
            int vScrollPos = 50;
            int vScrollPage = 100;
            if (totalYRange > viewRangeY && totalYRange > 0) {
                vScrollPage = static_cast<int>((viewRangeY / totalYRange) * 100);
                int maxScrollPos = 100 - vScrollPage + 1;
                
                // Вычисляем позицию: 0 = верх (maxY), maxScrollPos = низ (minY)
                double scrollableRange = totalYRange - viewRangeY;
                double scrollRatio = (chart2Zoom.minY - minYFull) / scrollableRange;
                vScrollPos = static_cast<int>((1.0 - scrollRatio) * maxScrollPos);
            }
            uiModule->SetVScrollBarRange(2, 0, 100, vScrollPage, vScrollPos);
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
    // FALSE - не стираем фон, т.к. используем double buffering для устранения мигания
    InvalidateRect(hWnd, &chartRect, FALSE);
    // Принудительно обновляем окно для немедленной перерисовки
    UpdateWindow(hWnd);
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
        // Используем zoom параметры только для X (не фильтруем по Y!)
        // Y zoom используется только для отображения, не для фильтрации данных
        curveSelectionModule->CalculateCurveData(config, dataModule.get(),
                                               leftAxisData, rightAxisData,
                                               leftAxisCurves, rightAxisCurves,
                                               zoom->minX, zoom->maxX, -1e10, 1e10);
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
    
    // v2.10.0: Проверяем тип оси X
    if (config.xAxisType == XAxisType::TEMPERATURE) {
        // Режим кроссплота: температура на оси X
        
        // Получаем температурные данные
        std::vector<float> temperatureData;
        if (zoom->isZoomed) {
            dataModule->GetTemperatureData(temperatureData, zoom->minX, zoom->maxX);
        } else {
            dataModule->GetTemperatureData(temperatureData);
        }
        
        // Проверяем, что данные получены
        if (temperatureData.empty()) {
            // Отрисовываем пустой график с сообщением
            chartModule->DrawDualAxisChart(hdc, chartRect, config.curves,
                                         leftAxisData, rightAxisData,
                                         title + L" (No temperature data)", false,
                                         0.0, 1.0, -1e6, 1e6, true);
            return;
        }
        
        // Определяем диапазоны
        double minXData = zoom->isZoomed ? zoom->minX : 0.0;
        double maxXData = zoom->isZoomed ? zoom->maxX : static_cast<double>(dataModule->GetDataSize() - 1);
        
        // Получаем диапазоны Y из данных
        double minYFull, maxYFull, minX, maxX, minY2, maxY2;
        chartModule->GetDataRanges(leftAxisData, rightAxisData, minX, maxX, minYFull, maxYFull, minY2, maxY2);
        
        double minYData = zoom->isZoomed ? zoom->minY : minYFull;
        double maxYData = zoom->isZoomed ? zoom->maxY : maxYFull;
        
        // Вызываем новый метод отрисовки с температурой и правильной легендой
        chartModule->DrawDualAxisChart(hdc, chartRect, config.curves,
                                       leftAxisData, rightAxisData,
                                       title, true, temperatureData,
                                       minXData, maxXData, minYData, maxYData, true);
    } else {
        // Обычный режим: номер кадра на оси X
        
        // Получаем диапазоны Y из данных
        double minYData, maxYData, minXData, maxXData, minY2, maxY2;
        chartModule->GetDataRanges(leftAxisData, rightAxisData, minXData, maxXData, minYData, maxYData, minY2, maxY2);
        
        // Проверяем, нужны ли двойные оси
        if (config.hasDualAxis && !rightAxisData.empty()) {
            // Используем новый метод для двойных осей с поддержкой zoom
            // Всегда показываем оси (включая ось X)
            bool showAxes = true;
            
            double displayMinY = zoom->isZoomed ? zoom->minY : minYData;
            double displayMaxY = zoom->isZoomed ? zoom->maxY : maxYData;
            
            chartModule->DrawDualAxisChart(hdc, chartRect, config.curves,
                                         leftAxisData, rightAxisData,
                                         title, dataModule->HasData(),
                                         zoom->isZoomed ? zoom->minX : 0.0,
                                         zoom->isZoomed ? zoom->maxX : static_cast<double>(dataModule->GetDataSize() > 0 ? dataModule->GetDataSize() - 1 : 0),
                                         displayMinY,
                                         displayMaxY,
                                         showAxes);
        } else {
            // Используем обычный метод для одной оси
            if (!leftAxisData.empty()) {
                // Отрисовываем график с кастомными кривыми
                // Используем метод DrawDualAxisChart даже для одной оси, чтобы правильно отобразить легенду
                bool showAxes = true;
                
                double displayMinY = zoom->isZoomed ? zoom->minY : minYData;
                double displayMaxY = zoom->isZoomed ? zoom->maxY : maxYData;
                
                chartModule->DrawDualAxisChart(hdc, chartRect, leftAxisCurves,
                                             leftAxisData, std::vector<std::vector<float>>(), // Пустая правая ось
                                             title, dataModule->HasData(),
                                             zoom->isZoomed ? zoom->minX : 0.0,
                                             zoom->isZoomed ? zoom->maxX : static_cast<double>(dataModule->GetDataSize() > 0 ? dataModule->GetDataSize() - 1 : 0),
                                             displayMinY,
                                             displayMaxY,
                                             showAxes);
            }
        }
    }
}

void ChartManager::DrawChartLegacy(HDC hdc, const RECT& chartRect, int chartNumber) {
    // Проверяем, загружены ли данные
    bool hasData = dataModule && dataModule->HasData();
    
    std::wstring title = L"Chart " + std::to_wstring(chartNumber);
    
    // Отрисовываем фон и рамку графика вручную (без подписей осей)
    chartModule->DrawChartBackground(hdc, chartRect);
    chartModule->DrawChartBorder(hdc, chartRect);
    chartModule->DrawChartTitle(hdc, chartRect, title);
    
    // Расчет области для графика
    // ВАЖНО: Используем те же отступы, что и в DrawDualAxisChart (CalculateDualAxisPlotRect)
    RECT plotRect = chartRect;
    plotRect.left += 60;
    plotRect.top += 45;
    plotRect.right -= 140;
    plotRect.bottom -= 50;
    
    // Рисуем оси
    chartModule->DrawChartAxes(hdc, plotRect, true);
    
    // Если данные загружены, рисуем только сетку БЕЗ подписей осей
    if (hasData) {
        chartModule->DrawGridOnly(hdc, plotRect);
    }
    
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
        chart1ZoomHistory.clear(); // v2.12.5: Очищаем историю zoom
    } else if (chartNumber == 2) {
        chart2Zoom.isZoomed = false;
        chart2ZoomHistory.clear(); // v2.12.5: Очищаем историю zoom
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
    
    // Получаем максимальное значение данных
    double dataSize = static_cast<double>(dataModule->GetDataSize() - 1);
    
    // Получаем значения из полей ввода для указанного графика
    double fromX, toX;
    uiModule->GetXRangeValues(chartNumber, fromX, toX);
    
    // Если поле "To" пустое, используем максимальное значение данных
    if (uiModule->IsXRangeToEmpty(chartNumber)) {
        toX = dataSize;
        // Обновляем поле "To" в UI, чтобы пользователь видел применённое значение
        uiModule->SetXRangeValues(chartNumber, fromX, toX);
    }
    
    // Валидация входных данных
    if (fromX >= toX) {
        wchar_t message[256];
        swprintf_s(message, L"Chart %d: 'From' value must be less than 'To' value", chartNumber);
        MessageBox(hWnd, message, L"X Range Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    if (fromX < 0 || toX > dataSize || fromX > dataSize || toX < 0) {
        wchar_t message[256];
        swprintf_s(message, L"Chart %d: X Range must be between 0 and %.0f", chartNumber, dataSize);
        MessageBox(hWnd, message, L"X Range Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Проверяем минимальный диапазон: подписи должны быть целыми и отличаться минимум на 10
    const double MIN_X_RANGE = 10.0;
    double xRange = toX - fromX;
    
    if (xRange < MIN_X_RANGE) {
        wchar_t message[256];
        swprintf_s(message, L"Chart %d: X Range must be at least %.0f points.\nX-axis labels must be integers with minimum spacing of 1.", 
                   chartNumber, MIN_X_RANGE);
        MessageBox(hWnd, message, L"X Range Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Применяем диапазон к указанному графику
    ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;
    zoom->minX = fromX;
    zoom->maxX = toX;
    
    // Вычисляем Y диапазон на основе данных в новом X диапазоне
    // Это обеспечивает правильный масштаб Y для видимого участка
    ChartConfiguration config = curveSelectionModule->GetChartConfiguration(chartNumber);
    std::vector<std::vector<float>> leftAxisData, rightAxisData;
    std::vector<CurveDefinition> leftAxisCurves, rightAxisCurves;
    
    // Получаем данные только для нового X диапазона
    curveSelectionModule->CalculateCurveData(config, dataModule.get(), 
                                             leftAxisData, rightAxisData, 
                                             leftAxisCurves, rightAxisCurves,
                                             fromX, toX, -1e10, 1e10);
    
    // Находим min/max Y для данных в этом диапазоне
    double minYData, maxYData, minXData, maxXData, minY2, maxY2;
    chartModule->GetDataRanges(leftAxisData, rightAxisData, minXData, maxXData, minYData, maxYData, minY2, maxY2);
    
    // Добавляем padding 5% для визуального комфорта
    double yRange = maxYData - minYData;
    double padding = yRange * 0.05;
    zoom->minY = minYData - padding;
    zoom->maxY = maxYData + padding;
    
    zoom->isZoomed = true;
    
    // Обновляем скроллбары
    UpdateScrollBars(hWnd);
    
    // Обновляем UI
    UpdateXRangeUI(hWnd);
    
    // Перерисовываем графики (FALSE для double buffering)
    InvalidateRect(hWnd, NULL, FALSE);
}

void ChartManager::HandleXRangeReset(HWND hWnd, int chartNumber) {
    // Сбрасываем масштабирование для указанного графика
    ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;
    zoom->isZoomed = false;
    
    // Обновляем скроллбары
    UpdateScrollBars(hWnd);
    
    // Обновляем UI
    UpdateXRangeUI(hWnd);
    
    // Перерисовываем графики (FALSE для double buffering)
    InvalidateRect(hWnd, NULL, FALSE);
}

void ChartManager::HandleResetZoom(HWND hWnd, int chartNumber) {
    // Сбрасываем zoom для указанного графика (полный сброс, не пошаговый)
    ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;
    zoom->isZoomed = false;
    
    // v2.12.5: Очищаем историю zoom при полном сбросе через кнопку
    if (chartNumber == 1) {
        chart1ZoomHistory.clear();
    } else if (chartNumber == 2) {
        chart2ZoomHistory.clear();
    }
    
    // Обновляем скроллбары
    UpdateScrollBars(hWnd);
    
    // Обновляем UI (X Range controls)
    UpdateXRangeUI(hWnd);
    
    // Перерисовываем только измененный график
    InvalidateChart(hWnd, chartNumber);
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
        // ВАЖНО: Используем те же отступы, что и в CalculateDualAxisPlotRect
        const int MARGIN_LEFT = 60;
        const int MARGIN_RIGHT = 140;
        const int MARGIN_TOP = 45;
        const int MARGIN_BOTTOM = 50;
        
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

// Отрисовка перекрестия (crosshair) на графике
void ChartManager::DrawCrosshair(HDC hdc, const RECT& chartRect, int chartNumber) const {
    // Получаем перекрестие для указанного графика
    const CrosshairPosition* crosshair = (chartNumber == 1) ? &chart1Crosshair : &chart2Crosshair;
    
    // Проверяем, нужно ли отображать перекрестие
    if (!crosshair->isVisible || crosshair->chartNumber != chartNumber) {
        return;
    }
    
    // Вычисляем область графика (plotRect) с учётом отступов
    // ВАЖНО: Всегда используем отступы для двойных осей, потому что DrawDualAxisChart
    // вызывается для всех графиков и использует CalculateDualAxisPlotRect
    RECT plotRect;
    plotRect.left = chartRect.left + 60;
    plotRect.top = chartRect.top + 45;
    plotRect.right = chartRect.right - 140;
    plotRect.bottom = chartRect.bottom - 50;
    
    // Проверяем, что перекрестие находится внутри области графика
    if (crosshair->screenX < plotRect.left || crosshair->screenX > plotRect.right ||
        crosshair->screenY < plotRect.top || crosshair->screenY > plotRect.bottom) {
        return;
    }
    
    // Создаём красное перо для перекрестия
    HPEN hCrosshairPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hCrosshairPen);
    
    // Рисуем вертикальную линию (от верха до низа plotRect)
    MoveToEx(hdc, crosshair->screenX, plotRect.top, NULL);
    LineTo(hdc, crosshair->screenX, plotRect.bottom);
    
    // Рисуем горизонтальную линию (от левого до правого края plotRect)
    MoveToEx(hdc, plotRect.left, crosshair->screenY, NULL);
    LineTo(hdc, plotRect.right, crosshair->screenY);
    
    // Восстанавливаем старое перо и удаляем созданное
    SelectObject(hdc, hOldPen);
    DeleteObject(hCrosshairPen);
}


// === Методы для работы с COM-портом (v2.13.0) ===

void ChartManager::HandleSerialConnect(HWND hWnd) {
    // v2.13.0: Подключение теперь происходит через диалог IDD_SERIAL_STATUS
    // Этот метод оставлен для совместимости с меню, но не используется напрямую
    // Вместо этого открывается диалог через IDM_DEVICE_CONNECT
}

void ChartManager::HandleSerialDisconnect(HWND hWnd) {
    if (!serialModule) return;
    
    // Останавливаем опрос перед отключением
    if (serialModule->IsPolling()) {
        serialModule->StopPolling();
    }
    
    serialModule->Disconnect();
    
    if (uiModule) {
        uiModule->SetSerialConnected(false);
        uiModule->SetSerialPolling(false);
    }
    
    UpdateSerialUI(hWnd);
}

void ChartManager::HandleSerialStart(HWND hWnd) {
    if (!serialModule || !serialModule->IsConnected()) return;
    
    // Запускаем периодический опрос (каждые 2 секунды)
    // Примечание: Очистка данных происходит в диалоге SerialStatusDialogProc
    // перед вызовом StartPolling через HandleSerialClear
    serialModule->StartPolling(hWnd, 2000);
    
    if (uiModule) {
        uiModule->SetSerialPolling(true);
    }
}

void ChartManager::HandleSerialStop(HWND hWnd) {
    if (!serialModule) return;
    
    serialModule->StopPolling();
    
    if (uiModule) {
        uiModule->SetSerialPolling(false);
    }
}

void ChartManager::HandleSerialClear(HWND hWnd) {
    if (!dataModule) return;
    
    // Очищаем данные
    dataModule->ClearData();
    
    // Сбрасываем статистику
    if (serialModule) {
        serialModule->ResetStatistics();
    }
    
    // Сбрасываем zoom
    chart1Zoom.isZoomed = false;
    chart2Zoom.isZoomed = false;
    chart1ZoomHistory.clear();
    chart2ZoomHistory.clear();
    
    // Обновляем UI
    if (uiModule) {
        uiModule->UpdateSerialFrameInfo(0, 0, 0);
        uiModule->UpdateXRangeControls(1, 0.0, 0.0, false);
        uiModule->UpdateXRangeControls(2, 0.0, 0.0, false);
    }
    
    // Перерисовываем графики
    InvalidateRect(hWnd, nullptr, TRUE);
    
    UpdateStatusBarFileInfo();
}

void ChartManager::HandleSerialTimer(HWND hWnd) {
    if (!serialModule) return;
    
    // Вызываем обработчик таймера в SerialModule
    serialModule->OnTimer();
}

void ChartManager::UpdateSerialUI(HWND hWnd) {
    // v2.13.0: UI обновляется через диалог IDD_SERIAL_STATUS
    // Этот метод оставлен для совместимости, но не выполняет действий
}

void ChartManager::RefreshPortList(HWND hWnd) {
    // v2.13.0: Список портов обновляется через диалог IDD_SERIAL_STATUS
    // Этот метод оставлен для совместимости, но не выполняет действий
}

bool ChartManager::IsSerialConnected() const {
    return serialModule && serialModule->IsConnected();
}

bool ChartManager::IsSerialPolling() const {
    return serialModule && serialModule->IsPolling();
}

// === Методы для режима Surf (v2.14.0) ===

void ChartManager::HandleSurfToggle(HWND hWnd, int chartNumber) {
    if (!dataModule || !dataModule->HasData()) {
        return;
    }
    
    bool surfEnabled = uiModule->IsSurfEnabled(chartNumber);
    
    if (surfEnabled) {
        // Surf включён - устанавливаем диапазон на последние N точек
        UpdateSurfRange(chartNumber);
    } else {
        // Surf выключён - сбрасываем zoom к полному диапазону
        ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;
        zoom->isZoomed = false;
        
        // Обновляем X Range controls на полный диапазон
        double maxX = static_cast<double>(dataModule->GetDataSize() - 1);
        uiModule->UpdateXRangeControls(chartNumber, 0.0, maxX, false);
    }
    
    // Обновляем статусную строку
    UpdateSurfStatusBar();
    
    // Перерисовываем график
    InvalidateChart(hWnd, chartNumber);
    UpdateScrollBars(hWnd);
}

void ChartManager::UpdateSurfRange(int chartNumber) {
    if (!dataModule || !dataModule->HasData()) {
        return;
    }
    
    if (!uiModule->IsSurfEnabled(chartNumber)) {
        return;
    }
    
    size_t dataSize = dataModule->GetDataSize();
    int surfWindow = uiModule->GetSurfWindowSize(chartNumber);
    
    double surfMinX = (dataSize > static_cast<size_t>(surfWindow)) 
        ? static_cast<double>(dataSize - surfWindow) : 0.0;
    double surfMaxX = static_cast<double>(dataSize - 1);
    
    // Устанавливаем zoom на область Surf (только по X)
    ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;
    zoom->minX = surfMinX;
    zoom->maxX = surfMaxX;
    
    // Вычисляем диапазон Y для данных в окне Surf
    // Получаем конфигурацию кривых и вычисляем данные (без zoom, чтобы получить все данные)
    ChartConfiguration config = curveSelectionModule->GetChartConfiguration(chartNumber);
    std::vector<std::vector<float>> leftAxisData, rightAxisData;
    std::vector<CurveDefinition> leftAxisCurves, rightAxisCurves;
    
    // Используем версию с zoom для получения данных только в диапазоне Surf
    // Передаём большой диапазон Y, чтобы не фильтровать по Y
    curveSelectionModule->CalculateCurveData(config, dataModule.get(),
                                            leftAxisData, rightAxisData,
                                            leftAxisCurves, rightAxisCurves,
                                            surfMinX, surfMaxX, -1e30, 1e30);
    
    // Получаем диапазоны Y из данных
    double minX, maxX, minYData, maxYData, minY2, maxY2;
    chartModule->GetDataRanges(leftAxisData, rightAxisData, minX, maxX, minYData, maxYData, minY2, maxY2);
    
    // Устанавливаем Y диапазон с небольшим padding
    double yRange = maxYData - minYData;
    double padding = yRange * 0.05;
    zoom->minY = minYData - padding;
    zoom->maxY = maxYData + padding;
    
    zoom->isZoomed = true;
    
    // Обновляем X Range controls
    uiModule->UpdateXRangeControls(chartNumber, surfMinX, surfMaxX, true);
}

void ChartManager::UpdateSurfStatusBar() {
    bool surf1Enabled = uiModule->IsSurfEnabled(1);
    int surf1Window = uiModule->GetSurfWindowSize(1);
    bool surf2Enabled = uiModule->IsSurfEnabled(2);
    int surf2Window = uiModule->GetSurfWindowSize(2);
    
    uiModule->UpdateStatusBarSurfInfo(surf1Enabled, surf1Window, surf2Enabled, surf2Window);
}
