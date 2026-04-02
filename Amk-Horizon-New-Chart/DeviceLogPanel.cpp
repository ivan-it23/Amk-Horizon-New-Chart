#include "framework.h"
#include "DeviceLogPanel.h"
#include "DeviceExchangeLog.h"
#include "UIModule.h"
#include "LayoutModule.h"
#include "resource.h"
#include <algorithm>
#include <commctrl.h>

static constexpr wchar_t kExpandedClassName[] = L"AmkHorizonDeviceLogExpandedFrame";

static HINSTANCE g_hInst = nullptr;
static HWND g_hMain = nullptr;

static HWND g_hDockHost = nullptr;
static HWND g_hDockEdit = nullptr;
static HWND g_hDockExpandBtn = nullptr;
static HWND g_hDockClearBtn = nullptr;

static HWND g_hExpandedRoot = nullptr;
static HWND g_hExpandedEdit = nullptr;
static HWND g_hExpandedClearBtn = nullptr;
static HWND g_hExpandedCollapseBtn = nullptr;

static bool g_dockVisible = false;
static bool g_expandedVisible = false;

static bool g_classesRegistered = false;

static int EditGetVisibleLineCount(HWND hEdit) {
    RECT rcFmt{};
    SendMessageW(hEdit, EM_GETRECT, 0, reinterpret_cast<LPARAM>(&rcFmt));
    HFONT hf = reinterpret_cast<HFONT>(SendMessageW(hEdit, WM_GETFONT, 0, 0));
    HDC hdc = GetDC(hEdit);
    if (hdc) {
        if (hf) {
            SelectObject(hdc, hf);
        }
        TEXTMETRICW tm{};
        if (GetTextMetricsW(hdc, &tm) && tm.tmHeight > 0) {
            ReleaseDC(hEdit, hdc);
            const int h = tm.tmHeight;
            const int areaH = static_cast<int>(rcFmt.bottom - rcFmt.top);
            return (std::max)(1, areaH / h);
        }
        ReleaseDC(hEdit, hdc);
    }
    return 1;
}

// Пользователь смотрит «хвост» лога: нижняя часть текста в поле видна (как после ручной прокрутки вниз).
static bool EditIsPinnedToBottom(HWND hEdit) {
    const int total = static_cast<int>(SendMessageW(hEdit, EM_GETLINECOUNT, 0, 0));
    if (total <= 0) {
        return true;
    }
    const int firstVis = static_cast<int>(SendMessageW(hEdit, EM_GETFIRSTVISIBLELINE, 0, 0));
    const int vis = EditGetVisibleLineCount(hEdit);
    return firstVis + vis >= total - 1;
}

static void EditApplyFullText(HWND hEdit, const wchar_t* text) {
    if (!hEdit || !IsWindow(hEdit)) {
        return;
    }
    const bool pinBottom = EditIsPinnedToBottom(hEdit);
    const int savedFirstLine = static_cast<int>(SendMessageW(hEdit, EM_GETFIRSTVISIBLELINE, 0, 0));

    SetWindowTextW(hEdit, text);

    if (pinBottom) {
        // SetWindowText сбрасывает прокрутку; для многострочного EDIT EM_SETSEL(-1,-1) часто не опускает вид.
        SendMessageW(hEdit, WM_VSCROLL, SB_BOTTOM, 0);
        const int len = GetWindowTextLengthW(hEdit);
        SendMessageW(hEdit, EM_SETSEL, static_cast<WPARAM>(len), static_cast<LPARAM>(len));
        SendMessageW(hEdit, EM_SCROLLCARET, 0, 0);
    } else if (savedFirstLine > 0) {
        const int total = static_cast<int>(SendMessageW(hEdit, EM_GETLINECOUNT, 0, 0));
        const int scrollTo = (total > 0) ? (std::min)(savedFirstLine, (std::max)(0, total - 1)) : 0;
        SendMessageW(hEdit, EM_LINESCROLL, 0, scrollTo);
    }
}

