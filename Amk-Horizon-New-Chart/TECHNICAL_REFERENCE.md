# 🔧 Техническая документация AMK-Horizon-New-Chart

## 📊 Структуры данных

### GP_TOOL_DATA
```cpp
struct GP_TOOL_DATA {
    float AM_RX_1[10];    // Амплитуды приемника 1
    float AM_RX_2[10];    // Амплитуды приемника 2
    float PH_RX_1[10];    // Фазы приемника 1
    float PH_RX_2[10];    // Фазы приемника 2
    float temperature;    // Температура прибора
};
```

### Индексация каналов
- **400 kHz**: Каналы 0-1 (Silent), 2-5 (T1-T4), 6-9 (резерв)
- **2000 kHz**: Каналы 0-1 (Silent), 2-5 (резерв), 6-9 (T1-T4)

### DataType
```cpp
enum class DataType {
    PHASE_DIFFERENCE,      // Разности фаз R2-R1
    AMPLITUDE_RATIO,       // Отношения амплитуд R2/R1
    AMPLITUDE_RX1,         // Амплитуды приемника 1
    AMPLITUDE_RX2,         // Амплитуды приемника 2
    PHASE_RX1,            // Фазы приемника 1
    PHASE_RX2,            // Фазы приемника 2
    SILENT_AMPLITUDE_RX1,  // Амплитуды молчащих для R1
    SILENT_AMPLITUDE_RX2,  // Амплитуды молчащих для R2
    TEMPERATURE           // Температура
};
```

### CurveDefinition
```cpp
struct CurveDefinition {
    std::string id;              // Уникальный идентификатор
    std::string displayName;     // Отображаемое имя
    std::string units;           // Единицы измерения
    int yAxisGroup;              // 0=левая ось, 1=правая ось
    bool enabled;                // Включена ли кривая
    COLORREF color;              // Цвет линии
    int lineStyle;               // PS_SOLID или PS_DASH
    ElementType sourceElement;   // Тип источника данных
    int elementIndex;            // Индекс элемента
    DataType dataType;           // Тип данных
    FrequencyBand frequency;     // Частотный диапазон
};
```

## 🔄 Формулы преобразования данных

### Нормализация фаз (v2.3.0)
```cpp
float NormalizePhase(float phaseRadians) {
    const float PI = 3.14159265358979323846f;
    while (phaseRadians > PI) phaseRadians -= 2.0f * PI;
    while (phaseRadians <= -PI) phaseRadians += 2.0f * PI;
    return phaseRadians;
}
// Приводит фазу к диапазону (-π, π]
```

### Преобразование амплитуд (v2.2.2)
```cpp
float ConvertAmplitudeToMillivolts(float rawValue) {
    return rawValue * 4000.0f / pow(2.0f, 24.0f);
}
// Сырые данные АЦП → милливольты (mV)
```

### Преобразование фаз (v2.2.3)
```cpp
float ConvertPhaseToMillidegrees(float radians) {
    return radians * 57295.7795f;  // (180/π) * 1000
}
// Радианы → миллиградусы (mdeg)
```

### Полный цикл обработки фазы
1. Чтение сырых данных в радианах
2. Нормализация: `NormalizePhase(phase)`
3. Преобразование: `phase * 57295.7795f`

## 📊 Расположение осей Y (v2.7.0)

### Логика двойных осей
При отображении графиков с двумя типами данных (амплитуды + фазы):
- **Левая ось Y**: Фазы в миллиградусах (мдег)
- **Правая ось Y**: Амплитуды в милливольтах (мВ)

### Назначение yAxisGroup
```cpp
// Фазовые данные
curve.yAxisGroup = 0;  // Левая ось (мдег)

// Амплитудные данные
curve.yAxisGroup = 1;  // Правая ось (мВ)
```

### Однородные данные
При отображении только одного типа данных:
- **Только фазы**: Левая ось (мдег)
- **Только амплитуды**: Левая ось (мВ)

## 🏗️ Основные модули

### DataModule
```cpp
// Загрузка данных
bool LoadDataFromFile(const std::wstring& filename);
size_t GetDataSize() const;

// Вычисление данных для графиков
void CalculateChartData(DataType type, FrequencyBand freq, 
                       std::vector<std::vector<float>>& outData) const;

// Работа с кривыми
void CalculateCustomCurveData(const std::vector<CurveDefinition>& curves,
                             std::vector<std::vector<float>>& leftAxisData,
                             std::vector<std::vector<float>>& rightAxisData) const;
```

