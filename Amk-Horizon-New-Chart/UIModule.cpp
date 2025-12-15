#include "framework.h"
#include "UIModule.h"

UIModule::UIModule()
    : hDualModeCheck(nullptr)
    , hChart1SchematicBtn(nullptr)
    , hChart2SchematicBtn(nullptr)
    , hChart1HScrollBar(nullptr)
    , hChart2HScrollBar(nullptr)
    , hChart1VScrollBar(nullptr)
    , hChart2VScrollBar(nullptr)
    , hXRangeFromLabel(nullptr)
    , hXRangeFromEdit(nullptr)
    , hXRangeToLabel(nullptr)
    , hXRangeToEdit(nullptr)
    , hXRangeApplyBtn(nullptr)
    , hXRangeResetBtn(nullptr)
    , hXRange2FromLabel(nullptr)
    , hXRange2FromEdit(nullptr)
    , hXRange2ToLabel(nullptr)
    , hXRange2ToEdit(nullptr)
    , hXRange2ApplyBtn(nullptr)
    , hXRange2ResetBtn(nullptr)
    , hSaveRange1Btn(nullptr)
    , hResetZoom1Btn(nullptr)
    , hResetZoom2Btn(nullptr)
    , hCursor1Label(nullptr)
    , hCursor1XLabel(nullptr)
    , hCursor1XEdit(nullptr)
    , hCursor1YLabel(nullptr)
    , hCursor1YEdit(nullptr)
    , hCursor2Label(nullptr)
    , hCursor2XLabel(nullptr)
    , hCursor2XEdit(nullptr)
    , hCursor2YLabel(nullptr)
    , hCursor2YEdit(nullptr)
    , hStatusBar(nullptr)
    , hChart1SurfCheck(nullptr)
    , hChart1SurfEdit(nullptr)
    , hChart1SurfLabel(nullptr)
    , hChart2SurfCheck(nullptr)
    , hChart2SurfEdit(nullptr)
    , hChart2SurfLabel(nullptr)
    , dualMode(false) {
    chart1EnabledTransmitters.resize(4, true); // Изначально все T1-T4 включены (для совместимости)
    chart2EnabledTransmitters.resize(4, true); // Изначально все T1-T4 включены (для совместимости)
}

UIModule::~UIModule() {
}

