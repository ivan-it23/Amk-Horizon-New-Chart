# Дизайн системы гибкого выбора кривых

## Обзор

Система гибкого выбора кривых расширяет существующую модульную архитектуру AMK-Horizon-Chart, добавляя интерактивную схему прибора, поддержку двойных осей ординат и гибкие возможности выбора данных для визуализации.

## Архитектура

### Новые модули

#### SchematicModule
Отвечает за интерактивную схему прибора и модальное окно выбора.

**Основные компоненты:**
- `SchematicDialog` - модальное окно со схемой прибора
- `SchematicRenderer` - отрисовка схемы с интерактивными элементами
- `ElementSelector` - логика выбора элементов и валидации

**Ключевые методы:**
```cpp
class SchematicModule {
public:
    void ShowSchematicDialog(HWND parent, int chartNumber);
    void RenderSchematic(HDC hdc, const RECT& rect);
    bool HandleElementClick(int x, int y);
    void ValidateSelection();
    SchematicSelection GetCurrentSelection();
};
```

#### CurveSelectionModule
Управляет логикой определения доступных кривых и их конфигурацией.

**Основные компоненты:**
- `CurveDefinitionEngine` - определение доступных кривых на основе выбора
- `CurveConfigPanel` - панель настройки кривых в модальном окне
- `PresetManager` - управление быстрыми пресетами

**Ключевые методы:**
```cpp
class CurveSelectionModule {
public:
    std::vector<CurveDefinition> GetAvailableCurves(const SchematicSelection& selection);
    void ApplyCurveConfiguration(int chartNumber, const std::vector<CurveDefinition>& curves);
    void ApplyPreset(PresetType preset, int chartNumber);
    void CopySettings(int fromChart, int toChart);
};
```

### Расширения существующих модулей

#### ChartModule (расширение)
Добавляет поддержку двойных осей ординат.

**Новые возможности:**
- Отрисовка двух независимых осей Y
- Различные стили линий (сплошные/пунктирные)
- Независимое масштабирование осей
- Подписи осей с единицами измерения

**Новые методы:**
```cpp
void DrawDualAxisChart(HDC hdc, const RECT& rect, 
                      const std::vector<CurveDefinition>& curves,
                      const std::vector<std::vector<float>>& leftAxisData,
                      const std::vector<std::vector<float>>& rightAxisData);
void DrawAxisLabels(HDC hdc, const RECT& rect, 
                   const std::string& leftUnits, const std::string& rightUnits);
```

#### UIModule (расширение)
Добавляет кнопки для новой функциональности.

**Новые элементы управления:**
- Кнопка "Схема прибора" для каждого графика
- Кнопки быстрых пресетов
- Кнопка копирования настроек между графиками

#### DataModule (расширение)
Расширяет возможности вычисления данных для гибкого выбора кривых.

**Новые методы:**
```cpp
void CalculateCustomCurveData(const std::vector<CurveDefinition>& curves,
                             FrequencyBand frequency,
                             std::vector<std::vector<float>>& leftAxisData,
                             std::vector<std::vector<float>>& rightAxisData);
```

## Компоненты и интерфейсы

### Структуры данных

```cpp
// Определение кривой
struct CurveDefinition {
    std::string id;              // "T1_R1_AMP_400"
    std::string displayName;     // "T1 Амплитуда R1"
    std::string units;           // "В" или "°"
    int yAxisGroup;             // 0=левая ось, 1=правая ось
    bool enabled;
    COLORREF color;
    int lineStyle;              // PS_SOLID или PS_DASH
    
    // Параметры для вычисления данных
    ElementType sourceElement;   // TRANSMITTER, RECEIVER, SILENT
    int elementIndex;           // 0-3 для T1-T4, 0-1 для R1-R2
    DataType dataType;          // AMPLITUDE, PHASE, RATIO, DIFFERENCE
    FrequencyBand frequency;
};

// Выбор элементов на схеме
struct SchematicSelection {
    std::vector<bool> transmitters;  // T1-T4 [4]
    std::vector<bool> receivers;     // R1-R2 [2]
    std::vector<bool> silentTx;      // Молчащие [2] (для R1 и R2)
    FrequencyBand frequency;
    
    bool IsValid() const;
    std::vector<CurveDefinition> GetAvailableCurves() const;
};

// Конфигурация графика
struct ChartConfiguration {
    std::vector<CurveDefinition> curves;
    bool hasDualAxis;
    std::string leftAxisUnits;
    std::string rightAxisUnits;
    
    void Clear();
    bool IsEmpty() const;
    ChartConfiguration Copy() const;
};

// Типы быстрых пресетов
enum class PresetType {
    ALL_AMPLITUDES_R1,
    ALL_AMPLITUDES_R2,
    ALL_PHASES_R1,
    ALL_PHASES_R2,
    AMPLITUDE_RATIOS,
    PHASE_DIFFERENCES,
    CUSTOM
};
```

