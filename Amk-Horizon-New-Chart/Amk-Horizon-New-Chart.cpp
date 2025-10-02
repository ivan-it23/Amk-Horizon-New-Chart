// Amk-Horizon-New-Chart.cpp : Определяет точку входа для приложения.
//

#include "framework.h"
#include "Amk-Horizon-New-Chart.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cfloat>
#include <cmath>

#define MAX_LOADSTRING 100

// Глобальные переменные:
HINSTANCE hInst;                                // текущий экземпляр
WCHAR szTitle[MAX_LOADSTRING];                  // Текст строки заголовка
WCHAR szWindowClass[MAX_LOADSTRING];            // имя класса главного окна
ChartManager* g_chartManager = nullptr;         // Менеджер графиков

// Отправить объявления функций, включенных в этот модуль кода:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Разместите код здесь.

    // Инициализация глобальных строк
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_AMKHORIZONNEWCHART, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Выполнить инициализацию приложения:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_AMKHORIZONNEWCHART));

    MSG msg;

    // Цикл основного сообщения:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  ФУНКЦИЯ: MyRegisterClass()
//
//  ЦЕЛЬ: Регистрирует класс окна.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_AMKHORIZONNEWCHART));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_AMKHORIZONNEWCHART);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   ФУНКЦИЯ: InitInstance(HINSTANCE, int)
//
//   ЦЕЛЬ: Сохраняет маркер экземпляра и создает главное окно
//
//   КОММЕНТАРИИ:
//
//        В этой функции маркер экземпляра сохраняется в глобальной переменной, а также
//        создается и выводится главное окно программы.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Сохранить маркер экземпляра в глобальной переменной

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
      CW_USEDEFAULT, 0, 800, 600, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   // Создаем менеджер графиков
   g_chartManager = new ChartManager();
   g_chartManager->CreateControls(hWnd, hInstance);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  ФУНКЦИЯ: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  ЦЕЛЬ: Обрабатывает сообщения в главном окне.
//
//  WM_COMMAND  - обработать меню приложения
//  WM_PAINT    - Отрисовка главного окна
//  WM_DESTROY  - отправить сообщение о выходе и вернуться
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Разобрать выбор в меню:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            case IDM_OPEN_FILE:
                {
                    OPENFILENAME ofn;
                    WCHAR szFile[260] = { 0 };
                    
                    ZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = hWnd;
                    ofn.lpstrFile = szFile;
                    ofn.nMaxFile = sizeof(szFile);
                    ofn.lpstrFilter = L"Binary Files\0*.bin\0All Files\0*.*\0";
                    ofn.nFilterIndex = 1;
                    ofn.lpstrFileTitle = NULL;
                    ofn.nMaxFileTitle = 0;
                    ofn.lpstrInitialDir = NULL;
                    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                    
                    if (GetOpenFileName(&ofn) && g_chartManager)
                    {
                        g_chartManager->LoadDataFromFile(szFile);
                        InvalidateRect(hWnd, NULL, TRUE);
                    }
                }
                break;
            case IDM_TOGGLE_DUAL:
                if (g_chartManager)
                {
                    g_chartManager->ToggleDualMode();
                    g_chartManager->UpdateLayout(hWnd);
                    InvalidateRect(hWnd, NULL, TRUE);
                }
                break;
            case IDM_TEST_REFRESH:
                if (g_chartManager)
                {
                    g_chartManager->RefreshChart1();
                    if (g_chartManager->IsDualMode())
                        g_chartManager->RefreshChart2();
                    InvalidateRect(hWnd, NULL, TRUE);
                }
                break;
            default:
                if (g_chartManager)
                {
                    if (g_chartManager->HandleCommand(wParam, lParam))
                    {
                        InvalidateRect(hWnd, NULL, TRUE);
                    }
                }
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_SIZE:
        if (g_chartManager)
        {
            g_chartManager->UpdateLayout(hWnd);
            InvalidateRect(hWnd, NULL, TRUE);
        }
        break;
    case WM_LBUTTONDOWN:
        if (g_chartManager)
        {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            g_chartManager->HandleMouseDown(hWnd, x, y);
        }
        break;
    case WM_MOUSEMOVE:
        if (g_chartManager)
        {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            g_chartManager->HandleMouseMove(hWnd, x, y);
        }
        break;
    case WM_LBUTTONUP:
        if (g_chartManager)
        {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            g_chartManager->HandleMouseUp(hWnd, x, y);
        }
        break;
    case WM_RBUTTONDOWN:
        if (g_chartManager)
        {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            g_chartManager->HandleRightClick(hWnd, x, y);
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            
            if (g_chartManager)
            {
                g_chartManager->DrawChart1(hdc);
                if (g_chartManager->IsDualMode())
                {
                    g_chartManager->DrawChart2(hdc);
                }
            }
            
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        if (g_chartManager)
        {
            delete g_chartManager;
            g_chartManager = nullptr;
        }
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Обработчик сообщений для окна "О программе".
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

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
    
    // Отрисовка графика
    chartModule->DrawChart(hdc, chart1Rect, chartData, dataType, title, dataModule->HasData(), enabledTransmitters, dataModule.get());
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
                                      0.0, static_cast<double>(dataModule->GetDataSize() - 1), -1e6, 1e6);
    }
    
    // Формируем заголовок
    std::wstring title = BuildChartTitle(2, dataType, frequency);
    
    // Отрисовка графика
    chartModule->DrawChart(hdc, chart2Rect, chartData, dataType, title, dataModule->HasData(), enabledTransmitters, dataModule.get());
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
    
    // Перерисовываем окно
    InvalidateRect(hWnd, NULL, TRUE);
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
    
    InvalidateRect(hWnd, NULL, TRUE);
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