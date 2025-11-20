# 🔧 Техническая документация AMK-Horizon-New-Chart

## 📊 Структуры данных

### XAxisType (v2.10.0)
```cpp
enum class XAxisType {
    FRAME_NUMBER,   // Номер кадра (по умолчанию)
    TEMPERATURE     // Температура
};
```

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

## 📊 Статусная строка (v2.8.0)

### Структура статусной строки
Статусная строка состоит из 4 частей:
```cpp
int statusBarParts[4] = { 150, 350, 550, -1 };
// Часть 0: X координата (0-150 пикселей)
// Часть 1: Y₁ координата (150-350 пикселей)
// Часть 2: Y₂ координата (350-550 пикселей)
// Часть 3: Информация о файле (550-конец)
```

### Преобразование координат
```cpp
// Экранные координаты → Координаты данных
bool ConvertScreenToChartCoords(int chartNum, int screenX, int screenY,
                                double& dataX, double& dataY1, double& dataY2,
                                std::wstring& y1Unit, std::wstring& y2Unit,
                                bool& hasDualAxis) const;
```

### Формулы преобразования
```cpp
// X координата
dataX = minX + (double)relX / plotWidth * (maxX - minX);

// Y координата (инвертированная, т.к. Y растёт вниз)
dataY = maxY - (double)relY / plotHeight * (maxY - minY);
```

### Форматирование координат
```cpp
// Фазы (mdeg) - целые числа
swprintf_s(text, L"Y: %.0f mdeg", value);

// Амплитуды (mV) - 3 знака
swprintf_s(text, L"Y: %.3f mV", value);

// Отношения (V/V) - 4 знака
swprintf_s(text, L"Y: %.4f V/V", value);

// Температура (°C) - 2 знака
swprintf_s(text, L"Y: %.2f °C", value);
```

### Интеграция с Layout
```cpp
// Константа высоты статусной строки
const int STATUSBAR_HEIGHT = 25;

// Учёт при расчёте layout
chart1Rect.bottom = windowHeight - MARGIN - STATUSBAR_HEIGHT;
```

## 🏗️ Основные модули

### DataModule
```cpp
// Загрузка данных
bool LoadDataFromFile(const std::wstring& filename);
size_t GetDataSize() const;
std::wstring GetLoadedFileName() const;

// Вычисление данных для графиков
void CalculateChartData(DataType type, FrequencyBand freq, 
                       std::vector<std::vector<float>>& outData) const;

// Работа с кривыми
void CalculateCustomCurveData(const std::vector<CurveDefinition>& curves,
                             std::vector<std::vector<float>>& leftAxisData,
                             std::vector<std::vector<float>>& rightAxisData) const;
```

### UIModule
```cpp
// Создание элементов управления
void CreateControls(HWND hWnd, HINSTANCE hInst);
void UpdateControlsVisibility(bool showChart2Controls);

// Управление статусной строкой (v2.8.0)
void UpdateStatusBar(const std::wstring& text);
void UpdateStatusBarCoordinates(double x, double y1, const std::wstring& y1Unit,
                                double y2, const std::wstring& y2Unit,
                                bool hasDualAxis);
void ClearStatusBarCoordinates();
void ResizeStatusBar();
HWND GetStatusBar() const;

// Управление X Range
void UpdateXRangeControls(int chartNumber, double minX, double maxX, bool isCustomRange);
void GetXRangeValues(int chartNumber, double& fromX, double& toX) const;
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

// Получение диапазонов данных (v2.8.0)
void GetDataRanges(const std::vector<std::vector<float>>& leftAxisData,
                  const std::vector<std::vector<float>>& rightAxisData,
                  double& minX, double& maxX,
                  double& minY1, double& maxY1,
                  double& minY2, double& maxY2) const;

// Интерактивность
int HitTestLegendCheckbox(const RECT& chartRect, const POINT& pt, DataType type) const;
```

