# 🔧 Техническая документация AMK-Horizon-New-Chart v2.2.4

## 📋 Обзор

Данный документ содержит техническую информацию о структурах данных, API модулей, алгоритмах и внутренней логике приложения AMK-Horizon-New-Chart версии 2.2.4.

## 🆕 Новые функции v2.2.2-2.2.4

### Преобразование единиц измерения (v2.2.2-2.2.3)

#### Амплитуды в милливольты (v2.2.2)
- **Формула преобразования**: `value * 4000.0f / pow(2.0f, 24.0f)`
- **Единицы**: "V" → "mV" (милливольты)
- **Применено к**: AMPLITUDE_RX1, AMPLITUDE_RX2, SILENT_AMPLITUDE_RX1, SILENT_AMPLITUDE_RX2
- **Не изменено**: AMPLITUDE_RATIO (безразмерная величина V/V)
- **Модули**: DataModule.cpp, CurveSelectionModule.cpp

#### Фазы в миллиградусы (v2.2.3)
- **Формула преобразования**: `value * 57295.7795f` (180/π * 1000)
- **Единицы**: "deg" → "mdeg" (миллиградусы)
- **Применено к**: PHASE_RX1, PHASE_RX2, PHASE_DIFFERENCE
- **Диапазон**: -180000 мдег до +180000 мдег
- **Модули**: DataModule.cpp, CurveSelectionModule.cpp

#### Увеличенные отступы для осей (v2.2.3)
- **Левая ось**: 50px → 80px (для длинных значений мВ и мдег)
- **Правая ось (обычный график)**: 20px → 30px
- **Правая ось (двойные оси)**: 80px → 120px
- **Модуль**: ChartModule.cpp

### Исправление выравнивания подписей (v2.2.4)
- **Проблема**: Длинные значения (например, -180000 мдег) обрезались
- **Решение**: Использование `GetTextExtentPoint32()` для вычисления ширины текста
- **Формула позиционирования**: `textX = plotRect.left - 5 - textWidth`
- **Результат**: Подписи используют всё доступное пространство (80px)
- **Модули**: ChartModule.cpp (методы DrawGridAndAxes, DrawLeftAxis)

### Упрощение Zoom Selection (v2.2.1)
- **Удалена диагональная штриховка**: ~40 строк кода удалено
- **Только оранжевая рамка**: RGB(255, 140, 0) с пунктирным стилем
- **Прозрачная заливка**: NULL_BRUSH - график полностью виден
- **Режим отрисовки**: R2_NOTXORPEN для XOR эффекта
- **Модуль**: ChartModule.cpp (метод DrawZoomRectangle)

### Независимые X Range Controls (v2.2.0)
- **12 новых элементов управления**: 6 для Chart 1 + 6 для Chart 2
- **Независимые ID**: IDC_XRANGE_* для Chart 1, IDC_XRANGE2_* для Chart 2  
- **Автоматическое управление видимостью**: Элементы Chart 2 показываются только в Dual Mode
- **Динамическое позиционирование**: Элементы Chart 2 позиционируются относительно layout

## 📊 Структуры данных

### GP_TOOL_DATA
Основная структура данных прибора:

```cpp
struct GP_TOOL_DATA {
    float AM_RX_1[10];    // Амплитуды приемника 1 (10 каналов)
    float AM_RX_2[10];    // Амплитуды приемника 2 (10 каналов)
    float PH_RX_1[10];    // Фазы приемника 1 (10 каналов)
    float PH_RX_2[10];    // Фазы приемника 2 (10 каналов)
    float temperature;    // Температура прибора
};
```

### Индексация каналов
```cpp
// Для частоты 400 kHz:
// Каналы 0-1: Молчащие передатчики
// Каналы 2-5: Активные передатчики T1-T4
// Каналы 6-9: Резерв

// Для частоты 2000 kHz:
// Каналы 0-1: Молчащие передатчики  
// Каналы 2-5: Резерв
// Каналы 6-9: Активные передатчики T1-T4
```

