#include "framework.h"
#include "UIModule.h"

UIModule::UIModule()
    : hDualModeCheck(nullptr)
    , hChart1DataCombo(nullptr)
    , hChart1FreqCombo(nullptr)
    , hChart1RefreshBtn(nullptr)
    , hChart1SchematicBtn(nullptr)
    , hChart2DataCombo(nullptr)
    , hChart2FreqCombo(nullptr)
    , hChart2RefreshBtn(nullptr)
    , hChart2SchematicBtn(nullptr)
    , hChart1HScrollBar(nullptr)
    , hChart2HScrollBar(nullptr)
    , chart1DataType(DataType::PHASE_DIFFERENCE)
    , chart2DataType(DataType::AMPLITUDE_RATIO)
    , chart1Frequency(FrequencyBand::FREQ_400_KHZ)
    , chart2Frequency(FrequencyBand::FREQ_400_KHZ)
    , dualMode(false) {
    chart1EnabledTransmitters.resize(4, true); // Изначально все T1-T4 включены
    chart2EnabledTransmitters.resize(4, true); // Изначально все T1-T4 включены
}

UIModule::~UIModule() {
}

void UIModule::CreateControls(HWND hWnd, HINSTANCE hInst) {
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
    
    hChart1SchematicBtn = CreateWindow(L"BUTTON", L"Device Schematic",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        510, 28, 120, 25, hWnd, (HMENU)IDC_CHART1_SCHEMATIC_BTN, hInst, NULL);
    
    // Создание элементов управления второго графика (изначально скрыты)
    hChart2DataCombo = CreateWindow(L"COMBOBOX", NULL,
        WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        10, 300, 180, 200, hWnd, (HMENU)IDC_CHART2_DATA_COMBO, hInst, NULL);
    
    hChart2FreqCombo = CreateWindow(L"COMBOBOX", NULL,
        WS_CHILD | CBS_DROPDOWNLIST,
        200, 300, 100, 100, hWnd, (HMENU)IDC_CHART2_FREQ_COMBO, hInst, NULL);
    
    hChart2RefreshBtn = CreateWindow(L"BUTTON", L"Refresh",
        WS_CHILD | BS_PUSHBUTTON,
        310, 300, 80, 25, hWnd, (HMENU)IDC_CHART2_REFRESH_BTN, hInst, NULL);
    
    hChart2SchematicBtn = CreateWindow(L"BUTTON", L"Device Schematic",
        WS_CHILD | BS_PUSHBUTTON,
        400, 300, 120, 25, hWnd, (HMENU)IDC_CHART2_SCHEMATIC_BTN, hInst, NULL);

    // Создание горизонтальных скроллбаров (изначально скрыты)
    hChart1HScrollBar = CreateWindow(L"SCROLLBAR", NULL,
        WS_CHILD | SBS_HORZ,
        0, 0, 100, 20, hWnd, (HMENU)IDC_CHART1_HSCROLL, hInst, NULL);
    
    hChart2HScrollBar = CreateWindow(L"SCROLLBAR", NULL,
        WS_CHILD | SBS_HORZ,
        0, 0, 100, 20, hWnd, (HMENU)IDC_CHART2_HSCROLL, hInst, NULL);

    // Заполнение комбобоксов и установка начальных значений
    PopulateComboBoxes();
    SetInitialValues();
}

void UIModule::UpdateControlsVisibility(bool showChart2Controls) {
    int showFlag = showChart2Controls ? SW_SHOW : SW_HIDE;
    ShowWindow(hChart2DataCombo, showFlag);
    ShowWindow(hChart2FreqCombo, showFlag);
    ShowWindow(hChart2RefreshBtn, showFlag);
    ShowWindow(hChart2SchematicBtn, showFlag);
}

void UIModule::PositionChart2Controls(int panel2Y) {
    SetWindowPos(hChart2DataCombo, HWND_TOP, 10, panel2Y, 180, 25, SWP_NOSIZE);
    SetWindowPos(hChart2FreqCombo, HWND_TOP, 200, panel2Y, 100, 25, SWP_NOSIZE);
    SetWindowPos(hChart2RefreshBtn, HWND_TOP, 310, panel2Y, 80, 25, SWP_NOSIZE);
    SetWindowPos(hChart2SchematicBtn, HWND_TOP, 400, panel2Y, 100, 25, SWP_NOSIZE);
}

