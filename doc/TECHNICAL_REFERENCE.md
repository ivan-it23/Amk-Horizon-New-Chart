# Техническая справка AMK-Horizon-New-Chart

> Актуально для текущей ветки `main`.

## Назначение документа

Этот файл фиксирует **текущие технические инварианты** проекта:

- архитектурные границы;
- текущий pipeline данных;
- инварианты графиков, UI и координат;
- runtime-особенности `4Tx/5Tx`;
- важные ограничения, которые должны учитываться при дальнейших изменениях.

Детали приведены в профильных документах:

- структуры данных и canonical mapping — `ANALYSIS_GP_TOOL_DATA.md`
- файловые контейнеры — `FILE_FORMATS_ANALYSIS.md`
- файловая валидация — `FILE_VALIDATION_FOR_5TX_MODEL.md`
- краткий статус ветки — `PROJECT_STATUS.md`

## 1. Архитектура

### 1.1 Координатор

Главный orchestration-класс — `ChartManager`.

Он связывает:

- `DataModule`
- `ChartModule`
- `UIModule`
- `LayoutModule`
- `SchematicModule`
- `CurveSelectionModule`
- `SerialModule`
- `MetrologyModule`
- `NotesModule`
- `AsyncTaskQueue`
- `DeviceExchangeLog` + `DeviceLogPanel` (см. §8.4)

### 1.2 Текущий pipeline данных

Актуальный pipeline:

`File / COM -> ToolDataAdapterRegistry -> ParsedToolFrame -> canonical GP_TOOL_DATA + ToolRecordMetadata -> ToolDataAccessView / MeasurementView -> chart / UI logic`

### 1.3 Что это означает на практике

- `ToolDataAdapterRegistry` — точка маршрутизации по сигнатуре и формату;
- `ParsedToolFrame` — adapter-level результат парсинга;
- `GP_TOOL_DATA` — текущая canonical compatibility shape;
- `ToolRecordMetadata` — capability-aware контекст рядом с записью;
- `ToolDataAccessView` и `MeasurementView` — предпочтительный access layer для верхних модулей.

## 2. Adapter-архитектура

### 2.1 UniversalCanonicalAdapter

Единственный адаптер, обрабатывающий все типы приборов:

- принимает любую сигнатуру с `toolCode > 0`;
- определяет `DeviceType`, `transmitterCount`, `modification` из palette (toolCode = XYZ);
- canonical passthrough: `memcpy` без преобразований;
- статические методы: `DetermineDeviceType()`, `DetermineTransmitterCount()`, `BuildCapabilities()`, `BuildDisplayName()`.

### 2.2 Что остаётся canonical boundary

- `DataModule` хранит данные как `GP_TOOL_DATA`;
- `SaveDataRange(...)` пишет canonical `.bin`;
- range export UI строится вокруг canonical `.bin` сценария;
- конструктор файлов и добавление отрезков работают с уже разобранными `GP_TOOL_DATA`, а не с сырым склеиванием байтов.

### 2.3 Предпочтительные API

- `ParseRawFrameDetailed(...)`
- `RequestData(ParsedToolFrame& ...)`
- `SetParsedDataCallback(...)`
- `TryGetRecord(...)`
- `TryGetRecordMetadata(...)`
- `GetDataView(...)`
- `GetMeasurementView(...)`

## 3. Ключевые структуры и типы

### 3.1 Измерительные данные

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

Размер:

- `GP_TOOL_DATA == 240 байт`

### 3.2 Метрология

```cpp
struct METROLOGY_DATA {
    uint32_t signature;
    uint32_t serial;
    uint16_t L1[5];
    uint16_t L2[5];
    uint16_t F[2];
    int16_t Air_zz[10];
    int16_t Air_zz_amt[10];
    uint32_t D_sonde_mm;
    uint8_t service[164];
};
```

Размер:

- `METROLOGY_DATA == 240 байт`

### 3.3 Capabilities и range analysis

Текущие важные типы:

- `ToolDataKind` (`UNKNOWN`, `AUTONOM`, `LWD` — определяется из palette)
- `ToolCapabilities`
- `ToolRecordMetadata`
- `DataRangeMetadataSummary`
- `DataRangeExportAnalysis`

Они используются для:

- runtime-определения числа передатчиков;
- capability-aware доступа к данным;
- анализа диапазона перед экспортом;
- разделения current-state и compatibility behavior.

## 4. Инварианты 3Tx/4Tx/5Tx

### 4.1 Runtime число передатчиков

