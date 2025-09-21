// Amk-Horizon-New-Chart.cpp : Определяет точку входа для приложения.
//

#include "framework.h"
#include "Amk-Horizon-New-Chart.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cfloat>

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

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
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
                    g_chartManager->HandleCommand(wParam, lParam);
                    InvalidateRect(hWnd, NULL, TRUE);
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

// Реализация класса ChartManager
ChartManager::ChartManager() 
    : dualMode(false)
    , chart1DataType(DataType::PHASE_DIFFERENCE)
    , chart2DataType(DataType::AMPLITUDE_RATIO)
    , chart1Frequency(FrequencyBand::FREQ_400_KHZ)
    , chart2Frequency(FrequencyBand::FREQ_400_KHZ)
    , hDualModeCheck(nullptr)
    , hChart1DataCombo(nullptr)
    , hChart1FreqCombo(nullptr)
    , hChart1RefreshBtn(nullptr)
    , hChart2DataCombo(nullptr)
    , hChart2FreqCombo(nullptr)
    , hChart2RefreshBtn(nullptr)
    , hChart2Panel(nullptr)
{
    ZeroMemory(&chart1Rect, sizeof(RECT));
    ZeroMemory(&chart2Rect, sizeof(RECT));
}

void ChartManager::CreateControls(HWND hWnd, HINSTANCE hInst)
{
    // Создание элементов управления первого графика
    hDualModeCheck = CreateWindow(L"BUTTON", L"Dual Mode", 
        WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
        10, 30, 100, 20, hWnd, (HMENU)IDC_DUAL_MODE_CHECK, hInst, NULL);
    
    hChart1DataCombo = CreateWindow(L"COMBOBOX", NULL,
        WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        120, 28, 180, 200, hWnd, (HMENU)IDC_CHART1_DATA_COMBO, hInst, NULL);
    
    hChart1FreqCombo = CreateWindow(L"COMBOBOX", NULL,
        WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
        310, 28, 100, 100, hWnd, (HMENU)IDC_CHART1_FREQ_COMBO, hInst, NULL);
    
    hChart1RefreshBtn = CreateWindow(L"BUTTON", L"Refresh",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        420, 28, 80, 25, hWnd, (HMENU)IDC_CHART1_REFRESH_BTN, hInst, NULL);
    
    // Заполнение комбобоксов
    const wchar_t* dataTypes[] = {
        L"Phase Difference", L"Amplitude Ratio", L"Amplitude RX1", L"Amplitude RX2",
        L"Phase RX1", L"Phase RX2", L"Silent Amplitude RX1", L"Silent Amplitude RX2", L"Temperature"
    };
    
    for (int i = 0; i < 9; i++)
    {
        SendMessage(hChart1DataCombo, CB_ADDSTRING, 0, (LPARAM)dataTypes[i]);
    }
    SendMessage(hChart1DataCombo, CB_SETCURSEL, 0, 0);
    
    SendMessage(hChart1FreqCombo, CB_ADDSTRING, 0, (LPARAM)L"400 кГц");
    SendMessage(hChart1FreqCombo, CB_ADDSTRING, 0, (LPARAM)L"2000 кГц");
    SendMessage(hChart1FreqCombo, CB_SETCURSEL, 0, 0);
    
    // Создание элементов управления второго графика (изначально скрыты)
    // Используем WS_EX_TOPMOST для обеспечения правильного Z-порядка
    hChart2DataCombo = CreateWindowEx(WS_EX_TOPMOST, L"COMBOBOX", NULL,
        WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        10, 300, 180, 200, hWnd, (HMENU)IDC_CHART2_DATA_COMBO, hInst, NULL);
    
    hChart2FreqCombo = CreateWindowEx(WS_EX_TOPMOST, L"COMBOBOX", NULL,
        WS_CHILD | CBS_DROPDOWNLIST,
        200, 300, 100, 100, hWnd, (HMENU)IDC_CHART2_FREQ_COMBO, hInst, NULL);
    
    hChart2RefreshBtn = CreateWindowEx(WS_EX_TOPMOST, L"BUTTON", L"Refresh",
        WS_CHILD | BS_PUSHBUTTON,
        310, 300, 80, 25, hWnd, (HMENU)IDC_CHART2_REFRESH_BTN, hInst, NULL);
    
    // Заполнение комбобоксов второго графика
    for (int i = 0; i < 9; i++)
    {
        SendMessage(hChart2DataCombo, CB_ADDSTRING, 0, (LPARAM)dataTypes[i]);
    }
    SendMessage(hChart2DataCombo, CB_SETCURSEL, 1, 0);
    
    SendMessage(hChart2FreqCombo, CB_ADDSTRING, 0, (LPARAM)L"400 кГц");
    SendMessage(hChart2FreqCombo, CB_ADDSTRING, 0, (LPARAM)L"2000 кГц");
    SendMessage(hChart2FreqCombo, CB_SETCURSEL, 0, 0);
}