### DataType (Типы данных)
```cpp
enum class DataType {
    PHASE_DIFFERENCE = 0,      // Разности фаз R2-R1
    AMPLITUDE_RATIO = 1,       // Отношения амплитуд R2/R1
    AMPLITUDE_RX1 = 2,         // Амплитуды приемника 1
    AMPLITUDE_RX2 = 3,         // Амплитуды приемника 2
    PHASE_RX1 = 4,            // Фазы приемника 1
    PHASE_RX2 = 5,            // Фазы приемника 2
    SILENT_AMPLITUDE_RX1 = 6,  // Амплитуды молчащих для R1
    SILENT_AMPLITUDE_RX2 = 7,  // Амплитуды молчащих для R2
    TEMPERATURE = 8            // Температура
};
```

### FrequencyBand (Частотные диапазоны)
```cpp
enum class FrequencyBand {
    FREQ_400_KHZ = 0,   // 400 кГц
    FREQ_2000_KHZ = 1   // 2000 кГц
};
```

### CurveDefinition (Определение кривой)
```cpp
struct CurveDefinition {
    std::string id;              // Уникальный идентификатор
    std::string displayName;     // Отображаемое имя
    std::string units;           // Единицы измерения
    int yAxisGroup;              // Группа оси (0=левая, 1=правая)
    bool enabled;                // Включена ли кривая
    COLORREF color;              // Цвет линии
    int lineStyle;               // Стиль линии (PS_SOLID, PS_DASH)
    ElementType sourceElement;   // Тип источника данных
    int elementIndex;            // Индекс элемента
    DataType dataType;           // Тип данных
    FrequencyBand frequency;     // Частотный диапазон
};
```

## 🏗️ API модулей

### UIModule (обновлено v2.2.0)

#### X Range Controls API
```cpp
class UIModule {
public:
    // Управление X Range (chartNumber: 1 или 2)
    void UpdateXRangeControls(int chartNumber, double minX, double maxX, bool isCustomRange);
    void GetXRangeValues(int chartNumber, double& fromX, double& toX) const;
    void SetXRangeValues(int chartNumber, double fromX, double toX);
    void ResetXRangeControls(int chartNumber);
    
    // Управление видимостью элементов
    void UpdateControlsVisibility(bool showChart2Controls);
    void PositionChart2Controls(int panel2Y);
    
    // Управление скроллбарами
    void ShowScrollBars(int chartNumber, bool show);
    void UpdateScrollBarPositions(int chartNumber, const RECT& chartRect);
    void SetScrollBarRange(int chartNumber, int min, int max, int page, int pos);
    HWND GetHScrollBar(int chartNumber) const;
    
private:
    // Элементы управления X Range для Chart 1 (в панели инструментов)
    HWND hXRangeFromLabel, hXRangeFromEdit, hXRangeToLabel;
    HWND hXRangeToEdit, hXRangeApplyBtn, hXRangeResetBtn;
    
    // Элементы управления X Range для Chart 2 (рядом с Device Schematic)
    HWND hXRange2FromLabel, hXRange2FromEdit, hXRange2ToLabel;
    HWND hXRange2ToEdit, hXRange2ApplyBtn, hXRange2ResetBtn;
};
```

### ChartManager (обновлено v2.2.0)

#### X Range Management API
```cpp
class ChartManager {
public:
    // НОВЫЕ методы v2.2.0 - X Range управление
    void HandleXRangeApply(HWND hWnd, int chartNumber);
    void HandleXRangeReset(HWND hWnd, int chartNumber);
    void UpdateXRangeUI(HWND hWnd);
    
private:
    // Независимые диапазоны масштабирования для каждого графика
    struct ZoomRange {
        double minX, maxX, minY, maxY;
        bool isZoomed;
        ZoomRange() : minX(0.0), maxX(0.0), minY(0.0), maxY(0.0), isZoomed(false) {}
    };
    
    ZoomRange chart1Zoom;
    ZoomRange chart2Zoom;
};
```