Источником истины является не UI-константа, а capability metadata:

- `ToolCapabilities.transmitterCount`
- `ToolDataAccessView::GetTransmitterCount()`

### 4.2 Что остаётся фиксированным в UI

UI исторически использует 5 визуальных слотов передатчиков:

- `T1`
- `T2`
- `T3`
- `T4`
- `T5`

Но это **не означает**, что у текущих данных всегда реально 5 активных TX.

### 4.3 Актуальное поведение схемы прибора

`SchematicModule` синхронизирован с runtime capability count (определяется из palette):

- недоступные передатчики отображаются как заблокированные;
- список доступных кривых и checkbox-ы скрываются динамически по capability.

### 4.4 Практический смысл

При изменении логики выбора кривых или схемы прибора:

- нельзя опираться только на `UI_TRANSMITTER_COUNT = 5`;
- должны учитываться текущие `ToolCapabilities`;
- UI layout можно оставлять 5-слотовым, но runtime доступность должна быть capability-aware.

## 5. Кривые, пресеты и SMT

### 5.1 Основная сборка кривых

`CurveSelectionModule` отвечает за:

- генерацию списка доступных кривых;
- восстановление/применение конфигурации;
- пресеты;
- назначение цветов;
- вычисление рядов для графиков.

### 5.2 Доступные основные типы данных

- `PHASE_DIFFERENCE`
- `AMPLITUDE_RATIO`
- `AMPLITUDE_RX1`
- `AMPLITUDE_RX2`
- `PHASE_SMT`
- `RHO_SMT`
- `SILENT_AMPLITUDE_RX1`
- `SILENT_AMPLITUDE_RX2`
- `SILENT_PHASE_DIFFERENCE`
- `TEMPERATURE`

### 5.3 `FROM_SONDE` / `FROM_COMPUTER`

Для `PHASE_SMT` и `RHO_SMT` есть два режима:

- `FROM_SONDE` — брать готовые значения из данных;
- `FROM_COMPUTER` — использовать legacy-compatible computer-side ветку.

Текущая computer-side ветка:

- использует `DELTA_PH` только по первым 4 входам `T1..T4`;
- строит до 5 выходов по legacy коэффициентам `simmetry_5`;
- затем при необходимости инвертирует фазу в `rho`;
- активируется только при наличии метрологии и совпадении `signature`.

Это важно:

- это **не** общий adapter-aware SMT solver;
- это **legacy-compatible вычислительная граница**, сохранённая ради текущего поведения приложения.

### 5.4 Цветовые инварианты

Цвета кривых централизованы через `RefreshCurveColors(...)`.

Обязательные инварианты:

- `T1` — зелёный;
- `T2` — жёлтый;
- `T3` — красный;
- `T4` — синий;
- палитра приглушённая, без кислотной яркости;
- распределение цветов идёт на весь набор кривых, а не по месту.

## 6. Графики, координаты и заметки

### 6.1 Ось X

Поддерживаются два режима:

- `FRAME_NUMBER`
- `TEMPERATURE`

Но внутренние zoom и навигация по-прежнему опираются на **индексы записей**.

### 6.2 Источники данных графиков

График 1 является основным потребителем текущего источника данных приложения. Источником может быть открытый файл, файл записи COM или файл, выбранный для просмотра при продолжающейся фоновой записи COM.

График 2 поддерживает два режима источника данных:

- `Linked` — график 2 использует тот же `DataModule`, что и график 1;
- `Independent file` — график 2 использует отдельный `DataModule`, загружаемый через собственную кнопку открытия файла.

Обязательные инварианты:

- изменение режима графика 2 не должно запускать, останавливать или перенастраивать COM-опрос графика 1;
- при переходе графика 2 в независимый режим отображается иконка открытия файла для графика 2;
- при возврате графика 2 в связанный режим иконка открытия файла скрывается, а источник данных графика 2 снова совпадает с источником графика 1;
- кэши кривых, температурной оси и асинхронного прогрева должны учитывать фактический источник данных графика, а не только `dataVersion`;
- кроссплот с `TEMPERATURE` по оси X должен использовать температурный ряд того же `DataModule`, из которого строятся Y-ряды данного графика;
- границы сегментов собранного файла читаются из sidecar-файла текущего источника данных графика; для независимого графика 2 используется его собственный путь файла.

### 6.3 X Range

Текущие инварианты:

- поля `X Range` не должны авто-перезаписывать пользовательский ввод до явного применения;
- можно задать только `From`, оставив `To` пустым;
- это означает режим `From .. latest frame`;
- в `Surf` то же правило сохраняется.