void ChartManager::UpdateLayout(HWND hWnd)
{
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    
    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;
    
    // Валидация ограничений компоновки
    if (!ValidateLayoutConstraints(width, height))
        return;
    
    CalculateLayout(width, height);
    
    if (dualMode)
    {
        // Режим двух графиков
        ShowWindow(hChart2DataCombo, SW_SHOW);
        ShowWindow(hChart2FreqCombo, SW_SHOW);
        ShowWindow(hChart2RefreshBtn, SW_SHOW);
        
        // Позиционирование элементов управления второго графика
        int panel2Y = chart1Rect.bottom + MARGIN;
        PositionChart2Controls(panel2Y);
    }
    else
    {
        // Режим одного графика
        ShowWindow(hChart2DataCombo, SW_HIDE);
        ShowWindow(hChart2FreqCombo, SW_HIDE);
        ShowWindow(hChart2RefreshBtn, SW_HIDE);
    }
}

void ChartManager::ToggleDualMode()
{
    dualMode = !dualMode;
    SendMessage(hDualModeCheck, BM_SETCHECK, dualMode ? BST_CHECKED : BST_UNCHECKED, 0);
}

void ChartManager::RefreshChart1()
{
    // Обновление данных первого графика
    int dataTypeIndex = (int)SendMessage(hChart1DataCombo, CB_GETCURSEL, 0, 0);
    int freqIndex = (int)SendMessage(hChart1FreqCombo, CB_GETCURSEL, 0, 0);
    
    if (dataTypeIndex >= 0 && dataTypeIndex < 9)
        chart1DataType = (DataType)dataTypeIndex;
    if (freqIndex >= 0 && freqIndex < 2)
        chart1Frequency = (FrequencyBand)freqIndex;
}

void ChartManager::RefreshChart2()
{
    // Обновление данных второго графика
    int dataTypeIndex = (int)SendMessage(hChart2DataCombo, CB_GETCURSEL, 0, 0);
    int freqIndex = (int)SendMessage(hChart2FreqCombo, CB_GETCURSEL, 0, 0);
    
    if (dataTypeIndex >= 0 && dataTypeIndex < 9)
        chart2DataType = (DataType)dataTypeIndex;
    if (freqIndex >= 0 && freqIndex < 2)
        chart2Frequency = (FrequencyBand)freqIndex;
}

void ChartManager::DrawChart1(HDC hdc)
{
    if (chart1Rect.right <= chart1Rect.left || chart1Rect.bottom <= chart1Rect.top)
        return;
    
    // Заливаем фон графика белым цветом
    HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(hdc, &chart1Rect, hBrush);
    DeleteObject(hBrush);
        
    // Рисуем рамку графика
    Rectangle(hdc, chart1Rect.left, chart1Rect.top, chart1Rect.right, chart1Rect.bottom);
    
    // Заголовок графика
    std::wstring title = L"График 1: " + GetDataTypeName(chart1DataType);
    if (chart1Frequency == FrequencyBand::FREQ_400_KHZ)
        title += L" (400 кГц)";
    else
        title += L" (2000 кГц)";
        
    TextOut(hdc, chart1Rect.left + 10, chart1Rect.top + 10, title.c_str(), (int)title.length());
    
    if (data.empty())
    {
        std::wstring noData = L"Нет данных. Загрузите файл через меню Файл -> Открыть файл данных...";
        TextOut(hdc, chart1Rect.left + 10, chart1Rect.top + 40, noData.c_str(), (int)noData.length());
        return;
    }
    
    // Вычисляем данные для отображения
    std::vector<std::vector<float>> chartData;
    CalculateChartData(chart1DataType, chart1Frequency, chartData);
    
    // Рисуем график
    DrawChart(hdc, chart1Rect, chartData, chart1DataType);
}