void UIModule::CreateControls(HWND hWnd, HINSTANCE hInst) {
    // Создание чекбокса Dual Mode
    hDualModeCheck = CreateWindow(L"BUTTON", L"Dual Mode", 
        WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
        10, 10, 100, 20, hWnd, (HMENU)IDC_DUAL_MODE_CHECK, hInst, NULL);
    
    // Создание кнопки Device Schematic для первого графика
    hChart1SchematicBtn = CreateWindow(L"BUTTON", L"Device Schematic",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        120, 10, 140, 25, hWnd, (HMENU)IDC_CHART1_SCHEMATIC_BTN, hInst, NULL);
    
    // Создание кнопки Device Schematic для второго графика (изначально скрыта)
    hChart2SchematicBtn = CreateWindow(L"BUTTON", L"Device Schematic",
        WS_CHILD | BS_PUSHBUTTON,
        10, 300, 140, 25, hWnd, (HMENU)IDC_CHART2_SCHEMATIC_BTN, hInst, NULL);

    // Создание горизонтальных скроллбаров (изначально скрыты)
    hChart1HScrollBar = CreateWindow(L"SCROLLBAR", NULL,
        WS_CHILD | SBS_HORZ,
        0, 0, 100, 20, hWnd, (HMENU)IDC_CHART1_HSCROLL, hInst, NULL);
    
    hChart2HScrollBar = CreateWindow(L"SCROLLBAR", NULL,
        WS_CHILD | SBS_HORZ,
        0, 0, 100, 20, hWnd, (HMENU)IDC_CHART2_HSCROLL, hInst, NULL);
    
    // Создание вертикальных скроллбаров (изначально скрыты)
    hChart1VScrollBar = CreateWindow(L"SCROLLBAR", NULL,
        WS_CHILD | SBS_VERT,
        0, 0, 20, 100, hWnd, (HMENU)IDC_CHART1_VSCROLL, hInst, NULL);
    
    hChart2VScrollBar = CreateWindow(L"SCROLLBAR", NULL,
        WS_CHILD | SBS_VERT,
        0, 0, 20, 100, hWnd, (HMENU)IDC_CHART2_VSCROLL, hInst, NULL);

    // Создание элементов управления X Range в панели инструментов
    // Размещаем их справа от кнопки Device Schematic
    int xRangeStartX = 280; // Начальная позиция X Range элементов
    int yPos = 10;          // Вертикальная позиция (та же, что у других элементов)
    
    // Метка "X Range:"
    hXRangeFromLabel = CreateWindow(L"STATIC", L"X Range:",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        xRangeStartX, yPos, 60, 20, hWnd, (HMENU)IDC_XRANGE_FROM_LABEL, hInst, NULL);
    
    // Поле ввода "From"
    hXRangeFromEdit = CreateWindow(L"EDIT", L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
        xRangeStartX + 65, yPos - 2, 60, 22, hWnd, (HMENU)IDC_XRANGE_FROM_EDIT, hInst, NULL);
    
    // Метка "to"
    hXRangeToLabel = CreateWindow(L"STATIC", L"to",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        xRangeStartX + 130, yPos, 20, 20, hWnd, (HMENU)IDC_XRANGE_TO_LABEL, hInst, NULL);
    
    // Поле ввода "To"
    hXRangeToEdit = CreateWindow(L"EDIT", L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
        xRangeStartX + 155, yPos - 2, 60, 22, hWnd, (HMENU)IDC_XRANGE_TO_EDIT, hInst, NULL);
    
    // Кнопка "Apply"
    hXRangeApplyBtn = CreateWindow(L"BUTTON", L"Apply",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        xRangeStartX + 225, yPos - 2, 50, 24, hWnd, (HMENU)IDC_XRANGE_APPLY_BTN, hInst, NULL);
    
    // Кнопка "Reset"
    hXRangeResetBtn = CreateWindow(L"BUTTON", L"Reset",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        xRangeStartX + 280, yPos - 2, 50, 24, hWnd, (HMENU)IDC_XRANGE_RESET_BTN, hInst, NULL);
    
    // Кнопка "Save Range" для Chart 1
    hSaveRange1Btn = CreateWindow(L"BUTTON", L"Save Range",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        xRangeStartX + 335, yPos - 2, 80, 24, hWnd, (HMENU)IDC_SAVE_RANGE1_BTN, hInst, NULL);
    
    // Кнопка "Reset Zoom" для Chart 1
    hResetZoom1Btn = CreateWindow(L"BUTTON", L"Reset Zoom",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        xRangeStartX + 420, yPos - 2, 80, 24, hWnd, (HMENU)IDC_RESET_ZOOM1_BTN, hInst, NULL);

    // Элементы отображения координат курсора для Chart 1
    int cursorStartX = xRangeStartX + 505;
    
    hCursor1Label = CreateWindow(L"STATIC", L"Cursor:",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        cursorStartX, yPos, 45, 20, hWnd, (HMENU)IDC_CURSOR1_LABEL, hInst, NULL);
    
    hCursor1XLabel = CreateWindow(L"STATIC", L"X:",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        cursorStartX + 50, yPos, 15, 20, hWnd, (HMENU)IDC_CURSOR1_X_LABEL, hInst, NULL);
    
    hCursor1XEdit = CreateWindow(L"EDIT", L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_READONLY,
        cursorStartX + 65, yPos - 2, 55, 22, hWnd, (HMENU)IDC_CURSOR1_X_EDIT, hInst, NULL);
    
    hCursor1YLabel = CreateWindow(L"STATIC", L"Y:",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        cursorStartX + 125, yPos, 15, 20, hWnd, (HMENU)IDC_CURSOR1_Y_LABEL, hInst, NULL);
    
    hCursor1YEdit = CreateWindow(L"EDIT", L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_READONLY,
        cursorStartX + 140, yPos - 2, 70, 22, hWnd, (HMENU)IDC_CURSOR1_Y_EDIT, hInst, NULL);

    // === Элементы управления режимом Surf для Chart 1 (v2.14.0) ===
    int surfStartX = cursorStartX + 220;
    
    // Checkbox "Surf"
    hChart1SurfCheck = CreateWindow(L"BUTTON", L"Surf",
        WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
        surfStartX, yPos, 50, 20, hWnd, (HMENU)IDC_CHART1_SURF_CHECK, hInst, NULL);
    
    // Поле ввода количества точек (по умолчанию 100)
    hChart1SurfEdit = CreateWindow(L"EDIT", L"100",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER | ES_CENTER,
        surfStartX + 55, yPos - 2, 45, 22, hWnd, (HMENU)IDC_CHART1_SURF_EDIT, hInst, NULL);
    
    // Метка "pts"
    hChart1SurfLabel = CreateWindow(L"STATIC", L"pts",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        surfStartX + 105, yPos, 25, 20, hWnd, (HMENU)IDC_CHART1_SURF_LABEL, hInst, NULL);

    // Создание элементов управления X Range для Chart 2 (изначально скрыты)
    // Размещаем их рядом с кнопкой Device Schematic для Chart 2
    int xRange2StartX = 160; // Позиция после кнопки Device Schematic для Chart 2
    
    // Метка "X Range:" для Chart 2
    hXRange2FromLabel = CreateWindow(L"STATIC", L"X Range:",
        WS_CHILD | SS_LEFT,
        xRange2StartX, 300, 60, 20, hWnd, (HMENU)IDC_XRANGE2_FROM_LABEL, hInst, NULL);
    
    // Поле ввода "From" для Chart 2
    hXRange2FromEdit = CreateWindow(L"EDIT", L"",
        WS_CHILD | WS_BORDER | ES_NUMBER,
        xRange2StartX + 65, 298, 60, 22, hWnd, (HMENU)IDC_XRANGE2_FROM_EDIT, hInst, NULL);
    
    // Метка "to" для Chart 2
    hXRange2ToLabel = CreateWindow(L"STATIC", L"to",
        WS_CHILD | SS_CENTER,
        xRange2StartX + 130, 300, 20, 20, hWnd, (HMENU)IDC_XRANGE2_TO_LABEL, hInst, NULL);
    
    // Поле ввода "To" для Chart 2
    hXRange2ToEdit = CreateWindow(L"EDIT", L"",
        WS_CHILD | WS_BORDER | ES_NUMBER,
        xRange2StartX + 155, 298, 60, 22, hWnd, (HMENU)IDC_XRANGE2_TO_EDIT, hInst, NULL);
    
    // Кнопка "Apply" для Chart 2
    hXRange2ApplyBtn = CreateWindow(L"BUTTON", L"Apply",
        WS_CHILD | BS_PUSHBUTTON,
        xRange2StartX + 225, 298, 50, 24, hWnd, (HMENU)IDC_XRANGE2_APPLY_BTN, hInst, NULL);
    
    // Кнопка "Reset" для Chart 2
    hXRange2ResetBtn = CreateWindow(L"BUTTON", L"Reset",
        WS_CHILD | BS_PUSHBUTTON,
        xRange2StartX + 280, 298, 50, 24, hWnd, (HMENU)IDC_XRANGE2_RESET_BTN, hInst, NULL);
    
    // Кнопка "Reset Zoom" для Chart 2 (изначально скрыта)
    hResetZoom2Btn = CreateWindow(L"BUTTON", L"Reset Zoom",
        WS_CHILD | BS_PUSHBUTTON,
        xRange2StartX + 335, 298, 80, 24, hWnd, (HMENU)IDC_RESET_ZOOM2_BTN, hInst, NULL);

    // Элементы отображения координат курсора для Chart 2 (изначально скрыты)
    int cursor2StartX = xRange2StartX + 420;
    
    hCursor2Label = CreateWindow(L"STATIC", L"Cursor:",
        WS_CHILD | SS_LEFT,
        cursor2StartX, 300, 45, 20, hWnd, (HMENU)IDC_CURSOR2_LABEL, hInst, NULL);
    
    hCursor2XLabel = CreateWindow(L"STATIC", L"X:",
        WS_CHILD | SS_LEFT,
        cursor2StartX + 50, 300, 15, 20, hWnd, (HMENU)IDC_CURSOR2_X_LABEL, hInst, NULL);
    
    hCursor2XEdit = CreateWindow(L"EDIT", L"",
        WS_CHILD | WS_BORDER | ES_READONLY,
        cursor2StartX + 65, 298, 55, 22, hWnd, (HMENU)IDC_CURSOR2_X_EDIT, hInst, NULL);
    
    hCursor2YLabel = CreateWindow(L"STATIC", L"Y:",
        WS_CHILD | SS_LEFT,
        cursor2StartX + 125, 300, 15, 20, hWnd, (HMENU)IDC_CURSOR2_Y_LABEL, hInst, NULL);
    
    hCursor2YEdit = CreateWindow(L"EDIT", L"",
        WS_CHILD | WS_BORDER | ES_READONLY,
        cursor2StartX + 140, 298, 70, 22, hWnd, (HMENU)IDC_CURSOR2_Y_EDIT, hInst, NULL);

    // === Элементы управления режимом Surf для Chart 2 (v2.14.0) ===
    int surf2StartX = cursor2StartX + 220;
    
    // Checkbox "Surf" для Chart 2 (изначально скрыт)
    hChart2SurfCheck = CreateWindow(L"BUTTON", L"Surf",
        WS_CHILD | BS_AUTOCHECKBOX,
        surf2StartX, 300, 50, 20, hWnd, (HMENU)IDC_CHART2_SURF_CHECK, hInst, NULL);
    
    // Поле ввода количества точек для Chart 2 (по умолчанию 100)
    hChart2SurfEdit = CreateWindow(L"EDIT", L"100",
        WS_CHILD | WS_BORDER | ES_NUMBER | ES_CENTER,
        surf2StartX + 55, 298, 45, 22, hWnd, (HMENU)IDC_CHART2_SURF_EDIT, hInst, NULL);
    
    // Метка "pts" для Chart 2
    hChart2SurfLabel = CreateWindow(L"STATIC", L"pts",
        WS_CHILD | SS_LEFT,
        surf2StartX + 105, 300, 25, 20, hWnd, (HMENU)IDC_CHART2_SURF_LABEL, hInst, NULL);

    // Создание статусной строки
    hStatusBar = CreateWindowEx(
        0,                              // Расширенный стиль
        STATUSCLASSNAME,                // Класс статусной строки
        NULL,                           // Текст
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, // Стили
        0, 0, 0, 0,                     // Позиция и размер (автоматически)
        hWnd,                           // Родительское окно
        (HMENU)IDC_STATUSBAR,           // ID
        hInst,                          // Instance
        NULL);                          // Дополнительные параметры

    // Настройка частей статусной строки (4 части)
    int statusBarParts[4] = { 150, 350, 550, -1 }; // -1 означает "до конца"
    SendMessage(hStatusBar, SB_SETPARTS, 4, (LPARAM)statusBarParts);
    
    // Установка начального текста
    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)L"Ready");
    SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM)L"");
    SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM)L"");
    SendMessage(hStatusBar, SB_SETTEXT, 3, (LPARAM)L"No file loaded");
}