### DataModule

#### Основные методы
```cpp
class DataModule {
public:
    // Загрузка данных
    bool LoadDataFromFile(const std::wstring& filename);
    void ClearData();
    bool HasData() const;
    size_t GetDataSize() const;
    
    // Вычисление данных для графиков
    void CalculateChartData(DataType type, FrequencyBand freq, 
                           std::vector<std::vector<float>>& outData) const;
    
    void CalculateChartData(DataType type, FrequencyBand freq, 
                           std::vector<std::vector<float>>& outData,
                           const std::vector<bool>& enabledTransmitters,
                           double minX, double maxX, double minY, double maxY) const;
    
    // Работа с кривыми
    void CalculateCustomCurveData(const std::vector<CurveDefinition>& curves,
                                 std::vector<std::vector<float>>& leftAxisData,
                                 std::vector<std::vector<float>>& rightAxisData) const;
    
    // Утилиты
    std::wstring GetDataTypeName(DataType type) const;
    
private:
    std::vector<GP_TOOL_DATA> data;
    
    // Вспомогательные методы
    int GetActiveTransmitterIndex(Transmitter transmitter, FrequencyBand freq) const;
    int GetSilentTransmitterIndex(FrequencyBand freq) const;
    std::pair<size_t, size_t> GetDataIndexRange(double minX, double maxX) const;
    
    // Методы преобразования единиц (v2.2.2-2.2.3)
    float ConvertAmplitudeToMillivolts(float rawValue) const;  // value * 4000.0f / pow(2.0f, 24.0f)
    float ConvertPhaseToMillidegrees(float radians) const;     // value * 57295.7795f
};
```

#### Формулы преобразования единиц
```cpp
// Амплитуды: сырые данные АЦП → милливольты (v2.2.2)
float ConvertAmplitudeToMillivolts(float rawValue) {
    return rawValue * 4000.0f / pow(2.0f, 24.0f);
    // 4000 мВ - диапазон измерения
    // 2^24 - разрядность АЦП (24 бита)
}

// Фазы: радианы → миллиградусы (v2.2.3)
float ConvertPhaseToMillidegrees(float radians) {
    return radians * 57295.7795f;
    // 57295.7795 = (180 / π) * 1000
    // Преобразование радианов в градусы и умножение на 1000
}
```

### ChartModule

#### Основные методы
```cpp
class ChartModule {
public:
    // Основная отрисовка
    void DrawChart(HDC hdc, const RECT& rect, 
                   const std::vector<std::vector<float>>& chartData,
                   DataType type, const std::wstring& title, bool hasData,
                   const std::vector<bool>& enabledTransmitters, 
                   const DataModule* dataModule,
                   double minXData, double maxXData, 
                   double minYData, double maxYData, 
                   bool showAxes = true) const;
    
    // Двойные оси
    void DrawDualAxisChart(HDC hdc, const RECT& rect, 
                          const std::vector<CurveDefinition>& curves,
                          const std::vector<std::vector<float>>& leftAxisData,
                          const std::vector<std::vector<float>>& rightAxisData,
                          const std::wstring& title, bool hasData) const;
    
    // Интерактивность
    void DrawZoomRectangle(HDC hdc, const RECT& zoomRect) const;
    int HitTestLegendCheckbox(const RECT& chartRect, const POINT& pt, DataType type) const;
    
private:
    // Цвета каналов
    static const COLORREF CHANNEL_COLORS[4];
    static const wchar_t* CHANNEL_NAMES[4];
    
    // Вспомогательные методы отрисовки
    void DrawChartBackground(HDC hdc, const RECT& rect) const;
    void DrawChartData(HDC hdc, const RECT& plotRect, 
                       const std::vector<std::vector<float>>& chartData) const;
    void DrawChartLegend(HDC hdc, const RECT& rect, 
                         const std::vector<std::vector<float>>& chartData,
                         DataType type, const std::vector<bool>& enabledTransmitters) const;
    void DrawDualAxisLegend(HDC hdc, const RECT& rect,
                           const std::vector<CurveDefinition>& curves) const;
};
```