### 6.4 Пустое состояние графика

Если данные не загружены, график должен отображать стандартную область построения с осями, сеткой и служебным сообщением. Пустое состояние не должно отрисовываться только как две оси без сетки.

Это правило применяется к графику 1, графику 2, двойному режиму и независимому режиму графика 2.

### 6.5 Заметки

`NotesModule` хранит:

- `configKey`
- `configSnapshot`
- `presetName`
- `xAxisType`
- `xIndex`
- `yLeft`
- `yRight`
- `anchorAxisGroup`
- `text`

Это означает:

- заметки привязаны к конфигурации и индексу записи;
- даже если ось X визуально температурная, привязка хранится по индексу кадра;
- hit-test идёт по render-time marker positions.

### 6.6 Координатная согласованность

При изменениях в `ChartManager` и `ChartModule` должны оставаться согласованными:

- render geometry;
- screen-to-data conversion;
- crosshair;
- note markers;
- dual-axis draw range.

Именно здесь возникают самые опасные визуальные расхождения.

Координаты курсора, поля координат красного перекрестия и статусная строка должны использовать те же диапазоны осей, отступы и источник данных, что и фактический draw path. Это требование распространяется на `Surf`, двойные оси, обычную ось X по номеру кадра и кроссплот с температурной осью X.

Разделители сегментов собранных файлов используют ту же геометрию. Для обычной оси X линия ставится по индексу первого кадра следующего сегмента. Для `TEMPERATURE` по оси X линия ставится между экранными X последнего кадра предыдущего сегмента и первого кадра следующего сегмента, чтобы начало нового эксперимента с минимальной температурой не попадало визуально на ось Y.

### 6.7 UI-инварианты кнопок подключения и опроса

Текущие инварианты элементов управления COM-портом:

- **Кнопка "Включить/Отключить"** (`IDC_CONNECT_TOGGLE_BTN`):
  - зелёная (RGB 50, 160, 50) когда прибор отключён, текст "Включить";
  - красная (RGB 210, 60, 60) когда прибор подключён, текст "Отключить";
  - отключена (серая), если нет сохранённого COM-порта и нет активного подключения;
  - использует сохранённые настройки `lastComPort` из `settings.txt`;
  - при клике выполняет быстрое подключение через `ChartManager::HandleSerialConnectQuick()`;
  - при активном подключении — выполняет отключение через `ChartManager::HandleSerialDisconnect()`.

- **Кнопка опроса ▶/■** (`IDC_POLLING_TOGGLE_BTN`):
  - отображает только иконку без текста (▶ для Старт, ■ для Стоп);
  - зелёная в состоянии "готов к старту", красная в состоянии "опрос активен";
  - активна только когда есть подключение к прибору;
  - при клике переключает состояние опроса через `ChartManager::HandleSerialStart/Stop()`.

- **Кнопка "К прибору"**:
  - отображается при просмотре стороннего файла, если существует активный или приостановленный файл записи COM;
  - возвращает график 1 и связанный график 2 к данным, полученным с прибора;
  - не должна требовать активного опроса, если прибор подключён, а запись была поставлена на паузу;
  - не должна создавать новый файл записи вместо отключения прибора.

- **Меню "Прибор"**:
  - пункт "Настройки подключения..." (`IDM_DEVICE_CONNECT`) открывает диалог расширенной конфигурации;
  - в диалоге можно выбрать COM-порт, скорость и папку для записи данных.

- **Сохранение настроек**:
  - `lastComPort` — последний использованный COM-порт;
  - `lastRecordFolder` — последняя папка для записи;
  - сохраняются через `SaveAppSettingValue()` / `LoadAppSettingValue()` в `settings.txt`.

## 7. Производительность

### 7.1 Что уже реализовано

- фоновая подготовка тяжёлых операций через `AsyncTaskQueue`;
- snapshot cache в `ChartManager`;
- lazy cache производных рядов в `DataModule`;
- screen-space / render cache части temperature path в `ChartModule`;
- LOD/downsampling;
- throttling `WM_MOUSEMOVE`;
- корректная обработка `NaN`.

### 7.2 Что важно не ломать

- подготовка тяжёлых данных не должна блокировать UI;
- coordinate mapping должен использовать те же диапазоны, что и draw path;
- derived series не должны лишний раз пересчитываться на каждый paint;
- фоновая логика не должна рассинхронизировать пользовательский X Range.
- кэши графиков не должны переиспользоваться между разными `DataModule`, даже если у модулей совпадает `dataVersion`.