void UIModule::ToggleDualMode() {
    dualMode = !dualMode;
    SendMessage(hDualModeCheck, BM_SETCHECK, dualMode ? BST_CHECKED : BST_UNCHECKED, 0);
}

void UIModule::RefreshChart1Settings() {
    int dataTypeIndex = (int)SendMessage(hChart1DataCombo, CB_GETCURSEL, 0, 0);
    int freqIndex = (int)SendMessage(hChart1FreqCombo, CB_GETCURSEL, 0, 0);
    
    if (dataTypeIndex >= 0 && dataTypeIndex < 9)
        chart1DataType = (DataType)dataTypeIndex;
    if (freqIndex >= 0 && freqIndex < 2)
        chart1Frequency = (FrequencyBand)freqIndex;
}

void UIModule::RefreshChart2Settings() {
    int dataIndex = (int)SendMessage(hChart2DataCombo, CB_GETCURSEL, 0, 0);
    chart2DataType = (DataType)dataIndex;

    int freqIndex = (int)SendMessage(hChart2FreqCombo, CB_GETCURSEL, 0, 0);
    chart2Frequency = (FrequencyBand)freqIndex;
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
    case IDC_CHART1_REFRESH_BTN:
        if (wmEvent == BN_CLICKED) {
            RefreshChart1Settings();
            needsRedraw = true;
        }
        break;
    case IDC_CHART2_REFRESH_BTN:
        if (wmEvent == BN_CLICKED) {
            RefreshChart2Settings();
            needsRedraw = true;
        }
        break;
    }
    return needsRedraw;
}

void UIModule::PopulateComboBoxes() {
    const wchar_t* dataTypes[] = {
        L"Phase Difference", L"Amplitude Ratio", L"Amplitude RX1", L"Amplitude RX2",
        L"Phase RX1", L"Phase RX2", L"Silent Amplitude RX1", L"Silent Amplitude RX2", L"Temperature"
    };
    
    // Заполнение комбобоксов типов данных
    for (int i = 0; i < 9; i++) {
        SendMessage(hChart1DataCombo, CB_ADDSTRING, 0, (LPARAM)dataTypes[i]);
        SendMessage(hChart2DataCombo, CB_ADDSTRING, 0, (LPARAM)dataTypes[i]);
    }
    
    // Заполнение комбобоксов частот
    SendMessage(hChart1FreqCombo, CB_ADDSTRING, 0, (LPARAM)L"400 kHz");
    SendMessage(hChart1FreqCombo, CB_ADDSTRING, 0, (LPARAM)L"2000 kHz");
    SendMessage(hChart2FreqCombo, CB_ADDSTRING, 0, (LPARAM)L"400 kHz");
    SendMessage(hChart2FreqCombo, CB_ADDSTRING, 0, (LPARAM)L"2000 kHz");
}

void UIModule::SetInitialValues() {
    SendMessage(hChart1DataCombo, CB_SETCURSEL, 0, 0);  // Phase Difference
    SendMessage(hChart2DataCombo, CB_SETCURSEL, 1, 0);  // Amplitude Ratio
    SendMessage(hChart1FreqCombo, CB_SETCURSEL, 0, 0);  // 400 кГц
    SendMessage(hChart2FreqCombo, CB_SETCURSEL, 0, 0);  // 400 кГц
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

// Методы для установки настроек (для синхронизации с новой системой кривых)
void UIModule::SetChart1DataType(DataType dataType) {
    chart1DataType = dataType;
    SendMessage(hChart1DataCombo, CB_SETCURSEL, static_cast<int>(dataType), 0);
}

void UIModule::SetChart1Frequency(FrequencyBand frequency) {
    chart1Frequency = frequency;
    SendMessage(hChart1FreqCombo, CB_SETCURSEL, static_cast<int>(frequency), 0);
}

void UIModule::SetChart2DataType(DataType dataType) {
    chart2DataType = dataType;
    SendMessage(hChart2DataCombo, CB_SETCURSEL, static_cast<int>(dataType), 0);
}

void UIModule::SetChart2Frequency(FrequencyBand frequency) {
    chart2Frequency = frequency;
    SendMessage(hChart2FreqCombo, CB_SETCURSEL, static_cast<int>(frequency), 0);
}