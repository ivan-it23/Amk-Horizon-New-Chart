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
- **Индексы 0-3**: TX1-TX4 @ 400 кГц (активные передатчики)
- **Индексы 4-7**: TX1-TX4 @ 2000 кГц (активные передатчики)
- **Индекс 8**: TX0 @ 400 кГц (молчащий передатчик)
- **Индекс 9**: TX0 @ 2000 кГц (молчащий передатчик)

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

## 📐 Отступы области графика (plotRect) (v2.12.2)

### Унифицированные отступы
Все методы, работающие с координатами графика, используют одинаковые отступы:

```cpp
// Константы отступов (относительно chartRect)
const int MARGIN_LEFT = 60;    // Отступ слева для левой оси Y
const int MARGIN_RIGHT = 140;  // Отступ справа для правой оси Y + scrollbar
const int MARGIN_TOP = 45;     // Отступ сверху для заголовка + легенды
const int MARGIN_BOTTOM = 50;  // Отступ снизу для оси X + scrollbar

// Вычисление plotRect
plotRect.left = chartRect.left + 60;
plotRect.top = chartRect.top + 45;
plotRect.right = chartRect.right - 140;
plotRect.bottom = chartRect.bottom - 50;
```

### Методы, использующие эти отступы
- `ChartModule::CalculateDualAxisPlotRect()` — основной метод вычисления plotRect
- `ChartManager::ConvertScreenToDataCoords()` — преобразование координат при клике
- `ChartManager::ConvertScreenToChartCoords()` — преобразование для статусной строки
- `ChartManager::DrawCrosshair()` — отрисовка перекрестия
- `ChartManager::DrawChartLegacy()` — отрисовка пустого графика

### Важно
Все методы должны использовать **одинаковые отступы**, иначе координаты при клике не будут совпадать с подписями на осях.

## 📊 Пропорциональный zoom для двойных осей (v2.12.2)

### Проблема
При zoom/scroll по оси Y изменяется диапазон левой оси, но правая ось имеет другие единицы измерения (мВ vs мдег) и другой диапазон значений.

### Решение
Вычисляется пропорция zoom от левой оси и применяется к правой:

```cpp
// Вычисляем полные диапазоны из данных
double leftMinFull, leftMaxFull;   // Полный диапазон левой оси
double rightMinFull, rightMaxFull; // Полный диапазон правой оси

// Вычисляем пропорцию zoom
double leftFullRange = leftMaxFull - leftMinFull;
double rightFullRange = rightMaxFull - rightMinFull;

double minRatio = (minYData - leftMinFull) / leftFullRange;
double maxRatio = (maxYData - leftMinFull) / leftFullRange;

// Применяем ту же пропорцию к правой оси
double minYRight = rightMinFull + minRatio * rightFullRange;
double maxYRight = rightMinFull + maxRatio * rightFullRange;
```