void ChartManager::DrawChart2(HDC hdc)
{
    if (!dualMode || chart2Rect.right <= chart2Rect.left || chart2Rect.bottom <= chart2Rect.top)
        return;
    
    // Заливаем фон графика белым цветом
    HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 255));
    FillRect(hdc, &chart2Rect, hBrush);
    DeleteObject(hBrush);
        
    // Рисуем рамку графика
    Rectangle(hdc, chart2Rect.left, chart2Rect.top, chart2Rect.right, chart2Rect.bottom);
    
    // Заголовок графика
    std::wstring title = L"График 2: " + GetDataTypeName(chart2DataType);
    if (chart2Frequency == FrequencyBand::FREQ_400_KHZ)
        title += L" (400 кГц)";
    else
        title += L" (2000 кГц)";
        
    TextOut(hdc, chart2Rect.left + 10, chart2Rect.top + 10, title.c_str(), (int)title.length());
    
    if (data.empty())
    {
        std::wstring noData = L"Нет данных";
        TextOut(hdc, chart2Rect.left + 10, chart2Rect.top + 40, noData.c_str(), (int)noData.length());
        return;
    }
    
    // Вычисляем данные для отображения
    std::vector<std::vector<float>> chartData;
    CalculateChartData(chart2DataType, chart2Frequency, chartData);
    
    // Рисуем график
    DrawChart(hdc, chart2Rect, chartData, chart2DataType);
}

void ChartManager::LoadDataFromFile(const std::wstring& filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
        return;
        
    data.clear();
    
    GP_TOOL_DATA record;
    while (file.read(reinterpret_cast<char*>(&record), sizeof(GP_TOOL_DATA)))
    {
        data.push_back(record);
    }
    
    file.close();
}

void ChartManager::HandleCommand(WPARAM wParam, LPARAM lParam)
{
    int wmId = LOWORD(wParam);
    int wmEvent = HIWORD(wParam);
    
    switch (wmId)
    {
    case IDC_DUAL_MODE_CHECK:
        if (wmEvent == BN_CLICKED)
        {
            ToggleDualMode();
            UpdateLayout(GetParent((HWND)lParam));
        }
        break;
    case IDC_CHART1_REFRESH_BTN:
        if (wmEvent == BN_CLICKED)
            RefreshChart1();
        break;
    case IDC_CHART2_REFRESH_BTN:
        if (wmEvent == BN_CLICKED)
            RefreshChart2();
        break;
    }
}

void ChartManager::CalculateChartData(DataType type, FrequencyBand freq, std::vector<std::vector<float>>& outData)
{
    outData.clear();
    
    if (data.empty())
        return;
    
    switch (type)
    {
    case DataType::PHASE_DIFFERENCE:
    case DataType::AMPLITUDE_RATIO:
    case DataType::AMPLITUDE_RX1:
    case DataType::AMPLITUDE_RX2:
    case DataType::PHASE_RX1:
    case DataType::PHASE_RX2:
        // 4 канала для активных передатчиков
        outData.resize(4);
        for (int channel = 0; channel < 4; channel++)
        {
            outData[channel].reserve(data.size());
            for (const auto& record : data)
            {
                int index = GetActiveTransmitterIndex(channel, freq);
                float value = 0.0f;
                
                switch (type)
                {
                case DataType::PHASE_DIFFERENCE:
                    value = record.PH_RX_2[index] - record.PH_RX_1[index];
                    break;
                case DataType::AMPLITUDE_RATIO:
                    value = (record.AM_RX_1[index] != 0.0f) ? record.AM_RX_2[index] / record.AM_RX_1[index] : 0.0f;
                    break;
                case DataType::AMPLITUDE_RX1:
                    value = record.AM_RX_1[index];
                    break;
                case DataType::AMPLITUDE_RX2:
                    value = record.AM_RX_2[index];
                    break;
                case DataType::PHASE_RX1:
                    value = record.PH_RX_1[index];
                    break;
                case DataType::PHASE_RX2:
                    value = record.PH_RX_2[index];
                    break;
                }
                
                outData[channel].push_back(value);
            }
        }
        break;
        
    case DataType::SILENT_AMPLITUDE_RX1:
    case DataType::SILENT_AMPLITUDE_RX2:
        // 1 канал для молчащих передатчиков
        outData.resize(1);
        outData[0].reserve(data.size());
        for (const auto& record : data)
        {
            int index = GetSilentTransmitterIndex(freq);
            float value = (type == DataType::SILENT_AMPLITUDE_RX1) ? 
                record.AM_RX_1[index] : record.AM_RX_2[index];
            outData[0].push_back(value);
        }
        break;
        
    case DataType::TEMPERATURE:
        // 1 значение температуры
        outData.resize(1);
        outData[0].reserve(data.size());
        for (const auto& record : data)
        {
            outData[0].push_back(record.temperature);
        }
        break;
    }
}