### ChartManager
```cpp
// Обработка событий мыши (v2.8.0)
void HandleMouseMoveForCoordinates(HWND hWnd, int x, int y);

// Преобразование координат (v2.8.0)
bool ConvertScreenToChartCoords(int chartNum, int screenX, int screenY,
                                double& dataX, double& dataY1, double& dataY2,
                                std::wstring& y1Unit, std::wstring& y2Unit,
                                bool& hasDualAxis) const;

// Получение единиц измерения (v2.8.0)
std::wstring GetUnitForDataType(DataType dataType) const;

// Обновление информации о файле (v2.8.0)
void UpdateStatusBarFileInfo();
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


## 🔬 Кроссплот: Температура на оси X (v2.10.0)

### Обзор
Версия 2.10.0 добавляет возможность отображения температуры на оси X вместо номера кадра, что позволяет строить кроссплоты для анализа температурных зависимостей.

### Новые методы DataModule

#### GetTemperatureData
```cpp
void GetTemperatureData(std::vector<float>& outData) const;
void GetTemperatureData(std::vector<float>& outData, double minX, double maxX) const;
```

**Назначение**: Получение массива температур для использования на оси X

**Параметры**:
- `outData` - выходной массив температур
- `minX`, `maxX` - диапазон индексов данных (опционально)

**Пример использования**:
```cpp
std::vector<float> temperatures;
dataModule->GetTemperatureData(temperatures);
// temperatures[i] содержит температуру для i-го кадра
```

### Новые методы ChartModule

#### DrawChart с температурой
```cpp
void DrawChart(HDC hdc, const RECT& rect, 
               const std::vector<std::vector<float>>& chartData,
               DataType type, const std::wstring& title, bool hasData,
               const std::vector<bool>& enabledTransmitters, 
               const DataModule* dataModule,
               double minXData, double maxXData, 
               double minYData, double maxYData,
               const std::vector<float>& xAxisData, 
               bool showAxes = true) const;
```

**Новый параметр**: `xAxisData` - массив значений для оси X (температуры)

#### DrawChartData с температурой
```cpp
void DrawChartData(HDC hdc, const RECT& plotRect, 
                   const std::vector<std::vector<float>>& chartData,
                   const std::vector<float>& xAxisData, 
                   double minXData, double maxXData) const;
```

**Логика отрисовки**:
- X координата точки определяется по температуре из `xAxisData[i]`
- Автоматическое определение диапазона температур
- Линейное масштабирование температуры на ширину графика

#### DrawGridAndAxes с температурой
```cpp
void DrawGridAndAxes(HDC hdc, const RECT& plotRect, 
                     const std::vector<std::vector<float>>& chartData,
                     const DataModule* dataModule, 
                     const std::vector<float>& xAxisData,
                     double minXData, double maxXData, 
                     double minYData, double maxYData) const;
```

**Особенности**:
- Подписи оси X в формате "%.1f°C"
- 10 вертикальных линий сетки с равномерным распределением температур
- Автоматическое определение min/max температуры для отображаемого диапазона

### Алгоритм отрисовки кроссплота

```cpp
// 1. Получение температурных данных
std::vector<float> temperatures;
dataModule->GetTemperatureData(temperatures, minX, maxX);

// 2. Определение диапазона температур
float minTemp = *std::min_element(temperatures.begin(), temperatures.end());
float maxTemp = *std::max_element(temperatures.begin(), temperatures.end());

// 3. Преобразование координат
for (size_t i = 0; i < data.size(); i++) {
    float temp = temperatures[i];
    int x = plotLeft + (temp - minTemp) / (maxTemp - minTemp) * plotWidth;
    int y = plotBottom - (data[i] - minY) / (maxY - minY) * plotHeight;
    // Отрисовка точки (x, y)
}
```

### Формат подписей оси X

**Frame Number режим**:
```
0    100    200    300    400    500
```

**Temperature режим**:
```
12.5°C  13.0°C  13.5°C  14.0°C  14.5°C  15.0°C
```

### Сохранение настроек

Тип оси X сохраняется в `ChartConfiguration::xAxisType` и применяется при нажатии "Apply" в Device Schematic.

```cpp
// Получение типа оси X из combobox
int selection = SendMessage(hXAxisCombo, CB_GETCURSEL, 0, 0);
config.xAxisType = (selection == 0) ? XAxisType::FRAME_NUMBER : XAxisType::TEMPERATURE;
```

### Примеры использования

#### Анализ температурной стабильности амплитуд
```
1. Выберите "All Ampl R1" пресет
2. Установите X-Axis: Temperature
3. Нажмите Apply
4. Анализируйте зависимость амплитуд от температуры
```

#### Температурный дрейф фаз
```
1. Выберите "All Phase R1" пресет
2. Установите X-Axis: Temperature
3. Нажмите Apply
4. Отслеживайте изменение фаз при изменении температуры
```

---

**Техническая документация AMK-Horizon-New-Chart v2.10.0** - Кроссплот и температурный анализ
