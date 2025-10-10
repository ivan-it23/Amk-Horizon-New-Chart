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
    HWND hChart1SchematicBtn;  // Кнопка схемы прибора для первого графика
    HWND hChart2DataCombo;
    HWND hChart2FreqCombo;
    HWND hChart2RefreshBtn;
    HWND hChart2SchematicBtn;  // Кнопка схемы прибора для второго графика

    // Скроллбары для графиков (показываются только при зуме)
    HWND hChart1HScrollBar;  // Горизонтальный скроллбар для первого графика
    HWND hChart2HScrollBar;  // Горизонтальный скроллбар для второго графика

    // Состояние элементов управления
    DataType chart1DataType;
    DataType chart2DataType;
    FrequencyBand chart1Frequency;
    FrequencyBand chart2Frequency;
    bool dualMode;

    // Состояние включенных/отключенных передатчиков для каждого графика (T1, T2, T3, T4)
    std::vector<bool> chart1EnabledTransmitters; // Размер 4: T1, T2, T3, T4
    std::vector<bool> chart2EnabledTransmitters; // Размер 4: T1, T2, T3, T4

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
    const std::vector<bool>& GetChart1EnabledTransmitters() const { return chart1EnabledTransmitters; }
    const std::vector<bool>& GetChart2EnabledTransmitters() const { return chart2EnabledTransmitters; }
    
    // Установка настроек (для синхронизации с новой системой кривых)
    void SetChart1DataType(DataType dataType);
    void SetChart1Frequency(FrequencyBand frequency);
    void SetChart2DataType(DataType dataType);
    void SetChart2Frequency(FrequencyBand frequency);

    // Обновление состояния передатчиков
    void SetChart1TransmitterEnabled(int transmitterIdx, bool enabled);
    void SetChart2TransmitterEnabled(int transmitterIdx, bool enabled);

    // Обновление данных из элементов управления
    void RefreshChart1Settings();
    void RefreshChart2Settings();

    // Обработка команд
    bool HandleCommand(WPARAM wParam, LPARAM lParam, HWND hWnd);

    // Управление скроллбарами
    void ShowScrollBars(int chartNumber, bool show);
    void UpdateScrollBarPositions(int chartNumber, const RECT& chartRect);
    void SetScrollBarRange(int chartNumber, int min, int max, int page, int pos);
    HWND GetHScrollBar(int chartNumber) const;

private:
    // Вспомогательные методы
    void PopulateComboBoxes();
    void SetInitialValues();
};