### Результат
- При zoom 50% левой оси, правая ось также показывает 50% своего диапазона
- При scroll обе оси смещаются пропорционально
- Визуально кривые на обеих осях масштабируются синхронно

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
    RGB(139, 69, 19),  // T2 - Коричневый
    RGB(255, 0, 0),    // T3 - Красный
    RGB(0, 0, 255)     // T4 - Синий
};
```

### Расширенная палитра (12 цветов)
```cpp
const COLORREF EXTENDED_PALETTE[12] = {
    RGB(0, 128, 0),      // Зелёный (T1)
    RGB(139, 69, 19),    // Коричневый (T2)
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

## 🔧 Zoom и скроллинг по оси Y (v2.12.0)

### Обзор
Версия 2.12.0 добавляет полную поддержку zoom и скроллинга по оси Y, что позволяет детально анализировать данные как по горизонтали, так и по вертикали.

### Структура ZoomRange
```cpp
struct ZoomRange {
    double minX, maxX, minY, maxY;
    bool isZoomed;
    
    ZoomRange() : minX(0.0), maxX(0.0), minY(0.0), maxY(0.0), isZoomed(false) {}
};
```

**Поля:**
- `minX`, `maxX` - диапазон по оси X (индексы кадров или температура)
- `minY`, `maxY` - диапазон по оси Y (значения данных)
- `isZoomed` - флаг активного zoom

### Новые методы UIModule

#### SetVScrollBarRange
```cpp
void SetVScrollBarRange(int chartNumber, int min, int max, int page, int pos);
```

**Назначение**: Настройка диапазона вертикального scrollbar

**Параметры**:
- `chartNumber` - номер графика (1 или 2)
- `min` - минимальное значение (обычно 0)
- `max` - максимальное значение (обычно 100)
- `page` - размер страницы (пропорционален видимому диапазону)
- `pos` - текущая позиция ползунка

**Особенность**: Вертикальный scrollbar инвертирован (верх = 0, низ = 100)

#### GetVScrollBar
```cpp
HWND GetVScrollBar(int chartNumber) const;
```

**Назначение**: Получение handle вертикального scrollbar

**Возвращает**: HWND вертикального scrollbar или nullptr

### Обновлённые методы ChartManager

#### HandleMouseUp
```cpp
void HandleMouseUp(HWND hWnd, int x, int y);
```

**Изменения**:
- Добавлена проверка минимального диапазона Y
- Константа `MIN_Y_RANGE = 10.0`
- Сообщение пользователю при достижении предела

**Логика проверки**:
```cpp
const double MIN_Y_RANGE = 10.0;
double yRange = maxDataY - minDataY;

if (yRange < MIN_Y_RANGE) {
    MessageBox(hWnd, 
        L"Cannot zoom further: Y-axis range must be at least 10 units.\nMinimum range is 10 units.",
        L"Zoom Limit Reached", 
        MB_OK | MB_ICONINFORMATION);
}
```

#### HandleScrollEvent (v2.12.3 - исправлено)
```cpp
void HandleScrollEvent(HWND hWnd, WPARAM wParam, LPARAM lParam);
```

**Изменения v2.12.3**:
- Исправлена формула скроллинга с учётом `nPage` в Windows API
- Реальный максимум позиции скроллбара = `nMax - nPage + 1`, а не `nMax`

**Алгоритм горизонтального скроллинга (исправленный)**:
```cpp
// Горизонтальный скроллинг (ось X)
double totalRange = static_cast<double>(dataModule->GetDataSize() - 1);
double viewRange = zoom->maxX - zoom->minX;

// Учитываем, что максимальная позиция скроллбара = nMax - nPage + 1
int maxScrollPos = si.nMax - (int)si.nPage + 1;
double scrollRatio = (maxScrollPos > 0) ? (double)newPos / (double)maxScrollPos : 0.0;
double newMinX = scrollRatio * (totalRange - viewRange);
zoom->minX = newMinX;
zoom->maxX = newMinX + viewRange;
```

**Алгоритм вертикального скроллинга (исправленный)**:
```cpp
if (isVertical) {
    // Получаем полный диапазон Y из данных
    double totalYRange = maxYFull - minYFull;
    double viewYRange = zoom->maxY - zoom->minY;
    
    // Вертикальный scrollbar инвертирован (верх = 0, низ = maxScrollPos)
    // Учитываем, что максимальная позиция скроллбара = nMax - nPage + 1
    int maxScrollPos = si.nMax - (int)si.nPage + 1;
    double scrollRatio = (maxScrollPos > 0) ? (double)(maxScrollPos - newPos) / (double)maxScrollPos : 0.0;
    double newMinY = minYFull + scrollRatio * (totalYRange - viewYRange);
    zoom->minY = newMinY;
    zoom->maxY = newMinY + viewYRange;
}
```

**Важно**: В Windows API при установке `SCROLLINFO` с `nPage`:
- `nMax = 100` (устанавливаем)
- `nPage = (viewRange / totalRange) * 100` (размер видимой области)
- Реальный максимум позиции = `100 - nPage + 1`

#### UpdateScrollBars
```cpp
void UpdateScrollBars(HWND hWnd);
```

**Изменения**:
- Добавлена настройка вертикального scrollbar
- Получение полного диапазона Y из данных
- Вычисление позиции с учётом инверсии

**Алгоритм настройки вертикального scrollbar**:
```cpp
// Получаем полный диапазон Y из данных
ChartConfiguration config = curveSelectionModule->GetChartConfiguration(chartNumber);
std::vector<std::vector<float>> leftAxisData, rightAxisData;
curveSelectionModule->CalculateCurveData(config, dataModule.get(), leftAxisData, rightAxisData, ...);

double minYFull, maxYFull, ...;
chartModule->GetDataRanges(leftAxisData, rightAxisData, ..., minYFull, maxYFull, ...);

double totalYRange = maxYFull - minYFull;
double viewRangeY = zoom->maxY - zoom->minY;

// Вертикальный scrollbar инвертирован (верх = 0, низ = 100)
int vScrollPos = static_cast<int>(100.0 - ((zoom->minY - minYFull) / (totalYRange - viewRangeY)) * 100.0);
uiModule->SetVScrollBarRange(chartNumber, 0, 100, static_cast<int>((viewRangeY / totalYRange) * 100), vScrollPos);
```

#### DrawChartWithCurves
```cpp
void DrawChartWithCurves(HDC hdc, const RECT& chartRect, int chartNumber);
```

**Изменения**:
- Передача параметров `zoom->minY` и `zoom->maxY` в методы вычисления данных
- Получение диапазонов Y из данных
- Передача Y параметров в методы отрисовки

**Пример**:
```cpp
if (zoom->isZoomed) {
    curveSelectionModule->CalculateCurveData(config, dataModule.get(),
                                           leftAxisData, rightAxisData,
                                           leftAxisCurves, rightAxisCurves,
                                           zoom->minX, zoom->maxX, zoom->minY, zoom->maxY);
}

// Получаем диапазоны Y из данных
double minYData, maxYData, ...;
chartModule->GetDataRanges(leftAxisData, rightAxisData, ..., minYData, maxYData, ...);

// Передаём в методы отрисовки
chartModule->DrawDualAxisChart(hdc, chartRect, config.curves,
                             leftAxisData, rightAxisData,
                             title, dataModule->HasData(),
                             zoom->isZoomed ? zoom->minX : 0.0,
                             zoom->isZoomed ? zoom->maxX : ...,
                             zoom->isZoomed ? zoom->minY : minYData,
                             zoom->isZoomed ? zoom->maxY : maxYData,
                             showAxes);
```

### Позиционирование вертикального scrollbar

**Константы**:
```cpp
const int SCROLLBAR_SIZE = 15;     // Размер scrollbar
const int MARGIN_RIGHT = 20;       // Отступ справа
const int MARGIN_TOP = 60;         // Отступ сверху
const int MARGIN_BOTTOM = 5;       // Отступ снизу
```

**Формула позиционирования**:
```cpp
SetWindowPos(hVScrollBar, HWND_TOP,
             chartRect.right - SCROLLBAR_SIZE - MARGIN_RIGHT,  // X позиция
             chartRect.top + MARGIN_TOP,                       // Y позиция
             SCROLLBAR_SIZE,                                   // Ширина
             chartRect.bottom - chartRect.top - MARGIN_TOP - MARGIN_BOTTOM - SCROLLBAR_SIZE,  // Высота
             SWP_NOZORDER);
```

### Двойные оси

**Поведение**: Zoom и scroll применяются к обеим осям Y одновременно

**Реализация**:
- Один вертикальный scrollbar управляет обеими осями
- Левая ось (фазы) и правая ось (амплитуды) масштабируются пропорционально
- Диапазон Y определяется по левой оси (основной)

### Инверсия вертикального scrollbar

**Причина**: В Windows координата Y растёт вниз, но для пользователя логично, что верх scrollbar = верх графика

**Формула преобразования**:
```cpp
// Позиция scrollbar → позиция данных
double newMinY = minYFull + ((100.0 - scrollPos) / 100.0) * (totalYRange - viewYRange);

// Позиция данных → позиция scrollbar
int scrollPos = static_cast<int>(100.0 - ((zoom->minY - minYFull) / (totalYRange - viewYRange)) * 100.0);
```

### Константы

**Минимальные диапазоны**:
```cpp
const double MIN_X_RANGE = 10.0;  // Минимальный диапазон по X
const double MIN_Y_RANGE = 10.0;  // Минимальный диапазон по Y
```

**ID элементов управления**:
```cpp
#define IDC_CHART1_VSCROLL  1041  // Вертикальный scrollbar для Chart 1
#define IDC_CHART2_VSCROLL  1042  // Вертикальный scrollbar для Chart 2
```

### Примеры использования

#### Zoom по обеим осям
```cpp
// Пользователь выделяет область мышью
HandleMouseDown(hWnd, x1, y1);
HandleMouseMove(hWnd, x2, y2);
HandleMouseUp(hWnd, x2, y2);

// Преобразование координат
ConvertScreenToDataCoords(chartNum, x1, y1, minDataX, minDataY);
ConvertScreenToDataCoords(chartNum, x2, y2, maxDataX, maxDataY);

// Проверка минимальных диапазонов
if (xRange >= MIN_X_RANGE && yRange >= MIN_Y_RANGE) {
    zoom->minX = minDataX;
    zoom->maxX = maxDataX;
    zoom->minY = minDataY;
    zoom->maxY = maxDataY;
    zoom->isZoomed = true;
}
```

#### Вертикальный скроллинг
```cpp
// Пользователь перемещает вертикальный scrollbar
HandleScrollEvent(hWnd, wParam, lParam);

// Определение типа scrollbar
if (hScrollBar == uiModule->GetVScrollBar(chartNumber)) {
    isVertical = true;
}

// Обновление диапазона Y
if (isVertical) {
    double newMinY = minYFull + ((100.0 - newPos) / 100.0) * (totalYRange - viewYRange);
    zoom->minY = newMinY;
    zoom->maxY = newMinY + viewYRange;
}
```

---

**Техническая документация AMK-Horizon-New-Chart v2.12.0** - справочник для разработчиков


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

## 🔍 Zoom для кроссплота (v2.10.1)

### Обзор
Версия 2.10.1 добавляет полную поддержку zoom (масштабирования) для режима кроссплота, где ось X представляет температуру.

### Проблема
В режиме кроссплота экранные координаты должны преобразовываться не в индексы кадров напрямую, а через температуру:
- Экранная X координата → Температура → Поиск ближайшего индекса кадра с этой температурой

### Решение: ConvertScreenToDataCoords

Метод `ConvertScreenToDataCoords` в `ChartManager.cpp` обновлён для поддержки температурного режима:

```cpp
void ChartManager::ConvertScreenToDataCoords(int chartNum, int screenX, int screenY, 
                                             double& dataX, double& dataY) const {
    // 1. Получение конфигурации графика
    ChartConfiguration config = curveSelectionModule->GetChartConfiguration(chartNum);
    
    // 2. Вычисление plotRect с учётом отступов
    RECT plotRect;
    if (config.hasDualAxis) {
        plotRect.left = chartRect->left + 80;
        plotRect.top = chartRect->top + 65;
        plotRect.right = chartRect->right - 120;
        plotRect.bottom = chartRect->bottom - 50;
    } else {
        plotRect.left = chartRect->left + 80;
        plotRect.top = chartRect->top + 65;
        plotRect.right = chartRect->right - 30;
        plotRect.bottom = chartRect->bottom - 50;
    }
    
    // 3. Вычисление относительных координат
    double xRatio = (screenX - plotRect.left) / plotWidth;
    double yRatio = (plotRect.bottom - screenY) / plotHeight;
    
    // 4. Специальная логика для температурного режима
    if (config.xAxisType == XAxisType::TEMPERATURE) {
        // Получаем температурные данные
        std::vector<float> temperatureData;
        dataModule->GetTemperatureData(temperatureData, zoom->minX, zoom->maxX);
        
        // Находим диапазон температур
        float minTemp = *std::min_element(temperatureData.begin(), temperatureData.end());
        float maxTemp = *std::max_element(temperatureData.begin(), temperatureData.end());
        
        // Температура, на которую кликнули
        float clickedTemp = minTemp + static_cast<float>(xRatio) * (maxTemp - minTemp);
        
        // Находим ближайший индекс кадра с этой температурой
        size_t closestIdx = startIdx;
        float minDiff = FLT_MAX;
        for (size_t i = startIdx; i < endIdx; i++) {
            float diff = fabs(temperatureData[i] - clickedTemp);
            if (diff < minDiff) {
                minDiff = diff;
                closestIdx = i;
            }
        }
        
        // Возвращаем индекс кадра (не температуру!)
        dataX = static_cast<double>(closestIdx);
    }
}
```

### Ключевые особенности

1. **Вычисление plotRect**: Учитываются отступы для одной и двойных осей (80px слева, 65px сверху, 30/120px справа, 50px снизу)

2. **Проверка типа оси X**: `config.xAxisType == XAxisType::TEMPERATURE`

3. **Преобразование координат**:
   - Экранная X → относительная позиция (xRatio)
   - xRatio → температура (clickedTemp)
   - Температура → поиск ближайшего индекса кадра
   - Возврат индекса кадра (не температуры!)

4. **Zoom работает с индексами**: `zoom->minX` и `zoom->maxX` всегда хранят индексы кадров, даже в температурном режиме

### Алгоритм zoom в кроссплоте

```
1. Пользователь выделяет область мышью
   ↓
2. HandleMouseUp вызывает ConvertScreenToDataCoords
   ↓
3. Преобразование: экранные координаты → температура → индекс кадра
   ↓
4. Сохранение zoom->minX и zoom->maxX (индексы кадров)
   ↓
5. Перерисовка графика с новым диапазоном
   ↓
6. DrawChartWithCurves получает температуры для zoom->minX..zoom->maxX
   ↓
7. Отрисовка увеличенного участка по температуре
```

### Исправления подписей оси X (v2.10.1)

**Проблема**: Подпись "°C" отображалась как "В°C" и была по центру

**Решение**:
```cpp
// Было:
const wchar_t* xAxisLabel = L"°C";
int xLabelX = plotRect.left + (plotRect.right - plotRect.left - xLabelSize.cx) / 2;
int xLabelY = plotRect.bottom + 5;

// Стало:
const wchar_t* xAxisLabel = L"\u00B0C"; // Unicode для символа градуса
int xLabelX = plotRect.right - xLabelSize.cx - 5; // Справа с отступом 5px
int xLabelY = plotRect.bottom + 25; // Ниже числовых значений
```

**Изменения**:
- Использован Unicode escape-код `\u00B0` для символа градуса
- Подпись перемещена в правый угол: `plotRect.right - xLabelSize.cx - 5`
- Увеличен отступ по вертикали: 25px вместо 5px (избегает наслоения на числа)

### Тестирование zoom

1. Откройте Device Schematic
2. Выберите "Temperature" в X-Axis
3. Нажмите Apply
4. Выделите область на графике мышью (левая кнопка + перетаскивание)
5. График должен корректно увеличить выбранный участок
6. Правая кнопка мыши сбрасывает zoom

---

**Техническая документация AMK-Horizon-New-Chart v2.10.1** - Кроссплот с полной поддержкой zoom


---

## 👨‍💻 ДЛЯ РАЗРАБОТЧИКОВ

### 🛠️ Настройка среды разработки

**Требования:**
- Visual Studio 2019/2022 с поддержкой C++17
- Windows SDK 10.0 или новее
- Git для контроля версий

**Структура проекта:**
```
Amk-Horizon-New-Chart/
├── Amk-Horizon-New-Chart.cpp    # Точка входа
├── ChartManager.cpp/h            # Главный координатор
├── DataModule.cpp/h              # Модуль данных
├── ChartModule.cpp/h             # Модуль отрисовки
├── UIModule.cpp/h                # Модуль интерфейса
├── LayoutModule.cpp/h            # Модуль компоновки
├── SchematicModule.cpp/h         # Модуль схемы прибора
└── CurveSelectionModule.cpp/h    # Модуль управления кривыми
```

### 🏗️ Архитектурные принципы

**Модульность:**
- Разделение ответственности - каждый модуль отвечает за конкретную область
- Слабая связанность - модули взаимодействуют через четко определенные интерфейсы
- Высокая сплоченность - функции внутри модуля тесно связаны по назначению

**Расширяемость:**
- Открыт для расширения - легко добавлять новые типы данных и графиков
- Закрыт для модификации - существующий код не требует изменений

### 📋 Стандарты кодирования

**Именование:**
```cpp
class DataModule { };              // Классы - PascalCase
void CalculateChartData();         // Методы - PascalCase
int chartNumber;                   // Переменные - camelCase
const int MAX_CHANNELS = 10;       // Константы - UPPER_CASE
```

**Комментарии:**
```cpp
/**
 * @brief Вычисляет данные для отображения на графике
 * @param type Тип данных для вычисления
 * @param freq Частотный диапазон
 * @param outData Выходной массив данных
 * @return true если вычисление успешно
 */
bool CalculateChartData(DataType type, FrequencyBand freq, 
                       std::vector<std::vector<float>>& outData);
```

### 🚀 Сборка релиза

1. Переключитесь на конфигурацию **Release x64**
2. Очистите решение (Build → Clean Solution)
3. Пересоберите решение (Build → Rebuild Solution)
4. Проверьте отсутствие предупреждений

---

---

**AMK-Horizon-New-Chart v2.12.1** - Техническая документация для разработчиков 🚀
