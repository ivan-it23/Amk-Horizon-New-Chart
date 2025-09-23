#include "framework.h"
#include "UIModule.h"

UIModule::UIModule()
    : hDualModeCheck(nullptr)
    , hChart1DataCombo(nullptr)
    , hChart1FreqCombo(nullptr)
    , hChart1RefreshBtn(nullptr)
    , hChart2DataCombo(nullptr)
    , hChart2FreqCombo(nullptr)
    , hChart2RefreshBtn(nullptr)
    , chart1DataType(DataType::PHASE_DIFFERENCE)
    , chart2DataType(DataType::AMPLITUDE_RATIO)
    , chart1Frequency(FrequencyBand::FREQ_400_KHZ)
    , chart2Frequency(FrequencyBand::FREQ_400_KHZ)
    , dualMode(false) {
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

    // Заполнение комбобоксов и установка начальных значений
    PopulateComboBoxes();
    SetInitialValues();
}

void UIModule::UpdateControlsVisibility(bool showChart2Controls) {
    int showFlag = showChart2Controls ? SW_SHOW : SW_HIDE;
    ShowWindow(hChart2DataCombo, showFlag);
    ShowWindow(hChart2FreqCombo, showFlag);
    ShowWindow(hChart2RefreshBtn, showFlag);
}

void UIModule::PositionChart2Controls(int panel2Y) {
    SetWindowPos(hChart2DataCombo, HWND_TOP, 10, panel2Y, 180, 25, SWP_NOSIZE);
    SetWindowPos(hChart2FreqCombo, HWND_TOP, 200, panel2Y, 100, 25, SWP_NOSIZE);
    SetWindowPos(hChart2RefreshBtn, HWND_TOP, 310, panel2Y, 80, 25, SWP_NOSIZE);
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
    int dataTypeIndex = (int)SendMessage(hChart2DataCombo, CB_GETCURSEL, 0, 0);
    int freqIndex = (int)SendMessage(hChart2FreqCombo, CB_GETCURSEL, 0, 0);
    
    if (dataTypeIndex >= 0 && dataTypeIndex < 9)
        chart2DataType = (DataType)dataTypeIndex;
    if (freqIndex >= 0 && freqIndex < 2)
        chart2Frequency = (FrequencyBand)freqIndex;
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