void UIModule::UpdateControlsVisibility(bool showChart2Controls) {
    int showFlag = showChart2Controls ? SW_SHOW : SW_HIDE;
    ShowWindow(hChart2SchematicBtn, showFlag);
    
    // Показываем/скрываем элементы управления X Range для Chart 2
    ShowWindow(hXRange2FromLabel, showFlag);
    ShowWindow(hXRange2FromEdit, showFlag);
    ShowWindow(hXRange2ToLabel, showFlag);
    ShowWindow(hXRange2ToEdit, showFlag);
    ShowWindow(hXRange2ApplyBtn, showFlag);
    ShowWindow(hXRange2ResetBtn, showFlag);
    ShowWindow(hResetZoom2Btn, showFlag);
    
    // Показываем/скрываем элементы координат курсора для Chart 2
    ShowWindow(hCursor2Label, showFlag);
    ShowWindow(hCursor2XLabel, showFlag);
    ShowWindow(hCursor2XEdit, showFlag);
    ShowWindow(hCursor2YLabel, showFlag);
    ShowWindow(hCursor2YEdit, showFlag);
    
    // Показываем/скрываем элементы Surf для Chart 2 (v2.14.0)
    ShowWindow(hChart2SurfCheck, showFlag);
    ShowWindow(hChart2SurfEdit, showFlag);
    ShowWindow(hChart2SurfLabel, showFlag);
}

