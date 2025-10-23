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

private:
    // Вспомогательные методы удалены после рефакторинга
};