static void RefreshBothEdits() {
    const std::wstring text = DeviceExchangeLogGetFullText();
    if (g_hDockEdit && IsWindow(g_hDockEdit)) {
        EditApplyFullText(g_hDockEdit, text.c_str());
    }
    if (g_hExpandedEdit && IsWindow(g_hExpandedEdit)) {
        EditApplyFullText(g_hExpandedEdit, text.c_str());
    }
}

LRESULT CALLBACK DockHostSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
    UINT_PTR, DWORD_PTR dwRefData) {
    if (msg == WM_COMMAND) {
        SendMessageW(reinterpret_cast<HWND>(dwRefData), msg, wParam, lParam);
        return 0;
    }
    if (msg == WM_NCDESTROY) {
        RemoveWindowSubclass(hwnd, DockHostSubclassProc, 1);
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

static void LayoutDockChildren() {
    if (!g_hDockHost || !IsWindow(g_hDockHost)) {
        return;
    }
    RECT rc{};
    GetClientRect(g_hDockHost, &rc);
    const int btnH = 22;
    const int pad = 4;
    const int expandW = 28;
    const int clearW = 72;
    if (g_hDockExpandBtn) {
        SetWindowPos(g_hDockExpandBtn, nullptr, pad, pad, expandW, btnH, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (g_hDockClearBtn) {
        SetWindowPos(g_hDockClearBtn, nullptr, pad + expandW + 4, pad, clearW, btnH, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    const int topEdit = pad + btnH + 4;
    const int editH = std::max(0, static_cast<int>(rc.bottom) - topEdit - pad);
    if (g_hDockEdit) {
        SetWindowPos(g_hDockEdit, nullptr, pad, topEdit, std::max(10, static_cast<int>(rc.right) - 2 * pad), std::max(10, editH),
            SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

static void LayoutExpandedChildren(HWND hRoot) {
    RECT rc{};
    GetClientRect(hRoot, &rc);
    const int btnH = 26;
    const int pad = 8;
    const int clearW = 90;
    const int collapseW = 100;
    if (g_hExpandedClearBtn) {
        SetWindowPos(g_hExpandedClearBtn, nullptr, pad, pad, clearW, btnH, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (g_hExpandedCollapseBtn) {
        SetWindowPos(g_hExpandedCollapseBtn, nullptr, pad + clearW + 8, pad, collapseW, btnH, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    const int topEdit = pad + btnH + 8;
    if (g_hExpandedEdit) {
        SetWindowPos(g_hExpandedEdit, nullptr, pad, topEdit, std::max(10, static_cast<int>(rc.right) - 2 * pad),
            std::max(10, static_cast<int>(rc.bottom) - topEdit - pad), SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

LRESULT CALLBACK ExpandedFrameWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        HINSTANCE hi = cs->hInstance;
        g_hExpandedClearBtn = CreateWindowExW(0, L"BUTTON", L"Очистить",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 1, 1, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_DEVICE_LOG_CLEAR)),
            hi, nullptr);
        g_hExpandedCollapseBtn = CreateWindowExW(0, L"BUTTON", L"Свернуть",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 1, 1, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_DEVICE_LOG_COLLAPSE)),
            hi, nullptr);
        g_hExpandedEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_WANTRETURN,
            0, 0, 1, 1, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_DEVICE_LOG_EDIT_EXPANDED)), hi, nullptr);
        SendMessageW(g_hExpandedEdit, EM_SETLIMITTEXT, 1024 * 1024, 0);
        HFONT hf = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        SendMessageW(g_hExpandedEdit, WM_SETFONT, reinterpret_cast<WPARAM>(hf), TRUE);
        SendMessageW(g_hExpandedClearBtn, WM_SETFONT, reinterpret_cast<WPARAM>(hf), TRUE);
        SendMessageW(g_hExpandedCollapseBtn, WM_SETFONT, reinterpret_cast<WPARAM>(hf), TRUE);
        RefreshBothEdits();
        return 0;
    }
    case WM_SIZE:
        LayoutExpandedChildren(hWnd);
        return 0;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_DEVICE_LOG_CLEAR && HIWORD(wParam) == BN_CLICKED) {
            DeviceExchangeLogClear();
            RefreshBothEdits();
            return 0;
        }
        if (LOWORD(wParam) == IDC_DEVICE_LOG_COLLAPSE && HIWORD(wParam) == BN_CLICKED) {
            DestroyWindow(hWnd);
            return 0;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0;
    case WM_DESTROY:
        if (hWnd == g_hExpandedRoot) {
            g_hExpandedRoot = nullptr;
            g_hExpandedEdit = nullptr;
            g_hExpandedClearBtn = nullptr;
            g_hExpandedCollapseBtn = nullptr;
            g_expandedVisible = false;
            if (g_dockVisible && g_hDockHost && IsWindow(g_hDockHost)) {
                ShowWindow(g_hDockHost, SW_SHOW);
            }
        }
        break;
    default:
        break;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

static void RegisterExpandedClass(HINSTANCE hInst) {
    if (g_classesRegistered) {
        return;
    }
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = ExpandedFrameWndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.lpszClassName = kExpandedClassName;
    const ATOM a = RegisterClassExW(&wc);
    if (!a && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return;
    }
    g_classesRegistered = true;
}

static void DestroyDockControls() {
    if (g_hDockHost && IsWindow(g_hDockHost)) {
        DestroyWindow(g_hDockHost);
    }
    g_hDockHost = nullptr;
    g_hDockEdit = nullptr;
    g_hDockExpandBtn = nullptr;
    g_hDockClearBtn = nullptr;
}

static void CreateDockControls() {
    if (g_hDockHost && IsWindow(g_hDockHost)) {
        return;
    }
    // WS_POPUP + координаты экрана: верх совпадает с верхом окна (включая заголовок),
    // низ задаётся в UpdateLayout (чуть ниже верхней границы графика, выше подписи легенды).
    g_hDockHost = CreateWindowExW(WS_EX_CONTROLPARENT, L"STATIC", L"",
        WS_POPUP | WS_CLIPCHILDREN | WS_BORDER, 0, 0, 10, 10, g_hMain, nullptr, g_hInst, nullptr);
    SetWindowSubclass(g_hDockHost, DockHostSubclassProc, 1, reinterpret_cast<DWORD_PTR>(g_hMain));

    g_hDockExpandBtn = CreateWindowExW(0, L"BUTTON", L"\u2922",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 1, 1, g_hDockHost,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_DEVICE_LOG_EXPAND)), g_hInst, nullptr);
    g_hDockClearBtn = CreateWindowExW(0, L"BUTTON", L"Очистить",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 1, 1, g_hDockHost,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_DEVICE_LOG_CLEAR)), g_hInst, nullptr);

    g_hDockEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_WANTRETURN,
        0, 0, 1, 1, g_hDockHost, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_DEVICE_LOG_EDIT)), g_hInst, nullptr);
    SendMessageW(g_hDockEdit, EM_SETLIMITTEXT, 1024 * 1024, 0);

    HFONT hf = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
    SendMessageW(g_hDockEdit, WM_SETFONT, reinterpret_cast<WPARAM>(hf), TRUE);
    SendMessageW(g_hDockExpandBtn, WM_SETFONT, reinterpret_cast<WPARAM>(hf), TRUE);
    SendMessageW(g_hDockClearBtn, WM_SETFONT, reinterpret_cast<WPARAM>(hf), TRUE);

    RefreshBothEdits();
    LayoutDockChildren();
    SetWindowPos(g_hDockHost, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOACTIVATE);
}

static void ShowExpandedWindow() {
    if (g_hExpandedRoot && IsWindow(g_hExpandedRoot)) {
        SetForegroundWindow(g_hExpandedRoot);
        return;
    }

    RECT rcDock{};
    int baseW = 400;
    int baseH = 200;
    if (g_hDockHost && IsWindow(g_hDockHost)) {
        GetWindowRect(g_hDockHost, &rcDock);
        baseW = std::max(80, static_cast<int>(rcDock.right - rcDock.left));
        baseH = std::max(40, static_cast<int>(rcDock.bottom - rcDock.top));
    }

    int w = std::max(560, baseW * 2);
    int h = std::max(320, baseH * 2);

    HMONITOR mon = MonitorFromWindow(g_hMain, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = {};
    mi.cbSize = sizeof(mi);
    int x = 0, y = 0;
    if (mon && GetMonitorInfo(mon, &mi)) {
        const RECT& work = mi.rcWork;
        x = work.left + (work.right - work.left - w) / 2;
        y = work.top + (work.bottom - work.top - h) / 2;
    }

    g_hExpandedRoot = CreateWindowExW(WS_EX_DLGMODALFRAME, kExpandedClassName, L"Лог обмена с прибором",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_CLIPCHILDREN,
        x, y, w, h, g_hMain, nullptr, g_hInst, nullptr);
    if (g_hExpandedRoot) {
        ShowWindow(g_hExpandedRoot, SW_SHOW);
        g_expandedVisible = true;
        if (g_hDockHost && IsWindow(g_hDockHost)) {
            ShowWindow(g_hDockHost, SW_HIDE);
        }
    }
}

void DeviceLogPanel_Init(HWND hMain, HINSTANCE hInst) {
    g_hMain = hMain;
    g_hInst = hInst;
    RegisterExpandedClass(hInst);
}

void DeviceLogPanel_Shutdown(HWND) {
    if (g_hExpandedRoot && IsWindow(g_hExpandedRoot)) {
        DestroyWindow(g_hExpandedRoot);
    }
    g_hExpandedRoot = nullptr;
    DestroyDockControls();
    g_dockVisible = false;
    g_hMain = nullptr;
}

void DeviceLogPanel_Toggle(HWND hMain) {
    UNREFERENCED_PARAMETER(hMain);
    if (g_dockVisible) {
        g_dockVisible = false;
        if (g_hExpandedRoot && IsWindow(g_hExpandedRoot)) {
            DestroyWindow(g_hExpandedRoot);
        }
        DestroyDockControls();
        return;
    }
    g_dockVisible = true;
    CreateDockControls();
    ShowWindow(g_hDockHost, SW_SHOW);
}

bool DeviceLogPanel_IsDockVisible() {
    return g_dockVisible;
}

void DeviceLogPanel_UpdateLayout(HWND hMain, UIModule* ui, const LayoutModule* layout) {
    g_hMain = hMain;
    if (!g_dockVisible || !g_hDockHost || !IsWindow(g_hDockHost)) {
        return;
    }
    if (!ui || !layout) {
        return;
    }

    HWND hConnect = ui->GetConnectToggleBtn();
    HWND hCursorIcon = ui->GetChart1CursorIconHwnd();
    if (!hConnect || !hCursorIcon) {
        return;
    }

    RECT rcMain{};
    GetWindowRect(hMain, &rcMain);

    RECT rcConnect{};
    RECT rcIcon{};
    GetWindowRect(hConnect, &rcConnect);
    GetWindowRect(hCursorIcon, &rcIcon);

    // Горизонталь: экранные координаты — зазор между «Включить/Отключить» и иконкой курсора
    const int leftScreen = rcConnect.right + 6;
    const int rightScreen = rcIcon.left - 6;
    const int width = rightScreen - leftScreen;

    const RECT chart1 = layout->GetChart1Rect();
    // ChartModule: подписи легенды на chartRect.top + 35 — нижний край лога чуть выше, с небольшим заходом на область графика
    static constexpr int kLegendYFromChartTop = 35;
    static constexpr int kPixelsAboveLegendLine = 3;
    const int bottomClientY = static_cast<int>(chart1.top) + kLegendYFromChartTop - kPixelsAboveLegendLine;
    POINT ptBottom = { chart1.left, bottomClientY };
    ClientToScreen(hMain, &ptBottom);
    const int topScreen = rcMain.top;
    const int height = ptBottom.y - topScreen;

    const int kMinW = 80;
    const int kMinH = 40;
    if (width < kMinW || height < kMinH) {
        ShowWindow(g_hDockHost, SW_HIDE);
        return;
    }

    SetWindowPos(g_hDockHost, HWND_TOP, leftScreen, topScreen, width, height, SWP_NOACTIVATE);
    LayoutDockChildren();
}

void DeviceLogPanel_OnRefresh(HWND) {
    RefreshBothEdits();
}

// Точка входа из главного окна: развернуть из dock-кнопки
void DeviceLogPanel_OnExpandCommand() {
    ShowExpandedWindow();
}