void UIModule::PositionChart2Controls(int panel2Y) {
    SetWindowPos(hChart2SchematicBtn, HWND_TOP, 10, panel2Y, 140, 25, SWP_NOSIZE);
    
    // Позиционируем элементы управления X Range для Chart 2
    int xRange2StartX = 160;
    int yPos = panel2Y + 2; // Выравниваем с кнопкой Device Schematic
    
    SetWindowPos(hXRange2FromLabel, HWND_TOP, xRange2StartX, yPos, 60, 20, SWP_NOSIZE);
    SetWindowPos(hXRange2FromEdit, HWND_TOP, xRange2StartX + 65, yPos - 2, 60, 22, SWP_NOSIZE);
    SetWindowPos(hXRange2ToLabel, HWND_TOP, xRange2StartX + 130, yPos, 20, 20, SWP_NOSIZE);
    SetWindowPos(hXRange2ToEdit, HWND_TOP, xRange2StartX + 155, yPos - 2, 60, 22, SWP_NOSIZE);
    SetWindowPos(hXRange2ApplyBtn, HWND_TOP, xRange2StartX + 225, yPos - 2, 50, 24, SWP_NOSIZE);
    SetWindowPos(hXRange2ResetBtn, HWND_TOP, xRange2StartX + 280, yPos - 2, 50, 24, SWP_NOSIZE);
    SetWindowPos(hResetZoom2Btn, HWND_TOP, xRange2StartX + 335, yPos - 2, 80, 24, SWP_NOSIZE);
    
    // Позиционируем элементы координат курсора для Chart 2
    int cursor2StartX = xRange2StartX + 420;
    SetWindowPos(hCursor2Label, HWND_TOP, cursor2StartX, yPos, 45, 20, SWP_NOSIZE);
    SetWindowPos(hCursor2XLabel, HWND_TOP, cursor2StartX + 50, yPos, 15, 20, SWP_NOSIZE);
    SetWindowPos(hCursor2XEdit, HWND_TOP, cursor2StartX + 65, yPos - 2, 55, 22, SWP_NOSIZE);
    SetWindowPos(hCursor2YLabel, HWND_TOP, cursor2StartX + 125, yPos, 15, 20, SWP_NOSIZE);
    SetWindowPos(hCursor2YEdit, HWND_TOP, cursor2StartX + 140, yPos - 2, 70, 22, SWP_NOSIZE);
    
    // Позиционируем элементы Surf для Chart 2 (v2.14.0)
    int surf2StartX = cursor2StartX + 220;
    SetWindowPos(hChart2SurfCheck, HWND_TOP, surf2StartX, yPos, 50, 20, SWP_NOSIZE);
    SetWindowPos(hChart2SurfEdit, HWND_TOP, surf2StartX + 55, yPos - 2, 45, 22, SWP_NOSIZE);
    SetWindowPos(hChart2SurfLabel, HWND_TOP, surf2StartX + 105, yPos, 25, 20, SWP_NOSIZE);
}

void UIModule::ToggleDualMode() {
    dualMode = !dualMode;
    SendMessage(hDualModeCheck, BM_SETCHECK, dualMode ? BST_CHECKED : BST_UNCHECKED, 0);
}



