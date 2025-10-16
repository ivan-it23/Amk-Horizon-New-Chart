#pragma once

#include "framework.h"
#include "DataModule.h"
#include <vector>
#include <string>
#include <memory>
#include <algorithm>

// Forward declarations
class CurveSelectionModule;

// Типы элементов схемы прибора
enum class ElementType {
    TRANSMITTER,    // Активный передатчик T1-T4
    RECEIVER,       // Приемник R1-R2
    SILENT_TX       // Молчащий передатчик
};

// Определение кривой для отображения
struct CurveDefinition {
    std::string id;              // Уникальный идентификатор "T1_R1_AMP_400"
    std::string displayName;     // Отображаемое имя "T1 Амплитуда R1"
    std::string units;           // Единицы измерения "В" или "°"
    int yAxisGroup;             // 0=левая ось, 1=правая ось
    bool enabled;               // Включена ли кривая
    COLORREF color;             // Цвет кривой
    int lineStyle;              // Стиль линии PS_SOLID или PS_DASH
    
    // Параметры для вычисления данных
    ElementType sourceElement;   // Тип источника данных
    int elementIndex;           // Индекс элемента (0-3 для T1-T4, 0-1 для R1-R2)
    DataType dataType;          // Тип данных
    FrequencyBand frequency;    // Частотный диапазон
    
    CurveDefinition();
    CurveDefinition(const std::string& id, const std::string& name, 
                   const std::string& units, ElementType element, 
                   int index, DataType type, FrequencyBand freq);
};

// Выбор элементов на схеме прибора
struct SchematicSelection {
    std::vector<bool> transmitters;  // T1-T4 [4 элемента]
    std::vector<bool> receivers;     // R1-R2 [2 элемента]  
    std::vector<bool> silentTx;      // Молчащие передатчики [2 элемента] для R1 и R2
    FrequencyBand frequency;         // Выбранная частота
    
    SchematicSelection();
    void Clear();
    bool IsValid() const;
    int GetSelectedReceiversCount() const;
    int GetSelectedTransmittersCount() const;
    int GetSelectedSilentCount() const;
    std::string GetValidationError() const;
};

// Конфигурация графика
struct ChartConfiguration {
    std::vector<CurveDefinition> curves;  // Список кривых для отображения
    bool hasDualAxis;                     // Использует ли график две оси
    std::string leftAxisUnits;            // Единицы измерения левой оси
    std::string rightAxisUnits;           // Единицы измерения правой оси
    
    ChartConfiguration();
    void Clear();
    bool IsEmpty() const;
    ChartConfiguration Copy() const;
    void AddCurve(const CurveDefinition& curve);
    void RemoveCurve(const std::string& curveId);
    void UpdateAxisConfiguration();
};

// Типы быстрых пресетов
enum class PresetType {
    ALL_AMPLITUDES_R1,      // Все амплитуды для R1
    ALL_AMPLITUDES_R2,      // Все амплитуды для R2
    ALL_PHASES_R1,          // Все фазы для R1
    ALL_PHASES_R2,          // Все фазы для R2
    AMPLITUDE_RATIOS,       // Отношения амплитуд R2/R1
    PHASE_DIFFERENCES,      // Разности фаз R2-R1
    CUSTOM                  // Пользовательская конфигурация
};

// Класс для управления интерактивной схемой прибора
class SchematicModule {
private:
    // Размеры и позиции элементов схемы
    struct ElementRect {
        RECT rect;
        ElementType type;
        int index;
        bool selected;
        bool hovered;  // Новое поле для hover состояния
        
        ElementRect(const RECT& r, ElementType t, int idx) 
            : rect(r), type(t), index(idx), selected(false), hovered(false) {}
    };
    
    std::vector<ElementRect> schematicElements;  // Элементы схемы для хит-теста
    SchematicSelection currentSelection;         // Текущий выбор элементов
    ElementRect* hoveredElement;                 // Указатель на элемент под курсором
    RECT deviceFrameRect;                        // Координаты рамки для основных элементов
    
