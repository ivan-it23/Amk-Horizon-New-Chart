# Техническая справка AMK-Horizon-New-Chart

> Статус: актуально для текущей ветки `main`.

## Назначение документа

Этот файл — **компактная техническая справка** по текущим инвариантам приложения:

- структурам данных
- преобразованиям значений
- правилам геометрии/осей/координат
- зонам ответственности модулей

Детали, вынесенные в отдельные документы:

- формат файлов: `..\FILE_FORMATS_ANALYSIS.md`
- файловая валидация: `..\FILE_VALIDATION_FOR_5TX_MODEL.md`
- точка истины по структурам: `ANALYSIS_GP_TOOL_DATA.md`
- краткий статус ветки: `..\PROJECT_STATUS.md`

## Текущее архитектурное состояние

### Актуальный pipeline данных

Текущий основной pipeline проекта:

`File/COM -> ToolDataAdapterRegistry -> ParsedToolFrame -> canonical GP_TOOL_DATA + ToolRecordMetadata -> ToolDataAccessView / MeasurementView -> chart/UI logic`

Практически это означает:

- файл и COM-поток уже парсятся через `ToolDataAdapterRegistry`
- live ingest в актуальном in-tree пути идёт как `SerialModule::SetParsedDataCallback(...) -> ChartManager -> DataModule::AddParsedData(...)`
- `DataModule` хранит canonical `GP_TOOL_DATA`, но дополнительно держит `ToolRecordMetadata` для capability-aware доступа
- верхние calculation/UI path-ы постепенно переводятся на `ToolDataAccessView` и `MeasurementView`

### Важные compatibility boundaries

На текущем этапе Phase 7 остаются важны следующие границы:

- `DataModule` по-прежнему хранит данные в canonical `GP_TOOL_DATA`
- `SaveDataRange(...)` остаётся основным live persistence/export boundary для canonical `.bin`
- публичные `DataModule::GetData()`, `DataModule::ParseRawFrame(...)`, `SerialModule::RequestData(GP_TOOL_DATA& ...)` и `SerialModule::SetDataCallback(...)` следует трактовать как compatibility-only API surface-ы
- предпочтительные внутренние path-ы теперь проходят через `ParseRawFrameDetailed(...)`, `RequestData(ParsedToolFrame& ...)`, `SetParsedDataCallback(...)`, `TryGetRecord(...)`, `TryGetRecordMetadata(...)`, `GetDataView(...)` и `GetMeasurementView(...)`


## Структуры данных

### XAxisType
```cpp
enum class XAxisType {
    FRAME_NUMBER,   // Номер кадра (по умолчанию)
    TEMPERATURE     // Температура
};
```

### GP_TOOL_DATA
```cpp
struct GP_TOOL_DATA {
    uint32_t signature;
    uint32_t condition;
    uint32_t frame;
    float temperature;
    float rho_smt[2][5];
    float phase_smt[2][5];
    float AM_RX_1[2][5];
    float ZERO_AM_RX_1[2];
    float AM_RX_2[2][5];
    float ZERO_AM_RX_2[2];
    float DELTA_PH[2][5];
    float ZERO_dPH[2];
};
```

### Индексация 5Tx
- **Частоты**: `freqIdx = 0 → 400 kHz`, `freqIdx = 1 → 2000 kHz`
- **Передатчики**: `txIdx = 0..4 → T1..T5`
- **Молчащий режим**: отдельные поля `ZERO_AM_RX_*[2]`, `ZERO_dPH[2]` (не “T0 в массиве”)

### DataType
```cpp
enum class DataType {
    PHASE_DIFFERENCE,        // Разность фаз (источник: DELTA_PH) (5 каналов T1-T5)
    AMPLITUDE_RATIO,         // Отношение амплитуд RX2/RX1 (5 каналов T1-T5)
    AMPLITUDE_RX1,           // Амплитуды RX1 активных передатчиков (5 каналов)
    AMPLITUDE_RX2,           // Амплитуды RX2 активных передатчиков (5 каналов)
    PHASE_SMT,               // Симметризованные фазы (5 каналов)
    RHO_SMT,                 // Симметризованные УЭС (5 каналов)
    SILENT_AMPLITUDE_RX1,    // Амплитуды молчащих передатчиков RX1 (1 канал)
    SILENT_AMPLITUDE_RX2,    // Амплитуды молчащих передатчиков RX2 (1 канал)
    SILENT_PHASE_DIFFERENCE, // Разность фаз молчащих передатчиков R2−R1 (источник: ZERO_dPH) (1 канал)
    TEMPERATURE              // Температура прибора (1 значение на запись)
};
```