## 8. COM, файлы и экспорт

### 8.1 COM-протокол

Текущий формат пакета:

`[CMD:1][LEN:2][PAYLOAD][CRC16:2]`

Рабочие команды:

- `0x11`
- `0x12`
- `0x13`
- `0x16`
- `0x17`

### 8.2 Файлы

Поддерживаются:

- `.bin`
- `.biv`
- `.DEV`

Подробности:

- `FILE_FORMATS_ANALYSIS.md`
- `FILE_VALIDATION_FOR_5TX_MODEL.md`

### 8.3 Export

На текущем этапе:

- export диапазона остаётся canonical `.bin`;
- `TryDescribeRangeMetadata(...)` и `TryAnalyzeExportRange(...)` уже существуют;
- mixed range сейчас может быть сохранён только как canonical compatibility export;
- adapter-aware export policy пока не внедрена.

### 8.4 Конструктор файлов

`FileConstructorModule` отвечает за сборку и переупорядочивание файлов данных:

- входные данные загружаются через `DataModule` и затем сохраняются как canonical `GP_TOOL_DATA` записи;
- совместимость проверяется по полной `signature`; разные типы приборов, число передатчиков или модификации не смешиваются;
- метаданные сегментов сохраняются в `%LOCALAPPDATA%\AMK-Horizon-New-Chart\segments\<data-file>.<hash>.segments.tsv`; при загрузке приложение автоматически мигрирует старые sidecar-файлы, ранее созданные рядом с данными;
- существующая сборка может быть открыта повторно, если исходные файлы сегментов доступны по путям из sidecar; после изменения порядка сегментов кнопка **"Сохранить"** перезаписывает тот же canonical `.bin` и обновляет sidecar;
- `Save range` может сохранить отрезок отдельно или добавить его к совместимому existing `.bin`;
- `Cut range` может сохранить файл без отрезка, сохранить копию без отрезка или добавить вырезанный отрезок к другому совместимому файлу.

### 8.5 Лог обмена с прибором

- **Хранение**: `DeviceExchangeLog` — потокобезопасный deque строк (`DeviceLogLine`: обычный текст и флаг жирного для блока идентификации); верхняя граница числа строк (~2000); уведомление UI через `PostMessage(WM_APP_DEVICE_LOG_REFRESH)` на главное окно.
- **Источники записей**: `SerialModule` (TX/RX, успешный кадр с полями из `ParsedToolFrame` / canonical данных), первая успешная метрология (`DeviceExchangeLogAppendDeviceIdentity` по `signature` + `serial` через `ToolDataAdapterRegistry`).
- **UI**: `DeviceLogPanel` — хост `WS_POPUP` у верхнего края главного окна; поле ввода — `RichEdit50W` при загрузке `Msftedit.dll`, иначе многострочный `EDIT`; развёрнутое окно — отдельный frame class.
- **Прокрутка**: определение «приклеен к низу» через `GetScrollInfo(SB_VERT)` с fallback на `EM_GETFIRSTVISIBLELINE`; инкрементальное дописывание хвоста при совпадении префикса текста и отсутствии новых жирных строк (Rich Edit); после первого `UpdateLayout` принудительный скролл вниз (`g_forceDockScrollToBottomOnce`), т.к. первый `RefreshBothEdits` может выполняться до финального размера панели.
- **Настройки COM в файле**: см. `GetAppSettingsPath()` / `app_settings.txt` в `Amk-Horizon-New-Chart.cpp` — не путать с логом.

## 9. Практические точки входа

Если меняете поведение, обычно смотреть сюда:

- `DataModule.h/.cpp`
- `ToolDataAdapters.h/.cpp`
- `ToolDataAccess.h`
- `ChartManager.cpp`
- `ChartModule.cpp`
- `CurveSelectionModule.cpp`
- `SchematicModule.cpp`
- `SerialModule.h/.cpp`
- `MetrologyModule.cpp`
- `NotesModule.h/.cpp`
- `DeviceExchangeLog.h/.cpp`
- `DeviceLogPanel.h/.cpp`

## 10. Что не нужно хранить в этом документе

Здесь **не нужен** архив исторических шагов миграции и старых экспериментов.

Для этого есть:

- `PROJECT_STATUS.md`
- `ADAPTER_REGISTRY_IMPLEMENTATION_PLAN.md`
- профильные историко-справочные документы в корне репозитория

---

Эта справка должна оставаться кратким документом про **текущие технические инварианты**, а не про историю проекта.