void UIModule::SetChart1TransmitterEnabled(int transmitterIdx, bool enabled) {
    if (transmitterIdx >= 0 && transmitterIdx < chart1EnabledTransmitters.size()) {
        chart1EnabledTransmitters[transmitterIdx] = enabled;
    }
}

void UIModule::SetChart2TransmitterEnabled(int transmitterIdx, bool enabled) {
    if (transmitterIdx >= 0 && transmitterIdx < chart2EnabledTransmitters.size()) {
        chart2EnabledTransmitters[transmitterIdx] = enabled;
    }
}

bool UIModule::HandleCommand(WPARAM wParam, LPARAM lParam, HWND hWnd) {
    int wmId = LOWORD(wParam);
    int wmEvent = HIWORD(wParam);
    bool needsRedraw = false;

    switch (wmId) {
    case IDC_DUAL_MODE_CHECK:
        if (wmEvent == BN_CLICKED) {
            ToggleDualMode();
            needsRedraw = true;
        }
        break;
    
    case IDC_XRANGE_APPLY_BTN:
        if (wmEvent == BN_CLICKED) {
            // Обработка будет в ChartManager
            needsRedraw = true;
        }
        break;
    
    case IDC_XRANGE_RESET_BTN:
        if (wmEvent == BN_CLICKED) {
            // Обработка будет в ChartManager
            needsRedraw = true;
        }
        break;
    
    case IDC_XRANGE2_APPLY_BTN:
        if (wmEvent == BN_CLICKED) {
            // Обработка будет в ChartManager
            needsRedraw = true;
        }
        break;
    
    case IDC_XRANGE2_RESET_BTN:
        if (wmEvent == BN_CLICKED) {
            // Обработка будет в ChartManager
            needsRedraw = true;
        }
        break;

    }
    return needsRedraw;
}



// Методы для управления скроллбарами
void UIModule::ShowScrollBars(int chartNumber, bool show) {
    int showFlag = show ? SW_SHOW : SW_HIDE;
    
    if (chartNumber == 1) {
        ShowWindow(hChart1HScrollBar, showFlag);
        ShowWindow(hChart1VScrollBar, showFlag);
    } else if (chartNumber == 2) {
        ShowWindow(hChart2HScrollBar, showFlag);
        ShowWindow(hChart2VScrollBar, showFlag);
    }
}

void UIModule::UpdateScrollBarPositions(int chartNumber, const RECT& chartRect) {
    const int SCROLLBAR_SIZE = 15; // Уменьшенный размер скроллбара
    const int MARGIN_LEFT = 50;    // Отступ слева (для Y-оси)
    const int MARGIN_RIGHT = 20;   // Отступ справа
    const int MARGIN_BOTTOM = 5;   // Отступ снизу от края графика
    const int MARGIN_TOP = 60;     // Отступ сверху (для заголовка)
    
    if (chartNumber == 1) {
        // Горизонтальный скроллбар располагается ВНУТРИ области графика, в нижней части
        SetWindowPos(hChart1HScrollBar, HWND_TOP, 
                     chartRect.left + MARGIN_LEFT, 
                     chartRect.bottom - SCROLLBAR_SIZE - MARGIN_BOTTOM,
                     chartRect.right - chartRect.left - MARGIN_LEFT - MARGIN_RIGHT - SCROLLBAR_SIZE, 
                     SCROLLBAR_SIZE,
                     SWP_NOZORDER);
        
        // Вертикальный скроллбар располагается справа от графика
        SetWindowPos(hChart1VScrollBar, HWND_TOP,
                     chartRect.right - SCROLLBAR_SIZE - MARGIN_RIGHT,
                     chartRect.top + MARGIN_TOP,
                     SCROLLBAR_SIZE,
                     chartRect.bottom - chartRect.top - MARGIN_TOP - MARGIN_BOTTOM - SCROLLBAR_SIZE,
                     SWP_NOZORDER);
    } else if (chartNumber == 2) {
        // Горизонтальный скроллбар для второго графика
        SetWindowPos(hChart2HScrollBar, HWND_TOP,
                     chartRect.left + MARGIN_LEFT, 
                     chartRect.bottom - SCROLLBAR_SIZE - MARGIN_BOTTOM,
                     chartRect.right - chartRect.left - MARGIN_LEFT - MARGIN_RIGHT - SCROLLBAR_SIZE, 
                     SCROLLBAR_SIZE,
                     SWP_NOZORDER);
        
        // Вертикальный скроллбар для второго графика
        SetWindowPos(hChart2VScrollBar, HWND_TOP,
                     chartRect.right - SCROLLBAR_SIZE - MARGIN_RIGHT,
                     chartRect.top + MARGIN_TOP,
                     SCROLLBAR_SIZE,
                     chartRect.bottom - chartRect.top - MARGIN_TOP - MARGIN_BOTTOM - SCROLLBAR_SIZE,
                     SWP_NOZORDER);
    }
}