### METROLOGY_DATA (240 байт)
Источник истины: `SerialModule.h` (`static_assert(sizeof(METROLOGY_DATA) == 240)`).

Ключевые поля, используемые приложением:
- `signature`, `serial`
- `L1[5]`, `L2[5]`, `F[2]`
- `Air_zz[10]`, `Air_zz_amt[10]`
- `D_sonde_mm`

Применение:
- форма метрологии (чтение/редактирование/запись)
- чтение/запись в прибор по COM
- именование файлов записи потока (`serial` + `D_sonde_mm`)

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

## Формулы преобразования данных

### Нормализация фаз
```cpp
float NormalizePhase(float phaseRadians) {
    const float PI = 3.14159265358979323846f;
    while (phaseRadians > PI) phaseRadians -= 2.0f * PI;
    while (phaseRadians <= -PI) phaseRadians += 2.0f * PI;
    return phaseRadians;
}
// Приводит фазу к диапазону (-π, π]
```

### Преобразование амплитуд
```cpp
float ConvertAmplitudeToMillivolts(float rawValue) {
    return rawValue * 4000.0f / pow(2.0f, 24.0f);
}
// Сырые данные АЦП → милливольты (mV)
```

### Преобразование фаз
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

## Отступы области графика (`plotRect`)

### Унифицированные отступы
Все методы, работающие с координатами графика, используют одинаковые отступы:

```cpp
// Константы отступов (относительно chartRect)
const int MARGIN_LEFT = 100;   // Отступ слева для левой оси Y (увеличено, чтобы подписи не обрезались)
const int MARGIN_RIGHT = 140;  // Отступ справа для правой оси Y + scrollbar
const int MARGIN_TOP = 45;     // Отступ сверху для заголовка + легенды
const int MARGIN_BOTTOM = 50;  // Отступ снизу для оси X + scrollbar

// Вычисление plotRect
plotRect.left = chartRect.left + 100;
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

## Производительность на больших данных

### Актуальные инварианты производительности

- тяжёлые операции подготовки данных не должны блокировать UI-поток
- повторная отрисовка не должна каждый раз пересобирать одни и те же большие массивы
- derived preset-ы должны по возможности использовать кешированные ряды, а не пересчёт на каждый paint/pass
- coordinate mapping, zoom и draw должны использовать один и тот же диапазон кадров

### Реализованные механизмы

- `AsyncTaskQueue` используется для фоновой загрузки и прогрева отображаемых snapshot-ов
- `ChartManager` хранит кеш snapshot-ов кривых и temperature-axis данных
- `DataModule` хранит lazy cache derived series для `DataType::PHASE_DIFFERENCE` и `DataType::AMPLITUDE_RATIO`
- `ChartModule` использует LOD/downsampling и screen-space cache полилиний для temperature mode
- в тяжёлых путях убраны лишние `UpdateWindow()` и снижена частота обновления `WM_MOUSEMOVE`

### Zoom и coordinate mapping

Для корректной работы на больших `.DEV` файлах действуют следующие правила:

- `zoom.minX` / `zoom.maxX` интерпретируются как диапазон индексов кадров
- snapshot, draw path и screen/data conversion должны опираться на один и тот же видимый диапазон кадров
- при повторном zoom нельзя повторно смешивать fractional X-границы с integer-индексацией кадров
- если ось X = `TEMPERATURE`, отображение остаётся temperature-based, но внутренняя навигация и сохранение ссылок по X всё равно идут через индекс кадра

### Заметки и перекрестие

Инварианты создания заметок:

- источник истины для новой заметки — текущая позиция перекрестия на экране (`screenX/screenY`)
- координаты заметки вычисляются через тот же `ConvertScreenToChartCoords(...)`, что и статусная строка
- для обычных пресетов допускается snap-to-curve
- для `PresetType::PHASE_DIFFERENCES` и `PresetType::SILENT_PHASE_DIFFERENCE` snap-to-curve отключён, потому что он может уводить заметку от фактической точки пользователя

## Пропорциональный zoom для двойных осей

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

## Расположение осей Y

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

## Статусная строка

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

## Основные модули

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

// Управление статусной строкой
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

// Получение диапазонов данных
void GetDataRanges(const std::vector<std::vector<float>>& leftAxisData,
                  const std::vector<std::vector<float>>& rightAxisData,
                  double& minX, double& maxX,
                  double& minY1, double& maxY1,
                  double& minY2, double& maxY2) const;
```