    // Указатель на модуль выбора кривых (устанавливается извне)
    CurveSelectionModule* curveSelectionModule;
    
    // Текущие доступные кривые (для работы с ListView)
    std::vector<CurveDefinition> currentAvailableCurves;
    
    // Цвета элементов схемы
    static const COLORREF TRANSMITTER_COLOR;     // Красный цвет для передатчиков
    static const COLORREF RECEIVER_COLOR;        // Зеленый цвет для приемников
    static const COLORREF SILENT_TX_COLOR;       // Серый цвет для молчащих
    static const COLORREF SELECTION_COLOR;       // Желтый цвет для выделения
    
public:
    int targetChartNumber;                       // Номер графика (1 или 2)
    
    SchematicModule();
    ~SchematicModule();
    
    // Основные методы интерфейса
    bool ShowSchematicDialog(HWND parent, int chartNumber, ChartConfiguration& config);
    
    // Методы отрисовки схемы
    void RenderSchematic(HDC hdc, const RECT& rect);
    void DrawElement(HDC hdc, const RECT& elementRect, ElementType type, 
                    int index, bool selected, bool hovered, const std::wstring& label);
    
    // Обработка взаимодействия
    bool HandleElementClick(int x, int y);
    void UpdateSelection(ElementType type, int index, bool selected);
    void UpdateHoverState(int x, int y);  // Новый метод для обработки hover
    bool IsSelectionCompatible(ElementType type, int index) const;
    void ClearIncompatibleSelections(ElementType newType);
    void SynchronizeElementStates();
    
    // Валидация и получение данных
    bool ValidateCurrentSelection();
    SchematicSelection GetCurrentSelection() const;
    void SetCurrentSelection(const SchematicSelection& selection);
    void SetCurveSelectionModule(CurveSelectionModule* module);
    
    // Вспомогательные методы
    void InitializeSchematicElements(const RECT& schemaRect);
    ElementRect* FindElementAt(int x, int y);
    std::wstring GetElementLabel(ElementType type, int index) const;
    std::wstring GetElementTooltip(ElementType type, int index) const;
    
    // Методы для работы с диалоговым окном
    void InitializeDialogControls(HWND hDlg);
    void UpdateAvailableCurves(HWND hDlg);
    void HandleFrequencyChange(HWND hDlg);
    void HandlePresetButton(HWND hDlg, int presetId);
    void HandleCopyFromChart(HWND hDlg);
    void HandleMouseHover(HWND hDlg, int x, int y);
    void HandleCurveCheckboxChange(HWND hDlg, int itemIndex);
    void ValidateCurrentCurveConfiguration(HWND hDlg);
    bool ApplyCurrentCurveConfiguration();
    
    // Дополнительные методы отрисовки схемы
    void DrawDeviceFrame(HDC hdc, const RECT& frameRect);
    void DrawSchematicConnections(HDC hdc, const RECT& rect);
    void DrawGroupLabels(HDC hdc, const RECT& rect);
    void DrawConnectionArrows(HDC hdc, const std::vector<RECT>& transmitterRects, 
                             const RECT& r1Rect, const RECT& r2Rect);
    
private:
    // Внутренние методы отрисовки
    void DrawTransmitter(HDC hdc, const RECT& rect, int index, bool selected);
    void DrawReceiver(HDC hdc, const RECT& rect, int index, bool selected);
    void DrawSilentTransmitter(HDC hdc, const RECT& rect, int index, bool selected);
    void DrawSelectionHighlight(HDC hdc, const RECT& rect);
    void DrawGradientRect(HDC hdc, const RECT& rect, COLORREF colorStart, COLORREF colorEnd);
    void DrawElementShadow(HDC hdc, const RECT& rect);
    
    // Расчет позиций элементов
    RECT CalculateElementRect(const RECT& schemaRect, ElementType type, int index);
    void CalculateSchematicLayout(const RECT& schemaRect);
};

// Процедура диалогового окна схемы прибора
INT_PTR CALLBACK SchematicDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);