void UIModule::SetScrollBarRange(int chartNumber, int min, int max, int page, int pos) {
    HWND hScrollBar = nullptr;
    
    if (chartNumber == 1) {
        hScrollBar = hChart1HScrollBar;
    } else if (chartNumber == 2) {
        hScrollBar = hChart2HScrollBar;
    }
    
    if (hScrollBar) {
        SCROLLINFO si = {};
        si.cbSize = sizeof(SCROLLINFO);
        si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        si.nMin = min;
        si.nMax = max;
        si.nPage = page;
        si.nPos = pos;
        SetScrollInfo(hScrollBar, SB_CTL, &si, TRUE);
    }
}

HWND UIModule::GetHScrollBar(int chartNumber) const {
    if (chartNumber == 1) return hChart1HScrollBar;
    if (chartNumber == 2) return hChart2HScrollBar;
    return nullptr;
}

HWND UIModule::GetVScrollBar(int chartNumber) const {
    if (chartNumber == 1) return hChart1VScrollBar;
    if (chartNumber == 2) return hChart2VScrollBar;
    return nullptr;
}

void UIModule::SetVScrollBarRange(int chartNumber, int min, int max, int page, int pos) {
    HWND hScrollBar = nullptr;
    
    if (chartNumber == 1) {
        hScrollBar = hChart1VScrollBar;
    } else if (chartNumber == 2) {
        hScrollBar = hChart2VScrollBar;
    }
    
    if (hScrollBar) {
        SCROLLINFO si = {};
        si.cbSize = sizeof(SCROLLINFO);
        si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        si.nMin = min;
        si.nMax = max;
        si.nPage = page;
        si.nPos = pos;
        SetScrollInfo(hScrollBar, SB_CTL, &si, TRUE);
    }
}

// Методы для управления X Range
void UIModule::UpdateXRangeControls(int chartNumber, double minX, double maxX, bool isCustomRange) {
    HWND hFromEdit, hToEdit, hResetBtn;
    
    if (chartNumber == 1) {
        hFromEdit = hXRangeFromEdit;
        hToEdit = hXRangeToEdit;
        hResetBtn = hXRangeResetBtn;
    } else if (chartNumber == 2) {
        hFromEdit = hXRange2FromEdit;
        hToEdit = hXRange2ToEdit;
        hResetBtn = hXRange2ResetBtn;
    } else {
        return;
    }
    
    if (!hFromEdit || !hToEdit) return;
    
    // Обновляем поля ввода текущими значениями (целые числа)
    wchar_t fromText[32], toText[32];
    swprintf_s(fromText, L"%.0f", minX);
    swprintf_s(toText, L"%.0f", maxX);
    
    SetWindowText(hFromEdit, fromText);
    SetWindowText(hToEdit, toText);
    
    // Включаем/отключаем кнопку Reset в зависимости от состояния
    EnableWindow(hResetBtn, isCustomRange);
}

void UIModule::GetXRangeValues(int chartNumber, double& fromX, double& toX) const {
    HWND hFromEdit, hToEdit;
    
    if (chartNumber == 1) {
        hFromEdit = hXRangeFromEdit;
        hToEdit = hXRangeToEdit;
    } else if (chartNumber == 2) {
        hFromEdit = hXRange2FromEdit;
        hToEdit = hXRange2ToEdit;
    } else {
        fromX = toX = 0.0;
        return;
    }
    
    if (!hFromEdit || !hToEdit) {
        fromX = toX = 0.0;
        return;
    }
    
    wchar_t fromText[32], toText[32];
    GetWindowText(hFromEdit, fromText, 32);
    GetWindowText(hToEdit, toText, 32);
    
    fromX = _wtof(fromText);
    toX = _wtof(toText);
}

void UIModule::SetXRangeValues(int chartNumber, double fromX, double toX) {
    HWND hFromEdit, hToEdit;
    
    if (chartNumber == 1) {
        hFromEdit = hXRangeFromEdit;
        hToEdit = hXRangeToEdit;
    } else if (chartNumber == 2) {
        hFromEdit = hXRange2FromEdit;
        hToEdit = hXRange2ToEdit;
    } else {
        return;
    }
    
    if (!hFromEdit || !hToEdit) return;
    
    wchar_t fromText[32], toText[32];
    swprintf_s(fromText, L"%.0f", fromX);
    swprintf_s(toText, L"%.0f", toX);
    
    SetWindowText(hFromEdit, fromText);
    SetWindowText(hToEdit, toText);
}

void UIModule::ResetXRangeControls(int chartNumber) {
    HWND hFromEdit, hToEdit, hResetBtn;
    
    if (chartNumber == 1) {
        hFromEdit = hXRangeFromEdit;
        hToEdit = hXRangeToEdit;
        hResetBtn = hXRangeResetBtn;
    } else if (chartNumber == 2) {
        hFromEdit = hXRange2FromEdit;
        hToEdit = hXRange2ToEdit;
        hResetBtn = hXRange2ResetBtn;
    } else {
        return;
    }
    
    if (!hFromEdit || !hToEdit) return;
    
    // Очищаем поля ввода
    SetWindowText(hFromEdit, L"");
    SetWindowText(hToEdit, L"");
    
    // Отключаем кнопку Reset
    EnableWindow(hResetBtn, FALSE);
}

