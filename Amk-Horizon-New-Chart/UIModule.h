#pragma once

#include "framework.h"
#include "DataModule.h"
#include "Resource.h"

// Класс для управления элементами интерфейса
class UIModule {
private:
    // Элементы управления
    HWND hDualModeCheck;
    HWND hChart1DataCombo;
    HWND hChart1FreqCombo;
    HWND hChart1RefreshBtn;
    HWND hChart2DataCombo;
    HWND hChart2FreqCombo;
    HWND hChart2RefreshBtn;

    // Состояние элементов управления
    DataType chart1DataType;
    DataType chart2DataType;
    FrequencyBand chart1Frequency;
    FrequencyBand chart2Frequency;
    bool dualMode;

public:
    UIModule();
    ~UIModule();

    // Создание и управление элементами управления
    void CreateControls(HWND hWnd, HINSTANCE hInst);
    void UpdateControlsVisibility(bool showChart2Controls);
    void PositionChart2Controls(int panel2Y);

    // Управление состоянием
    void ToggleDualMode();
    bool IsDualMode() const { return dualMode; }
    
    // Получение текущих настроек
    DataType GetChart1DataType() const { return chart1DataType; }
    DataType GetChart2DataType() const { return chart2DataType; }
    FrequencyBand GetChart1Frequency() const { return chart1Frequency; }
    FrequencyBand GetChart2Frequency() const { return chart2Frequency; }

    // Обновление данных из элементов управления
    void RefreshChart1Settings();
    void RefreshChart2Settings();

    // Обработка команд
    bool HandleCommand(WPARAM wParam, LPARAM lParam, HWND hWnd);

private:
    // Вспомогательные методы
    void PopulateComboBoxes();
    void SetInitialValues();
};