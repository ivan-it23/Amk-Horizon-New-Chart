#include "framework.h"
#include "UIModule.h"

UIModule::UIModule()
    : hDualModeCheck(nullptr)
    , hChart1SchematicBtn(nullptr)
    , hChart2SchematicBtn(nullptr)
    , hChart1HScrollBar(nullptr)
    , hChart2HScrollBar(nullptr)
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
        10, 30, 100, 20, hWnd, (HMENU)IDC_DUAL_MODE_CHECK, hInst, NULL);
    
    // Создание кнопки Device Schematic для первого графика
    hChart1SchematicBtn = CreateWindow(L"BUTTON", L"Device Schematic",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        120, 28, 140, 25, hWnd, (HMENU)IDC_CHART1_SCHEMATIC_BTN, hInst, NULL);
    
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

    // Создание элементов управления X Range в панели инструментов
    // Размещаем их справа от кнопки Device Schematic
    int xRangeStartX = 280; // Начальная позиция X Range элементов
    int yPos = 30;          // Вертикальная позиция (та же, что у других элементов)
    
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
    } else if (chartNumber == 2) {
        ShowWindow(hChart2HScrollBar, showFlag);
    }
}

void UIModule::UpdateScrollBarPositions(int chartNumber, const RECT& chartRect) {
    const int SCROLLBAR_SIZE = 15; // Уменьшенный размер скроллбара
    const int MARGIN_LEFT = 50;    // Отступ слева (для Y-оси)
    const int MARGIN_RIGHT = 20;   // Отступ справа
    const int MARGIN_BOTTOM = 5;   // Отступ снизу от края графика
    
    if (chartNumber == 1) {
        // Горизонтальный скроллбар располагается ВНУТРИ области графика, в нижней части
        SetWindowPos(hChart1HScrollBar, HWND_TOP, 
                     chartRect.left + MARGIN_LEFT, 
                     chartRect.bottom - SCROLLBAR_SIZE - MARGIN_BOTTOM,
                     chartRect.right - chartRect.left - MARGIN_LEFT - MARGIN_RIGHT, 
                     SCROLLBAR_SIZE,
                     SWP_NOZORDER);
    } else if (chartNumber == 2) {
        // Горизонтальный скроллбар для второго графика
        SetWindowPos(hChart2HScrollBar, HWND_TOP,
                     chartRect.left + MARGIN_LEFT, 
                     chartRect.bottom - SCROLLBAR_SIZE - MARGIN_BOTTOM,
                     chartRect.right - chartRect.left - MARGIN_LEFT - MARGIN_RIGHT, 
                     SCROLLBAR_SIZE,
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
    
    // Обновляем поля ввода текущими значениями
    wchar_t fromText[32], toText[32];
    swprintf_s(fromText, L"%.1f", minX);
    swprintf_s(toText, L"%.1f", maxX);
    
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
    swprintf_s(fromText, L"%.1f", fromX);
    swprintf_s(toText, L"%.1f", toX);
    
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

