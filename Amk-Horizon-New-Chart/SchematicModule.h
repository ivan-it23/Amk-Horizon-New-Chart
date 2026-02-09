#pragma once

#include "framework.h"
#include "DataModule.h"
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <functional>  // Для std::function (v2.9.4)

// Forward declarations
class CurveSelectionModule;

// Типы элементов схемы прибора
enum class ElementType {
    TRANSMITTER,    // Активный передатчик T1-T5
    RECEIVER,       // Приемник R1-R2
    SILENT_TX       // Молчащий передатчик
};

// Типы данных для оси X
enum class XAxisType {
    FRAME_NUMBER,   // Номер кадра (по умолчанию)
    TEMPERATURE     // Температура
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
    int elementIndex;           // Индекс элемента (0-4 для T1-T5, 0-1 для R1-R2)
    DataType dataType;          // Тип данных
    FrequencyBand frequency;    // Частотный диапазон
    
    CurveDefinition();
    CurveDefinition(const std::string& id, const std::string& name, 
                   const std::string& units, ElementType element, 
                   int index, DataType type, FrequencyBand freq);
};

// Фильтр типов данных для пресетов
enum class DataTypeFilter {
    ALL,                    // Все типы данных (по умолчанию)
    AMPLITUDE_ONLY,         // Только амплитуды
    PHASE_ONLY,             // Только фазы
    RATIO_ONLY,             // Только отношения амплитуд
    DIFFERENCE_ONLY,        // Только разности фаз
    TEMPERATURE_ONLY,       // Только температура (v2.9.7)
    RHO_SMT_ONLY,           // Только УЭС (v2.16.0)
    PHASE_SMT_ONLY          // Только симметризованные фазы (v2.16.0)
};

// Выбор элементов на схеме прибора
struct SchematicSelection {
    std::vector<bool> transmitters;  // T1-T5 [5 элементов]
    std::vector<bool> receivers;     // R1-R2 [2 элемента]  
    std::vector<bool> silentTx;      // Молчащие передатчики [2 элемента] для R1 и R2
    FrequencyBand frequency;         // Выбранная частота
    DataTypeFilter dataFilter;       // Фильтр типов данных для пресетов
    
    SchematicSelection();
    void Clear();
    bool IsValid() const;
    int GetSelectedReceiversCount() const;
    int GetSelectedTransmittersCount() const;
    int GetSelectedSilentCount() const;
    std::string GetValidationError() const;
};

// Типы быстрых пресетов (объявление перенесено выше для использования в ChartConfiguration)
enum class PresetType {
    NONE,                   // Нет пресета (пользовательская конфигурация)
    ALL_AMPLITUDES_R1,      // Все амплитуды для R1
    ALL_AMPLITUDES_R2,      // Все амплитуды для R2
    AMPLITUDE_RATIOS,       // Отношения амплитуд R2/R1
    PHASE_DIFFERENCES,      // Разности фаз R2-R1
    TEMPERATURE,            // Температура
    RHO_SMT,                // УЭС (v2.16.0)
    PHASE_SMT,              // Симметризованные фазы (v2.16.0)
    CUSTOM                  // Пользовательская конфигурация (устаревшее, для совместимости)
};

// Конфигурация графика
struct ChartConfiguration {
    std::vector<CurveDefinition> curves;  // Список кривых для отображения
    bool hasDualAxis;                     // Использует ли график две оси
    std::string leftAxisUnits;            // Единицы измерения левой оси
    std::string rightAxisUnits;           // Единицы измерения правой оси
    XAxisType xAxisType;                  // Тип данных для оси X
    PresetType activePreset;              // Активный пресет (NONE если пользовательская конфигурация)
    std::wstring presetDisplayName;       // Русское название пресета для отображения в легенде
    
    ChartConfiguration();
    void Clear();
    bool IsEmpty() const;
    ChartConfiguration Copy() const;
    void AddCurve(const CurveDefinition& curve);
    void RemoveCurve(const std::string& curveId);
    void UpdateAxisConfiguration();
    
    // Проверка, используется ли пресет
    bool IsPresetActive() const { return activePreset != PresetType::NONE && activePreset != PresetType::CUSTOM; }
};

// Типы быстрых пресетов (перенесено выше в ChartConfiguration)
// enum class PresetType определён выше

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
    
    // Информация о текущем пресете (v2.15.0)
    PresetType currentPreset;                    // Текущий активный пресет
    std::wstring currentPresetDisplayName;       // Русское название пресета
    
    // Немодальное окно (v2.9.4)
    HWND hSchematicDialog;                       // Handle немодального окна
    HWND hParentWindow;                          // Handle родительского окна
    
    // Callback для уведомления об изменениях (v2.9.4)
    std::function<void(int)> onConfigurationApplied;  // Вызывается после Apply с номером графика
    
    // Цвета элементов схемы
    static const COLORREF TRANSMITTER_COLOR;     // Красный цвет для передатчиков
    static const COLORREF RECEIVER_COLOR;        // Зеленый цвет для приемников
    static const COLORREF SILENT_TX_COLOR;       // Серый цвет для молчащих
    static const COLORREF SELECTION_COLOR;       // Желтый цвет для выделения
    
public:
    int targetChartNumber;                       // Номер графика (1 или 2)
    
    SchematicModule();
    ~SchematicModule();
    
    // Основные методы интерфейса (v2.9.4: немодальное окно)
    void ShowSchematicDialog(HWND parent, int chartNumber);  // Показать/активировать окно
    void HideSchematicDialog();                              // Скрыть окно
    void CloseSchematicDialog();                             // Закрыть окно
    bool IsSchematicDialogVisible() const;                   // Проверка видимости
    HWND GetSchematicDialogHandle() const;                   // Получить handle окна
    
    // Установка callback для уведомления об изменениях (v2.9.4)
    void SetOnConfigurationAppliedCallback(std::function<void(int)> callback);
    
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
    void HandleXAxisTypeChange(HWND hDlg);  // v2.10.0: Обработка изменения типа оси X
    void HandlePresetButton(HWND hDlg, int presetId);
    void HandleMouseHover(HWND hDlg, int x, int y);
    void HandleCurveCheckboxChange(HWND hDlg, int itemIndex);
    void ValidateCurrentCurveConfiguration(HWND hDlg);
    bool ApplyCurrentCurveConfiguration();
    
    // Определение фильтра по существующим кривым
    DataTypeFilter DetermineDataFilterFromCurves(const std::vector<CurveDefinition>& curves) const;
    
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