### ChartModule
```cpp
// Основная отрисовка
void DrawChart(HDC hdc, const RECT& rect, 
               const std::vector<std::vector<float>>& chartData,
               DataType type, const std::wstring& title, bool hasData) const;

// Двойные оси
void DrawDualAxisChart(HDC hdc, const RECT& rect, 
                      const std::vector<CurveDefinition>& curves,
                      const std::vector<std::vector<float>>& leftAxisData,
                      const std::vector<std::vector<float>>& rightAxisData,
                      const std::wstring& title, bool hasData) const;

// Интерактивность
int HitTestLegendCheckbox(const RECT& chartRect, const POINT& pt, DataType type) const;
```

### SchematicModule
```cpp
// Диалог схемы прибора
bool ShowSchematicDialog(HWND parent, int chartNumber, ChartConfiguration& config);

// Обработка событий
bool HandleElementClick(int x, int y);
void UpdateSelection(ElementType type, int index, bool selected);

// Управление выбором
SchematicSelection GetCurrentSelection() const;
bool ValidateCurrentSelection();
```

### CurveSelectionModule
```cpp
// Получение доступных кривых
std::vector<CurveDefinition> GetAvailableCurves(const SchematicSelection& selection) const;

// Управление конфигурациями
void ApplyCurveConfiguration(int chartNumber, const ChartConfiguration& config);
ChartConfiguration GetChartConfiguration(int chartNumber) const;

// Пресеты
void ApplyPreset(PresetType preset, int chartNumber, FrequencyBand frequency);

// Вычисление данных кривых
void CalculateCurveData(const ChartConfiguration& config, 
                       const DataModule* dataModule,
                       std::vector<std::vector<float>>& leftAxisData,
                       std::vector<std::vector<float>>& rightAxisData) const;
```

## 🎨 Константы

### Цвета каналов T1-T4
```cpp
const COLORREF CHANNEL_COLORS[4] = {
    RGB(0, 255, 0),    // T1 - Зеленый
    RGB(255, 255, 0),  // T2 - Желтый
    RGB(255, 0, 0),    // T3 - Красный
    RGB(0, 0, 255)     // T4 - Синий
};
```

### Расширенная палитра (12 цветов)
```cpp
const COLORREF EXTENDED_PALETTE[12] = {
    RGB(0, 128, 0),      // Зелёный (T1)
    RGB(255, 255, 0),    // Жёлтый (T2)
    RGB(255, 0, 0),      // Красный (T3)
    RGB(0, 0, 255),      // Синий (T4)
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

### Цвета элементов схемы
```cpp
const COLORREF TRANSMITTER_COLOR = RGB(180, 100, 100);  // Передатчики
const COLORREF RECEIVER_COLOR = RGB(100, 180, 100);     // Приемники
const COLORREF SILENT_TX_COLOR = RGB(128, 128, 128);    // Молчащие
const COLORREF SELECTION_COLOR = RGB(255, 255, 0);      // Выделение
```

## 📁 Формат файлов

### Бинарный формат данных
- **Расширение**: *.bin
- **Структура**: Последовательность записей GP_TOOL_DATA
- **Размер записи**: 164 байта
- **Порядок байтов**: Little Endian
- **Тип данных**: IEEE 754 float (32-bit)

### Структура записи
```
Offset  Size  Field
0       40    AM_RX_1[10]
40      40    AM_RX_2[10]
80      40    PH_RX_1[10]
120     40    PH_RX_2[10]
160     4     temperature
```

## 🔧 Логика выбора элементов

### Правила валидации
```cpp
bool IsValid() const {
    // Должен быть выбран хотя бы один элемент
    if (receivers == 0 && transmitters == 0 && silent == 0) return false;
    
    // Нельзя одновременно выбирать передатчики и приемники
    if (transmitters > 0 && receivers > 0) return false;
    
    // Запрещена комбинация: оба приемника + молчащие
    if (silent > 0 && receivers == 2) return false;
    
    return true;
}
```

### Определение доступных кривых
- **Два приемника (R1+R2)**: Отношения амплитуд и разности фаз (4 кривые T1-T4)
- **Один приемник (R1 или R2)**: Амплитуды и фазы для T1-T4 (8 кривых)
- **Выбранные передатчики**: Амплитуды и фазы для R1 и R2
- **Молчащие передатчики**: Только амплитуды

---

**Техническая документация AMK-Horizon-New-Chart** - справочник для разработчиков