### SchematicModule

#### Основные методы
```cpp
class SchematicModule {
public:
    // Диалог схемы прибора
    bool ShowSchematicDialog(HWND parent, int chartNumber, ChartConfiguration& config);
    
    // Отрисовка схемы
    void RenderSchematic(HDC hdc, const RECT& rect);
    
    // Обработка событий
    bool HandleElementClick(int x, int y);
    void UpdateSelection(ElementType type, int index, bool selected);
    
    // Управление выбором
    SchematicSelection GetCurrentSelection() const;
    void SetCurrentSelection(const SchematicSelection& selection);
    bool ValidateCurrentSelection();
    
    // Интеграция с модулем кривых
    void SetCurveSelectionModule(CurveSelectionModule* module);
    
private:
    // Элементы схемы
    struct ElementRect {
        RECT rect;
        ElementType type;
        int index;
        bool selected;
    };
    
    std::vector<ElementRect> schematicElements;
    SchematicSelection currentSelection;
    std::vector<CurveDefinition> currentAvailableCurves;
    
    // Цвета элементов
    static const COLORREF TRANSMITTER_COLOR;
    static const COLORREF RECEIVER_COLOR;
    static const COLORREF SILENT_TX_COLOR;
    static const COLORREF SELECTION_COLOR;
};
```

### CurveSelectionModule

#### Основные методы
```cpp
class CurveSelectionModule {
public:
    // Получение доступных кривых
    std::vector<CurveDefinition> GetAvailableCurves(const SchematicSelection& selection) const;
    
    // Управление конфигурациями
    void ApplyCurveConfiguration(int chartNumber, const ChartConfiguration& config);
    ChartConfiguration GetChartConfiguration(int chartNumber) const;
    bool HasConfiguration(int chartNumber) const;
    
    // Пресеты
    void ApplyPreset(PresetType preset, int chartNumber, FrequencyBand frequency);
    ChartConfiguration CreatePresetConfiguration(PresetType preset, FrequencyBand frequency) const;
    
    // Копирование настроек
    void CopySettings(int fromChart, int toChart);
    
    // Валидация
    bool ValidateConfiguration(const ChartConfiguration& config) const;
    std::string GetConfigurationError(const ChartConfiguration& config) const;
    
    // Вычисление данных кривых
    void CalculateCurveData(const ChartConfiguration& config, 
                           const DataModule* dataModule,
                           std::vector<std::vector<float>>& leftAxisData,
                           std::vector<std::vector<float>>& rightAxisData,
                           std::vector<CurveDefinition>& leftAxisCurves,
                           std::vector<CurveDefinition>& rightAxisCurves) const;
    
private:
    ChartConfiguration chart1Config;
    ChartConfiguration chart2Config;
    
    // Цвета кривых
    static const COLORREF CURVE_COLORS[4];
    
    // Методы создания кривых
    CurveDefinition CreateRatioCurve(int transmitterIndex, FrequencyBand frequency) const;
    CurveDefinition CreateDifferenceCurve(int transmitterIndex, FrequencyBand frequency) const;
    CurveDefinition CreateAmplitudeCurve(int transmitterIndex, int receiverIndex, 
                                        FrequencyBand frequency) const;
    CurveDefinition CreatePhaseCurve(int transmitterIndex, int receiverIndex, 
                                    FrequencyBand frequency) const;
};
```

## 🔄 Алгоритмы и логика

### Логика выбора элементов схемы

