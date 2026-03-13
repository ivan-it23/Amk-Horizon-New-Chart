#pragma once

#include "framework.h"
#include "DataModule.h"
#include "Resource.h"

// Класс для управления элементами интерфейса
class UIModule {
public:
    static constexpr int UI_TRANSMITTER_COUNT = kLegacyUiTransmitterCount;

private:
    // Элементы управления
    HWND hDualModeCheck;
    HWND hChart1SchematicBtn;  // Кнопка схемы прибора для первого графика
    HWND hChart2SchematicBtn;  // Кнопка схемы прибора для второго графика

    // Скроллбары для графиков (показываются только при зуме)
    HWND hChart1HScrollBar;  // Горизонтальный скроллбар для первого графика
    HWND hChart2HScrollBar;  // Горизонтальный скроллбар для второго графика
    HWND hChart1VScrollBar;  // Вертикальный скроллбар для первого графика
    HWND hChart2VScrollBar;  // Вертикальный скроллбар для второго графика
    HWND hChart1HUndoBtn;    // Кнопка '-' для undo по оси X (Chart 1)
    HWND hChart1VUndoBtn;    // Кнопка '-' для undo по оси Y (Chart 1)
    HWND hChart2HUndoBtn;    // Кнопка '-' для undo по оси X (Chart 2)
    HWND hChart2VUndoBtn;    // Кнопка '-' для undo по оси Y (Chart 2)

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
    HWND hCutRange1Btn;

    // Кнопки сброса zoom
    HWND hResetZoom1Btn;     // Кнопка "Reset Zoom" для Chart 1
    HWND hResetZoom2Btn;     // Кнопка "Reset Zoom" для Chart 2

    // Элементы отображения координат курсора (Chart 1) - компактный дизайн v2.15.1
    // Формат: [X] : [Y1] | [Y2]
    HWND hCursor1Icon;       // Иконка 
    HWND hCursor1XEdit;      // Поле координаты X (только чтение)
    HWND hCursor1Separator;  // Разделитель ":"
    HWND hCursor1Y1Edit;     // Поле координаты Y1 с единицами (только чтение)
    HWND hCursor1Y2Separator;// Разделитель "|" для второй оси
    HWND hCursor1Y2Edit;     // Поле координаты Y2 с единицами (только чтение, для двойных осей)

    // Элементы отображения координат курсора (Chart 2) - компактный дизайн v2.15.1
    HWND hCursor2Icon;       // Иконка 
    HWND hCursor2XEdit;      // Поле координаты X (только чтение)
    HWND hCursor2Separator;  // Разделитель ":"
    HWND hCursor2Y1Edit;     // Поле координаты Y1 с единицами (только чтение)
    HWND hCursor2Y2Separator;// Разделитель "|" для второй оси
    HWND hCursor2Y2Edit;     // Поле координаты Y2 с единицами (только чтение, для двойных осей)

    // Статусная строка
    HWND hStatusBar;         // Статусная строка для отображения координат курсора

    // === Элементы управления COM-портом (v2.13.0) ===
    HWND hSerialPortCombo;   // Выпадающий список COM-портов
    HWND hSerialConnectBtn;  // Кнопка Connect/Disconnect
    HWND hSerialStatusLabel; // Метка статуса подключения
    HWND hSerialStartBtn;    // Кнопка Start (начать опрос)
    HWND hSerialStopBtn;     // Кнопка Stop (остановить опрос)
    HWND hSerialClearBtn;    // Кнопка Clear (очистить данные)
    HWND hSerialFrameLabel;  // Метка номера кадра
    HWND hSerialPacketsLabel;// Метка количества пакетов

    // === Элементы управления режимом Surf (v2.14.0) ===
    HWND hChart1SurfCheck;   // Checkbox "Surf" для Chart 1
    HWND hChart1SurfEdit;    // Поле ввода количества точек для Chart 1
    HWND hChart2SurfCheck;   // Checkbox "Surf" для Chart 2
    HWND hChart2SurfEdit;    // Поле ввода количества точек для Chart 2

    // === Элементы управления отображением заметок на графике ===
    HWND hChart1UndoZoomBtn;
    HWND hChart2UndoZoomBtn;
    HWND hChart1ShowNotesCheck; // Checkbox "Отображение заметок" для Chart 1
    HWND hChart2ShowNotesCheck; // Checkbox "Отображение заметок" для Chart 2

    // Состояние элементов управления
    bool dualMode;

