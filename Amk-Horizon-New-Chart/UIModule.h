#pragma once

#include "framework.h"
#include "DataModule.h"
#include "Resource.h"

// Класс для управления элементами интерфейса
class UIModule {
private:
    // Элементы управления
    HWND hDualModeCheck;
    HWND hChart1SchematicBtn;  // Кнопка схемы прибора для первого графика
    HWND hChart2SchematicBtn;  // Кнопка схемы прибора для второго графика

    // Скроллбары для графиков (показываются только при зуме)
    HWND hChart1HScrollBar;  // Горизонтальный скроллбар для первого графика
    HWND hChart2HScrollBar;  // Горизонтальный скроллбар для второго графика

    // Элементы управления X Range для Chart 1 (в панели инструментов)
    HWND hXRangeFromLabel;   // Метка "X Range:"
    HWND hXRangeFromEdit;    // Поле ввода начальной точки
    HWND hXRangeToLabel;     // Метка "to"
    HWND hXRangeToEdit;      // Поле ввода конечной точки
    HWND hXRangeApplyBtn;    // Кнопка "Apply"
    HWND hXRangeResetBtn;    // Кнопка "Reset"

    // Элементы управления X Range для Chart 2 (рядом с кнопкой Device Schematic)
    HWND hXRange2FromLabel;  // Метка "X Range:"
    HWND hXRange2FromEdit;   // Поле ввода начальной точки
    HWND hXRange2ToLabel;    // Метка "to"
    HWND hXRange2ToEdit;     // Поле ввода конечной точки
    HWND hXRange2ApplyBtn;   // Кнопка "Apply"
    HWND hXRange2ResetBtn;   // Кнопка "Reset"
    
    // Кнопки сохранения диапазона
    HWND hSaveRange1Btn;     // Кнопка "Save Range" для Chart 1

    // Состояние элементов управления
    bool dualMode;

    // Состояние включенных/отключенных передатчиков (для совместимости с ChartModule)
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
    
    // Методы для работы с состоянием передатчиков (используются ChartModule для легенды)
    const std::vector<bool>& GetChart1EnabledTransmitters() const { return chart1EnabledTransmitters; }
    const std::vector<bool>& GetChart2EnabledTransmitters() const { return chart2EnabledTransmitters; }
    void SetChart1TransmitterEnabled(int transmitterIdx, bool enabled);
    void SetChart2TransmitterEnabled(int transmitterIdx, bool enabled);

    // Обработка команд
    bool HandleCommand(WPARAM wParam, LPARAM lParam, HWND hWnd);

    // Управление скроллбарами
    void ShowScrollBars(int chartNumber, bool show);
    void UpdateScrollBarPositions(int chartNumber, const RECT& chartRect);
    void SetScrollBarRange(int chartNumber, int min, int max, int page, int pos);
    HWND GetHScrollBar(int chartNumber) const;

    // Управление X Range (chartNumber: 1 или 2)
    void UpdateXRangeControls(int chartNumber, double minX, double maxX, bool isCustomRange);
    void GetXRangeValues(int chartNumber, double& fromX, double& toX) const;
    void SetXRangeValues(int chartNumber, double fromX, double toX);
    void ResetXRangeControls(int chartNumber);

private:
    // Вспомогательные методы удалены после рефакторинга
};