bool UIModule::IsXRangeToEmpty(int chartNumber) const {
    HWND hToEdit = nullptr;
    
    if (chartNumber == 1) {
        hToEdit = hXRangeToEdit;
    } else if (chartNumber == 2) {
        hToEdit = hXRange2ToEdit;
    } else {
        return true;
    }
    
    if (!hToEdit) return true;
    
    wchar_t toText[32];
    GetWindowText(hToEdit, toText, 32);
    
    // Проверяем, пустая ли строка или содержит только пробелы
    for (int i = 0; toText[i] != L'\0'; i++) {
        if (toText[i] != L' ' && toText[i] != L'\t') {
            return false;  // Найден непробельный символ
        }
    }
    return true;  // Строка пустая или содержит только пробелы
}


// Методы для управления статусной строкой
void UIModule::UpdateStatusBar(const std::wstring& text) {
    if (hStatusBar) {
        SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)text.c_str());
    }
}

void UIModule::UpdateStatusBarCoordinates(double x, double y1, const std::wstring& y1Unit,
                                         double y2, const std::wstring& y2Unit,
                                         bool hasDualAxis) {
    if (!hStatusBar) return;

    // Форматирование X координаты (целое число)
    wchar_t xText[64];
    swprintf_s(xText, L"X: %.0f", x);
    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)xText);

    // Форматирование Y координат
    if (hasDualAxis && !y2Unit.empty()) {
        // Двойные оси: Y₁ (левая ось) и Y₂ (правая ось)
        wchar_t y1Text[128];
        wchar_t y2Text[128];
        
        // Левая ось (фазы)
        if (y1Unit == L"mdeg") {
            swprintf_s(y1Text, L"Y\u2081: %.0f %s", y1, y1Unit.c_str());
        } else {
            swprintf_s(y1Text, L"Y\u2081: %.3f %s", y1, y1Unit.c_str());
        }
        
        // Правая ось (амплитуды)
        if (y2Unit == L"mV") {
            swprintf_s(y2Text, L"Y\u2082: %.3f %s", y2, y2Unit.c_str());
        } else {
            swprintf_s(y2Text, L"Y\u2082: %.3f %s", y2, y2Unit.c_str());
        }
        
        SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM)y1Text);
        SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM)y2Text);
    } else {
        // Одна ось
        wchar_t yText[128];
        
        if (y1Unit == L"mdeg") {
            swprintf_s(yText, L"Y: %.0f %s", y1, y1Unit.c_str());
        } else if (y1Unit == L"mV") {
            swprintf_s(yText, L"Y: %.3f %s", y1, y1Unit.c_str());
        } else if (y1Unit == L"V/V") {
            swprintf_s(yText, L"Y: %.4f %s", y1, y1Unit.c_str());
        } else if (y1Unit == L"°C") {
            swprintf_s(yText, L"Y: %.2f %s", y1, y1Unit.c_str());
        } else {
            swprintf_s(yText, L"Y: %.3f %s", y1, y1Unit.c_str());
        }
        
        SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM)yText);
        SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM)L"");
    }
}

void UIModule::ClearStatusBarCoordinates() {
    if (!hStatusBar) return;
    
    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)L"Ready");
    SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM)L"");
    SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM)L"");
}

// Обновление размера статусной строки при изменении размера окна
void UIModule::ResizeStatusBar() {
    if (hStatusBar) {
        // Отправляем сообщение WM_SIZE статусной строке для автоматического изменения размера
        SendMessage(hStatusBar, WM_SIZE, 0, 0);
    }
}

// Обновление координат курсора при клике на график
void UIModule::UpdateClickCoordinates(int chartNumber, double x, double y) {
    HWND hXEdit, hYEdit;
    
    if (chartNumber == 1) {
        hXEdit = hCursor1XEdit;
        hYEdit = hCursor1YEdit;
    } else if (chartNumber == 2) {
        hXEdit = hCursor2XEdit;
        hYEdit = hCursor2YEdit;
    } else {
        return;
    }
    
    if (!hXEdit || !hYEdit) return;
    
    // Форматируем координаты
    wchar_t xText[32], yText[32];
    swprintf_s(xText, L"%.0f", x);      // X как целое число (индекс кадра)
    swprintf_s(yText, L"%.2f", y);      // Y с 2 знаками после запятой
    
    SetWindowText(hXEdit, xText);
    SetWindowText(hYEdit, yText);
}

void UIModule::ClearClickCoordinates(int chartNumber) {
    HWND hXEdit, hYEdit;
    
    if (chartNumber == 1) {
        hXEdit = hCursor1XEdit;
        hYEdit = hCursor1YEdit;
    } else if (chartNumber == 2) {
        hXEdit = hCursor2XEdit;
        hYEdit = hCursor2YEdit;
    } else {
        return;
    }
    
    if (!hXEdit || !hYEdit) return;
    
    SetWindowText(hXEdit, L"");
    SetWindowText(hYEdit, L"");
}