    // Состояние включенных/отключенных передатчиков (для совместимости с ChartModule)
    std::vector<bool> chart1EnabledTransmitters; // Размер 5: T1, T2, T3, T4, T5
    std::vector<bool> chart2EnabledTransmitters; // Размер 5: T1, T2, T3, T4, T5

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
    void SetVScrollBarRange(int chartNumber, int min, int max, int page, int pos);
    HWND GetHScrollBar(int chartNumber) const;
    HWND GetVScrollBar(int chartNumber) const;
    void ShowAxisUndoButtons(int chartNumber, bool showX, bool showY);

    // Управление X Range (chartNumber: 1 или 2)
    void UpdateXRangeControls(int chartNumber, double minX, double maxX, bool isCustomRange);
    void GetXRangeValues(int chartNumber, double& fromX, double& toX) const;
    void SetXRangeValues(int chartNumber, double fromX, double toX);
    void SetXRangeResetEnabled(int chartNumber, bool enabled);
    void ResetXRangeControls(int chartNumber);
    bool IsXRangeFromEmpty(int chartNumber) const; // Проверка, пустое ли поле "From"
    bool IsXRangeToEmpty(int chartNumber) const;  // Проверка, пустое ли поле "To"

    // Управление статусной строкой
    void UpdateStatusBar(const std::wstring& text);
    void UpdateStatusBarCoordinates(double x, double y1, const std::wstring& y1Unit, 
                                    double y2 = 0.0, const std::wstring& y2Unit = L"", 
                                    bool hasDualAxis = false);
    void ClearStatusBarCoordinates();
    void ResizeStatusBar(); // Обновление размера статусной строки при изменении размера окна
    HWND GetStatusBar() const { return hStatusBar; }

    // Управление отображением координат курсора при клике (v2.15.1 - поддержка двойных осей)
    void UpdateClickCoordinates(int chartNumber, double x, double y1, const std::wstring& y1Unit,
                                double y2 = 0.0, const std::wstring& y2Unit = L"", bool hasDualAxis = false);
    void ClearClickCoordinates(int chartNumber);
    void ShowDualAxisControls(int chartNumber, bool show);  // Показать/скрыть элементы для второй оси
    void PositionCursorControls(int windowWidth, int panel2Y = 0);  // Позиционирование координат к правому краю

    // Управление отображением заметок
    void PositionNotesVisibilityControls(const RECT& chart1Rect, const RECT& chart2Rect, bool dualMode);
    bool IsNotesVisible(int chartNumber) const;
    void SetNotesVisible(int chartNumber, bool visible);
    void SetUndoZoomStepEnabled(int chartNumber, bool enabled);

    // === Управление элементами COM-порта (v2.13.0) ===
    void CreateSerialControls(HWND hWnd, HINSTANCE hInst);
    void UpdateSerialPortList(const std::vector<std::wstring>& ports);
    std::wstring GetSelectedPort() const;
    void SetSerialConnected(bool connected);
    void SetSerialPolling(bool polling);
    void UpdateSerialStatus(const std::wstring& status);
    void UpdateSerialFrameInfo(uint32_t frameNumber, uint32_t packetsReceived, uint32_t packetsErrors);
    HWND GetSerialPortCombo() const { return hSerialPortCombo; }
    HWND GetSerialConnectBtn() const { return hSerialConnectBtn; }
    HWND GetSerialStartBtn() const { return hSerialStartBtn; }
    HWND GetSerialStopBtn() const { return hSerialStopBtn; }
    HWND GetSerialClearBtn() const { return hSerialClearBtn; }

    // === Управление режимом Surf (v2.14.0) ===
    void CreateSurfControls(HWND hWnd, HINSTANCE hInst);
    void UpdateSurfControlsVisibility(bool showChart2Controls);
    void PositionSurfControls(int chart2PanelY);
    
    // Получение/установка состояния Surf
    bool IsSurfEnabled(int chartNumber) const;
    void SetSurfEnabled(int chartNumber, bool enabled);
    int GetSurfWindowSize(int chartNumber) const;
    void SetSurfWindowSize(int chartNumber, int windowSize);
    
    // Обновление статусной строки с информацией о Surf
    void UpdateStatusBarSurfInfo(bool surf1Enabled, int surf1Window, bool surf2Enabled, int surf2Window);

    // Обработка Owner-Draw кнопок
    void DrawIconButton(LPDRAWITEMSTRUCT lpDrawItem);
    bool IsIconButton(UINT ctrlId) const;

private:
    // Вспомогательные методы
    void CreateTooltip(HWND hWnd, HWND hControl, const wchar_t* text);
    HWND hTooltip;  // Общий tooltip для всех элементов
};