### Модальное окно схемы прибора

**Компоновка окна:**
```
┌─────────────────────────────────────────────────────────┐
│ Схема прибора - График 1                               │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  [Схема прибора с интерактивными элементами]           │
│                                                         │
├─────────────────────────────────────────────────────────┤
│ Доступные кривые:                                       │
│ ☐ T1 Амплитуда R1 [Левая ось ▼] [400 кГц ▼]           │
│ ☐ T2 Амплитуда R1 [Левая ось ▼] [400 кГц ▼]           │
│ ...                                                     │
├─────────────────────────────────────────────────────────┤
│ Быстрые пресеты:                                        │
│ [Все амп. R1] [Все фазы R2] [Отношения] [Разности]     │
├─────────────────────────────────────────────────────────┤
│              [Применить] [Отмена] [Копировать из...]    │
└─────────────────────────────────────────────────────────┘
```

## Модели данных

### Цветовая схема

**Элементы схемы:**
- Передатчики T1-T4: `RGB(180, 100, 100)` (приглушенный красный)
- Приемники R1-R2: `RGB(100, 180, 100)` (приглушенный зеленый)
- Молчащие передатчики: `RGB(128, 128, 128)` (серый)
- Выделение активного элемента: `RGB(255, 255, 0)` (желтая рамка)

**Кривые на графике:**
- Левая ось (сплошные линии): стандартная палитра
- Правая ось (пунктирные линии): та же палитра, но пунктирный стиль

### Логика определения кривых

```cpp
std::vector<CurveDefinition> DetermineCurves(const SchematicSelection& selection) {
    std::vector<CurveDefinition> curves;
    
    int selectedReceivers = CountSelected(selection.receivers);
    int selectedTransmitters = CountSelected(selection.transmitters);
    int selectedSilent = CountSelected(selection.silentTx);
    
    if (selectedReceivers == 2) {
        // Два приемника -> отношения и разности
        AddRatioCurves(curves, selection);
        AddDifferenceCurves(curves, selection);
    }
    else if (selectedReceivers == 1) {
        // Один приемник -> амплитуды и фазы для T1-T4
        AddSingleReceiverCurves(curves, selection);
    }
    else if (selectedTransmitters > 0) {
        // Передатчики -> амплитуды и фазы для выбранных R
        AddTransmitterCurves(curves, selection);
    }
    else if (selectedSilent > 0) {
        // Молчащие -> только амплитуды
        AddSilentCurves(curves, selection);
    }
    
    return curves;
}
```

## Обработка ошибок

### Валидация выбора

**Правила валидации:**
1. Нельзя выбрать передатчики и приемники одновременно
2. Для отношений/разностей нужны оба приемника
3. Молчащие передатчики нельзя комбинировать с активными
4. Должен быть выбран хотя бы один элемент

**Сообщения об ошибках:**
- "Выберите элементы для построения графика"
- "Для отношений амплитуд необходимо выбрать оба приемника"
- "Нельзя одновременно выбирать передатчики и приемники"
- "Молчащие передатчики нельзя комбинировать с активными элементами"

## Стратегия тестирования

### Модульные тесты
- Валидация выбора элементов
- Определение доступных кривых
- Вычисление данных для кривых
- Применение пресетов

### Интеграционные тесты
- Взаимодействие с существующими модулями
- Корректность отрисовки двойных осей
- Копирование настроек между графиками

### Пользовательские тесты
- Интуитивность интерфейса схемы
- Корректность отображения кривых
- Производительность при большом количестве данных