```cpp
// Правила валидации выбора (ОБНОВЛЕНО в версии 2.1.1)
bool SchematicSelection::IsValid() const {
    int receivers = GetSelectedReceiversCount();
    int transmitters = GetSelectedTransmittersCount();
    int silent = GetSelectedSilentCount();
    
    // Должен быть выбран хотя бы один элемент
    if (receivers == 0 && transmitters == 0 && silent == 0) {
        return false;
    }
    
    // Нельзя одновременно выбирать передатчики и приемники
    if (transmitters > 0 && receivers > 0) {
        return false;
    }
    
    // ИСПРАВЛЕНО: Разрешены комбинации молчащих с активными элементами
    // Запрещена только комбинация: оба приемника + молчащие
    if (silent > 0 && receivers == 2) {
        return false;
    }
    
    return true;
}
```

### Определение доступных кривых

```cpp
std::vector<CurveDefinition> GetAvailableCurves(const SchematicSelection& selection) {
    if (selectedReceivers == 2) {
        // Два приемника -> отношения амплитуд и разности фаз (4 кривые T1-T4)
        return CreateTwoReceiversCurves(selection);
    }
    else if (selectedReceivers == 1) {
        // Один приемник -> амплитуды и фазы для T1-T4 (8 кривых)
        return CreateSingleReceiverCurves(selection);
    }
    else if (selectedTransmitters > 0) {
        // Выбранные передатчики -> амплитуды и фазы для R1 и R2
        return CreateTransmitterCurves(selection);
    }
    else if (selectedSilent > 0) {
        // Молчащие передатчики -> только амплитуды
        return CreateSilentTransmitterCurves(selection);
    }
}
```

### Алгоритм масштабирования

```cpp
void HandleMouseUp(int x, int y) {
    // Преобразуем экранные координаты в координаты данных
    double minDataX, minDataY, maxDataX, maxDataY;
    
    ConvertScreenToDataCoords(activeChart, 
        min(selectionStart.x, selectionEnd.x), 
        max(selectionStart.y, selectionEnd.y), 
        minDataX, minDataY);
        
    ConvertScreenToDataCoords(activeChart, 
        max(selectionStart.x, selectionEnd.x), 
        min(selectionStart.y, selectionEnd.y), 
        maxDataX, maxDataY);
    
    // Устанавливаем новый видимый диапазон
    if (activeChart == 1) {
        chart1Zoom.minX = minDataX;
        chart1Zoom.maxX = maxDataX;
        chart1Zoom.minY = minDataY;
        chart1Zoom.maxY = maxDataY;
        chart1Zoom.isZoomed = true;
    }
}
```

### Вычисление данных графика

```cpp
void CalculateChartData(DataType type, FrequencyBand freq, 
                       std::vector<std::vector<float>>& outData,
                       const std::vector<bool>& enabledTransmitters,
                       double minX, double maxX, double minY, double maxY) {
    
    // Определяем диапазон индексов данных
    auto indexRange = GetDataIndexRange(minX, maxX);
    size_t startIndex = indexRange.first;
    size_t endIndex = indexRange.second;
    
    // Создаем каналы для активных передатчиков
    outData.resize(4); // T1-T4
    
    for (size_t i = startIndex; i < endIndex; ++i) {
        const GP_TOOL_DATA& record = data[i];
        
        for (int t = 0; t < 4; ++t) {
            if (!enabledTransmitters[t]) continue;
            
            int rxIdx = GetActiveTransmitterIndex(static_cast<Transmitter>(t), freq);
            float value = 0.0f;
            
            switch (type) {
                case DataType::PHASE_DIFFERENCE:
                    value = record.PH_RX_2[rxIdx] - record.PH_RX_1[rxIdx];
                    break;
                case DataType::AMPLITUDE_RATIO:
                    value = (record.AM_RX_1[rxIdx] != 0.0f) ? 
                           (record.AM_RX_2[rxIdx] / record.AM_RX_1[rxIdx]) : 0.0f;
                    break;
                // ... другие типы данных
            }
            
            if (value >= minY && value <= maxY) {
                outData[t].push_back(value);
            }
        }
    }
}
```

