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
    case WM_HSCROLL:
    case WM_VSCROLL:
        if (g_chartManager)
        {
            g_chartManager->HandleScrollEvent(hWnd, wParam, lParam);
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

// Основная логика приложения теперь находится в ChartManager