### ChartManager
```cpp
// Обработка событий мыши
void HandleMouseMoveForCoordinates(HWND hWnd, int x, int y);

// Преобразование координат
bool ConvertScreenToChartCoords(int chartNum, int screenX, int screenY,
                                double& dataX, double& dataY1, double& dataY2,
                                std::wstring& y1Unit, std::wstring& y2Unit,
                                bool& hasDualAxis) const;

// Получение единиц измерения
std::wstring GetUnitForDataType(DataType dataType) const;

// Обновление информации о файле
void UpdateStatusBarFileInfo();
```

### SchematicModule
```cpp
// Диалог схемы прибора
void ShowSchematicDialog(HWND parent, int chartNumber);

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

## UI и визуальные инварианты

### Цвета каналов T1-T5

Каналы T1-T5 имеют стабильное цветовое кодирование. Для актуальных значений нужно ориентироваться на константы в коде (`ChartModule.cpp`, `CurveSelectionModule.cpp`, `SchematicModule.cpp`), а не на исторические changelog-записи.

### Правила выбора элементов на схеме

Ключевые инварианты выбора:

- должен быть выбран хотя бы один источник данных
- нельзя одновременно выбирать передатчики и приёмники как два независимых сценария
- комбинация `R1 + R2 + Silent` невалидна

Практический эффект:

- `R1 + R2` даёт отношения амплитуд и разности фаз
- одиночный `R1` или `R2` даёт амплитуды и фазы выбранного приёмника
- `Silent` даёт silent-амплитуды и `ZERO_dPH`-кривые

---

## Zoom, scroll и ось X

### Инварианты zoom

- `zoom->minX` и `zoom->maxX` всегда хранят **индексы кадров**
- диапазон по Y влияет на отображение, а не на фильтрацию исходных данных по Y
- минимальный диапазон zoom:
  - `MIN_X_RANGE = 10`
  - `MIN_Y_RANGE = 10`

### Вертикальный scrollbar

- один вертикальный scrollbar управляет отображаемым диапазоном Y
- для dual-axis графика правая ось масштабируется **пропорционально** левой
- вертикальный scrollbar инвертирован: визуальный верх соответствует верхней части графика

### XAxisType

Поддерживаются два режима оси X:

- `FRAME_NUMBER`
- `TEMPERATURE`

Важно:

- даже в режиме `TEMPERATURE` внутренний zoom и навигация работают через **индексы кадров**
- температура используется как способ позиционирования точки по X при отрисовке и coordinate mapping

### Surf и X Range

При активном Surf поля `X Range` не должны автоматически перезаписывать пользовательский ввод. Это поведение считается инвариантом UI.

---

## Файлы, протокол и метрология

В этом документе эти темы подробно не дублируются.

Используйте профильные документы:

- файловые контейнеры `.bin/.biv/.DEV` → `..\FILE_FORMATS_ANALYSIS.md`
- файловая валидация → `..\FILE_VALIDATION_FOR_5TX_MODEL.md`
- измерительные и метрологические структуры → `ANALYSIS_GP_TOOL_DATA.md`
- текущие ограничения и статус ветки → `..\PROJECT_STATUS.md`

Коротко:

- файловая модель приложения — adapter-driven parsing с canonical materialization в `GP_TOOL_DATA` (`240` байт)
- метрология — `METROLOGY_DATA` (`240` байт)
- COM-протокол использует пакет `[CMD][LEN][PAYLOAD][CRC16]`
- текущие команды приложения: `0x11`, `0x12`, `0x13`, `0x16`, `0x17`

---

## Практические точки входа для разработки

Если нужно менять поведение приложения, обычно смотреть сюда:

- `DataModule.h/.cpp` — структура измерений, загрузка файла, валидация
- `CurveSelectionModule.cpp` — вычисление кривых, режимы `FROM_SONDE` / `FROM_COMPUTER`
- `ChartModule.cpp` — геометрия графика, dual-axis отрисовка, plotRect
- `ChartManager.cpp` — zoom, scroll, coordinate mapping, UI orchestration
- `SerialModule.h/.cpp` — COM-протокол, запись потока, CRC, метрология
- `MetrologyModule.cpp` — декодирование сигнатуры и работа с метрологией

---

## Что не является целью этого документа

Этот файл больше **не** хранит:

- подробную историю версий по функциям
- длинные tutorial-примеры использования UI
- пошаговые release-notes по отдельным фичам
- инструкции по сборке релиза

Для этого используются:

- `README.md`
- `..\PROJECT_STATUS.md`
- `..\ADAPTER_REGISTRY_IMPLEMENTATION_PLAN.md`

---

Эта техническая справка должна оставаться коротким документом про **текущие инварианты системы**, а не архивом исторических изменений.