## 🎨 Константы и настройки

### Новые ID элементов управления v2.2.0
```cpp
// X Range Controls для Chart 1 (в панели инструментов)
#define IDC_XRANGE_FROM_LABEL    2001
#define IDC_XRANGE_FROM_EDIT     2002
#define IDC_XRANGE_TO_LABEL      2003
#define IDC_XRANGE_TO_EDIT       2004
#define IDC_XRANGE_APPLY_BTN     2005
#define IDC_XRANGE_RESET_BTN     2006

// X Range Controls для Chart 2 (рядом с Device Schematic)
#define IDC_XRANGE2_FROM_LABEL   2007
#define IDC_XRANGE2_FROM_EDIT    2008
#define IDC_XRANGE2_TO_LABEL     2009
#define IDC_XRANGE2_TO_EDIT      2010
#define IDC_XRANGE2_APPLY_BTN    2011
#define IDC_XRANGE2_RESET_BTN    2012
```

### Цвета Zoom Selection v2.1.5+
```cpp
// Улучшенное выделение области zoom
const COLORREF ZOOM_SELECTION_COLOR = RGB(255, 165, 0);  // Оранжевый
const int ZOOM_SELECTION_HATCH = HS_DIAGCROSS;           // Диагональная штриховка
```

### Цвета каналов
```cpp
const COLORREF CHANNEL_COLORS[4] = {
    RGB(0, 255, 0),    // T1 - Зеленый
    RGB(255, 255, 0),  // T2 - Желтый
    RGB(255, 0, 0),    // T3 - Красный
    RGB(0, 0, 255)     // T4 - Синий
};
```

### Система умного назначения цветов

#### Расширенная палитра (12 цветов)
```cpp
const COLORREF EXTENDED_PALETTE[12] = {
    RGB(0, 128, 0),      // Зелёный (приоритет T1 на левой оси)
    RGB(255, 255, 0),    // Жёлтый (приоритет T2 на левой оси)
    RGB(255, 0, 0),      // Красный (приоритет T3 на левой оси)
    RGB(0, 0, 255),      // Синий (приоритет T4 на левой оси)
    RGB(0, 100, 100),    // Тёмно-бирюзовый
    RGB(139, 69, 19),    // Коричневый
    RGB(128, 0, 128),    // Фиолетовый
    RGB(255, 20, 147),   // Розовый
    RGB(255, 140, 0),    // Тёмно-оранжевый
    RGB(0, 128, 128),    // Бирюзовый
    RGB(128, 128, 0),    // Оливковый
    RGB(75, 0, 130)      // Индиго
};
```

#### Алгоритм умного назначения цветов
```cpp
COLORREF AssignSmartColor(const std::vector<CurveDefinition>& existingCurves,
                         int transmitterIndex, int yAxisGroup) {
    // 1. Собираем уже использованные цвета на этой оси
    std::vector<COLORREF> usedColors;
    for (const auto& curve : existingCurves) {
        if (curve.yAxisGroup == yAxisGroup && curve.enabled) {
            usedColors.push_back(curve.color);
        }
    }
    
    // 2. Для T1-T4 на левой оси используем приоритетные цвета
    if (yAxisGroup == 0 && transmitterIndex >= 0 && transmitterIndex < 4) {
        COLORREF priorityColor = LEFT_AXIS_COLORS[transmitterIndex];
        if (!IsColorUsed(priorityColor, usedColors)) {
            return priorityColor;
        }
    }
    
    // 3. Ищем первый свободный цвет из расширенной палитры
    for (int i = 0; i < 12; i++) {
        COLORREF candidateColor = EXTENDED_PALETTE[i];
        if (!IsColorUsed(candidateColor, usedColors)) {
            return candidateColor;
        }
    }
    
    // 4. Если все цвета заняты, возвращаем чёрный
    return RGB(0, 0, 0);
}
```