void ChartManager::DrawChart(HDC hdc, const RECT& rect, const std::vector<std::vector<float>>& chartData, DataType type)
{
    if (chartData.empty())
        return;
        
    // Цвета для разных каналов
    COLORREF colors[] = { RGB(255, 0, 0), RGB(0, 255, 0), RGB(0, 0, 255), RGB(255, 165, 0) };
    
    // Область для рисования графика (с отступами для осей)
    RECT plotRect = rect;
    plotRect.left += 50;
    plotRect.top += 40;
    plotRect.right -= 20;
    plotRect.bottom -= 30;
    
    if (plotRect.right <= plotRect.left || plotRect.bottom <= plotRect.top)
        return;
    
    // Находим минимальные и максимальные значения
    float minVal = FLT_MAX, maxVal = -FLT_MAX;
    for (const auto& channel : chartData)
    {
        for (float val : channel)
        {
            minVal = std::min(minVal, val);
            maxVal = std::max(maxVal, val);
        }
    }
    
    if (minVal == maxVal)
    {
        minVal -= 1.0f;
        maxVal += 1.0f;
    }
    
    // Рисуем оси
    MoveToEx(hdc, plotRect.left, plotRect.bottom, NULL);
    LineTo(hdc, plotRect.right, plotRect.bottom); // X-ось
    MoveToEx(hdc, plotRect.left, plotRect.top, NULL);
    LineTo(hdc, plotRect.left, plotRect.bottom); // Y-ось
    
    // Рисуем данные
    for (size_t channelIdx = 0; channelIdx < chartData.size(); channelIdx++)
    {
        const auto& channel = chartData[channelIdx];
        if (channel.empty())
            continue;
            
        HPEN hPen = CreatePen(PS_SOLID, 1, colors[channelIdx % 4]);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        
        bool firstPoint = true;
        for (size_t i = 0; i < channel.size(); i++)
        {
            int x = plotRect.left + (int)((double)i / (channel.size() - 1) * (plotRect.right - plotRect.left));
            int y = plotRect.bottom - (int)((channel[i] - minVal) / (maxVal - minVal) * (plotRect.bottom - plotRect.top));
            
            if (firstPoint)
            {
                MoveToEx(hdc, x, y, NULL);
                firstPoint = false;
            }
            else
            {
                LineTo(hdc, x, y);
            }
        }
        
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
    }
    
    // Рисуем легенду для многоканальных данных
    if (chartData.size() > 1)
    {
        const wchar_t* channelNames[] = { L"T1", L"T2", L"T3", L"T4" };
        for (size_t i = 0; i < chartData.size() && i < 4; i++)
        {
            HPEN hPen = CreatePen(PS_SOLID, 2, colors[i]);
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
            
            int legendY = rect.top + 25 + (int)i * 15;
            MoveToEx(hdc, rect.right - 80, legendY, NULL);
            LineTo(hdc, rect.right - 60, legendY);
            
            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
            
            TextOut(hdc, rect.right - 55, legendY - 7, channelNames[i], 2);
        }
    }
}

std::wstring ChartManager::GetDataTypeName(DataType type)
{
    switch (type)
    {
    case DataType::PHASE_DIFFERENCE: return L"Разность фаз";
    case DataType::AMPLITUDE_RATIO: return L"Отношение амплитуд";
    case DataType::AMPLITUDE_RX1: return L"Амплитуды RX1";
    case DataType::AMPLITUDE_RX2: return L"Амплитуды RX2";
    case DataType::PHASE_RX1: return L"Фазы RX1";
    case DataType::PHASE_RX2: return L"Фазы RX2";
    case DataType::SILENT_AMPLITUDE_RX1: return L"Молчащие амплитуды RX1";
    case DataType::SILENT_AMPLITUDE_RX2: return L"Молчащие амплитуды RX2";
    case DataType::TEMPERATURE: return L"Температура";
    default: return L"Неизвестный тип";
    }
}