// === Методы для работы с COM-портом (v2.13.0) ===
// Элементы управления COM-портом теперь в отдельном диалоге IDD_SERIAL_STATUS
// Эти методы оставлены пустыми для совместимости API

void UIModule::CreateSerialControls(HWND hWnd, HINSTANCE hInst) {
    // Элементы управления COM-портом теперь в меню "Прибор" и диалоге
    // Этот метод больше не создаёт элементы на панели инструментов
    hSerialPortCombo = nullptr;
    hSerialConnectBtn = nullptr;
    hSerialStatusLabel = nullptr;
    hSerialStartBtn = nullptr;
    hSerialStopBtn = nullptr;
    hSerialClearBtn = nullptr;
    hSerialFrameLabel = nullptr;
    hSerialPacketsLabel = nullptr;
}

void UIModule::UpdateSerialPortList(const std::vector<std::wstring>& ports) {
    // Реализация перенесена в диалог
}

std::wstring UIModule::GetSelectedPort() const {
    return L"";  // Реализация перенесена в диалог
}

void UIModule::SetSerialConnected(bool connected) {
    // Реализация перенесена в диалог
}

void UIModule::SetSerialPolling(bool polling) {
    // Реализация перенесена в диалог
}

void UIModule::UpdateSerialStatus(const std::wstring& status) {
    // Реализация перенесена в диалог
}

void UIModule::UpdateSerialFrameInfo(uint32_t frameNumber, uint32_t packetsReceived, uint32_t packetsErrors) {
    // Реализация перенесена в диалог
}

// === Методы для работы с режимом Surf (v2.14.0) ===

void UIModule::CreateSurfControls(HWND hWnd, HINSTANCE hInst) {
    // Контролы создаются в CreateControls(), этот метод для совместимости
}

void UIModule::UpdateSurfControlsVisibility(bool showChart2Controls) {
    int showFlag = showChart2Controls ? SW_SHOW : SW_HIDE;
    ShowWindow(hChart2SurfCheck, showFlag);
    ShowWindow(hChart2SurfEdit, showFlag);
    ShowWindow(hChart2SurfLabel, showFlag);
}

void UIModule::PositionSurfControls(int chart2PanelY) {
    // Позиционирование выполняется в PositionChart2Controls()
}

bool UIModule::IsSurfEnabled(int chartNumber) const {
    HWND hCheck = nullptr;
    
    if (chartNumber == 1) {
        hCheck = hChart1SurfCheck;
    } else if (chartNumber == 2) {
        hCheck = hChart2SurfCheck;
    }
    
    if (!hCheck) return false;
    
    return SendMessage(hCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

void UIModule::SetSurfEnabled(int chartNumber, bool enabled) {
    HWND hCheck = nullptr;
    
    if (chartNumber == 1) {
        hCheck = hChart1SurfCheck;
    } else if (chartNumber == 2) {
        hCheck = hChart2SurfCheck;
    }
    
    if (hCheck) {
        SendMessage(hCheck, BM_SETCHECK, enabled ? BST_CHECKED : BST_UNCHECKED, 0);
    }
}

int UIModule::GetSurfWindowSize(int chartNumber) const {
    HWND hEdit = nullptr;
    
    if (chartNumber == 1) {
        hEdit = hChart1SurfEdit;
    } else if (chartNumber == 2) {
        hEdit = hChart2SurfEdit;
    }
    
    if (!hEdit) return 100; // Значение по умолчанию
    
    wchar_t text[32];
    GetWindowText(hEdit, text, 32);
    
    int value = _wtoi(text);
    return (value > 0) ? value : 100; // Минимум 1, по умолчанию 100
}

void UIModule::SetSurfWindowSize(int chartNumber, int windowSize) {
    HWND hEdit = nullptr;
    
    if (chartNumber == 1) {
        hEdit = hChart1SurfEdit;
    } else if (chartNumber == 2) {
        hEdit = hChart2SurfEdit;
    }
    
    if (hEdit) {
        wchar_t text[32];
        swprintf_s(text, L"%d", windowSize);
        SetWindowText(hEdit, text);
    }
}

void UIModule::UpdateStatusBarSurfInfo(bool surf1Enabled, int surf1Window, bool surf2Enabled, int surf2Window) {
    if (!hStatusBar) return;
    
    // Формируем строку статуса Surf
    wchar_t surfText[128];
    
    if (surf1Enabled && surf2Enabled) {
        swprintf_s(surfText, L"Surf: C1 ON (%d) | C2 ON (%d)", surf1Window, surf2Window);
    } else if (surf1Enabled) {
        swprintf_s(surfText, L"Surf: C1 ON (%d) | C2 OFF", surf1Window);
    } else if (surf2Enabled) {
        swprintf_s(surfText, L"Surf: C1 OFF | C2 ON (%d)", surf2Window);
    } else {
        swprintf_s(surfText, L"Surf: OFF");
    }
    
    // Обновляем часть 2 статусной строки (индекс 2)
    SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM)surfText);
}