#### Логика назначения цветов
- **Приоритет для T1-T4 на левой оси**: Каждому передатчику назначается свой приоритетный цвет (зелёный, жёлтый, красный, синий)
- **Анализ использованных цветов**: Перед назначением проверяются уже использованные цвета на той же оси
- **Избежание конфликтов**: Если приоритетный цвет занят, выбирается первый свободный из расширенной палитры
- **Поддержка до 12 кривых**: Расширенная палитра позволяет различать до 12 кривых на одной оси
- **Независимость осей**: Цвета назначаются независимо для левой и правой осей

### Цвета элементов схемы
```cpp
const COLORREF TRANSMITTER_COLOR = RGB(180, 100, 100);  // Приглушенный красный
const COLORREF RECEIVER_COLOR = RGB(100, 180, 100);     // Приглушенный зеленый  
const COLORREF SILENT_TX_COLOR = RGB(128, 128, 128);    // Серый
const COLORREF SELECTION_COLOR = RGB(255, 255, 0);      // Желтый для выделения
```

### Ограничения системы
```cpp
const int MAX_CURVES_PER_CHART = 8;        // Максимум кривых на графике
const int MAX_TRANSMITTERS = 4;            // Количество передатчиков
const int MAX_RECEIVERS = 2;               // Количество приемников
const int MAX_CHANNELS = 10;               // Каналов в структуре данных
const int MIN_WINDOW_WIDTH = 800;          // Минимальная ширина окна
const int MIN_WINDOW_HEIGHT = 600;         // Минимальная высота окна
```

## 📁 Форматы файлов

### Бинарный формат данных
```
Файл: *.bin
Структура: последовательность записей GP_TOOL_DATA
Размер записи: sizeof(GP_TOOL_DATA) = 164 байта
Порядок байтов: Little Endian (Intel)
Тип данных: IEEE 754 float (32-bit)
```

### Структура записи
```
Offset  Size  Field
0       40    AM_RX_1[10] (float[10])
40      40    AM_RX_2[10] (float[10])  
80      40    PH_RX_1[10] (float[10])
120     40    PH_RX_2[10] (float[10])
160     4     temperature (float)
```

## 🔧 Конфигурация и настройки

### Настройки компоновки
```cpp
struct LayoutSettings {
    int chartMargin = 10;           // Отступы между графиками
    int controlPanelHeight = 60;    // Высота панели управления
    int minChartHeight = 200;       // Минимальная высота графика
    int legendWidth = 200;          // Ширина области легенды
    int axisLabelMargin = 50;       // Отступ для подписей осей
};
```

### Настройки отрисовки
```cpp
struct RenderSettings {
    int gridLineWidth = 1;          // Толщина линий сетки
    int dataLineWidth = 2;          // Толщина линий данных
    int axisLineWidth = 2;          // Толщина осей координат
    COLORREF gridColor = RGB(200, 200, 200);     // Цвет сетки
    COLORREF backgroundColor = RGB(255, 255, 255); // Цвет фона
    COLORREF textColor = RGB(0, 0, 0);           // Цвет текста
};
```

## 🚀 Оптимизация производительности

### Кэширование данных
- Вычисленные данные графиков кэшируются до изменения параметров
- Индексы каналов предвычисляются для оптимизации циклов
- Диапазоны данных рассчитываются один раз при масштабировании

### Оптимизация отрисовки
- Используется двойная буферизация для плавной анимации
- Перерисовка только измененных областей
- Оптимизированные алгоритмы поиска min/max значений

### Управление памятью
- Автоматическое освобождение GDI объектов
- Резервирование памяти для векторов данных
- Ленивая инициализация тяжелых объектов

---

**Техническая документация AMK-Horizon-New-Chart** - полный справочник для разработчиков и системных интеграторов.