int ChartManager::GetActiveTransmitterIndex(int transmitter, FrequencyBand freq)
{
    // Индексы для активных передатчиков T1-T4
    if (freq == FrequencyBand::FREQ_400_KHZ)
        return 2 + transmitter; // индексы 2, 3, 4, 5
    else
        return 6 + transmitter; // индексы 6, 7, 8, 9
}

int ChartManager::GetSilentTransmitterIndex(FrequencyBand freq)
{
    // Индексы для молчащих передатчиков
    return (freq == FrequencyBand::FREQ_400_KHZ) ? 0 : 1;
}

// Вспомогательные методы для компоновки
void ChartManager::CalculateLayout(int windowWidth, int windowHeight)
{
    if (dualMode)
    {
        // Расчет для режима двух графиков
        // Доступное пространство = Общая высота - Фиксированные элементы
        int menuAndPanel1Height = 60; // Меню + панель управления первого графика
        int totalFixedHeight = menuAndPanel1Height + PANEL_HEIGHT + (4 * MARGIN) + PANEL_SPACING;
        int availableHeight = windowHeight - totalFixedHeight;
        
        // Каждый график получает равную долю доступного пространства
        int chartHeight = availableHeight / 2;
        
        // Убеждаемся, что высота графика не меньше минимальной
        if (chartHeight < MIN_CHART_HEIGHT)
            chartHeight = MIN_CHART_HEIGHT;
        
        // Первый график
        chart1Rect.left = MARGIN;
        chart1Rect.top = menuAndPanel1Height;
        chart1Rect.right = windowWidth - MARGIN;
        chart1Rect.bottom = chart1Rect.top + chartHeight;
        
        // Второй график (симметрично первому)
        chart2Rect.left = MARGIN;
        chart2Rect.top = chart1Rect.bottom + PANEL_HEIGHT + (2 * MARGIN);
        chart2Rect.right = windowWidth - MARGIN;
        chart2Rect.bottom = chart2Rect.top + chartHeight;
    }
    else
    {
        // Режим одного графика - занимает всё доступное пространство
        chart1Rect.left = MARGIN;
        chart1Rect.top = 60; // Меню + панель управления
        chart1Rect.right = windowWidth - MARGIN;
        chart1Rect.bottom = windowHeight - MARGIN;
        
        // Очищаем прямоугольник второго графика
        ZeroMemory(&chart2Rect, sizeof(RECT));
    }
}

void ChartManager::PositionChart1Controls()
{
    // Элементы управления первого графика уже позиционированы при создании
    // Этот метод может быть использован для динамического репозиционирования при необходимости
}

void ChartManager::PositionChart2Controls(int panel2Y)
{
    // ИСПРАВЛЕНИЕ Z-ПОРЯДКА: Используем HWND_TOP вместо SWP_NOZORDER
    SetWindowPos(hChart2DataCombo, HWND_TOP, 10, panel2Y, 180, 25, SWP_SHOWWINDOW);
    SetWindowPos(hChart2FreqCombo, HWND_TOP, 200, panel2Y, 100, 25, SWP_SHOWWINDOW);
    SetWindowPos(hChart2RefreshBtn, HWND_TOP, 310, panel2Y, 80, 25, SWP_SHOWWINDOW);
}

bool ChartManager::ValidateLayoutConstraints(int width, int height)
{
    // Минимальные размеры окна для корректной работы
    const int MIN_WINDOW_WIDTH = 600;
    const int MIN_WINDOW_HEIGHT = 400;
    
    if (width < MIN_WINDOW_WIDTH || height < MIN_WINDOW_HEIGHT)
        return false;
    
    // В режиме двух графиков нужно больше места
    if (dualMode)
    {
        const int MIN_DUAL_HEIGHT = 600;
        if (height < MIN_DUAL_HEIGHT)
            return false;
    }
    
    return true;
}