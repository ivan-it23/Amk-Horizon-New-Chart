# AMK-Horizon-New-Chart — Project Status

## Текущее состояние

Проект находится в стабильном current-state, где:

- file и COM ingest уже работают через adapter-driven parsing;
- данные materialize-ятся в canonical `GP_TOOL_DATA` (`240` байт);
- рядом с каждой записью хранится `ToolRecordMetadata`;
- верхний слой частично переведён на `ToolDataAccessView` и `MeasurementView`;
- UI, графики, метрология, заметки и экспорт работают в одной согласованной модели.

## Поддерживаемые данные и контейнеры

- сигнатуры:
  - `AUTONOM_5TX (151)`
  - `AUTONOM_5TX_SDR (152)`
  - `LWD_4TX (241)`
- контейнеры:
  - `.bin`
  - `.biv`
  - `.DEV`

## Актуальная архитектурная модель

Основной pipeline:

`File / COM -> ToolDataAdapterRegistry -> ParsedToolFrame -> GP_TOOL_DATA + ToolRecordMetadata -> ToolDataAccessView / MeasurementView -> chart / UI`

Realtime in-tree path:

- `SerialModule::RequestData(ParsedToolFrame& ...)`
- `SerialModule::SetParsedDataCallback(...)`
- `ChartManager -> DataModule::AddParsedData(...)`

## Последние стабильные улучшения

- **лог обмена с прибором**: немодальная панель (`DeviceLogPanel`) + буфер `DeviceExchangeLog`, Rich Edit с жирным блоком идентификации (тип прибора, число передатчиков, S/N), TX/RX и строки кадра с `signature` / `condition` / `frame`; прокрутка «как в консоли» (пин к низу, инкремент без полной пересборки где возможно);
- настройки приложения в `%LOCALAPPDATA%\AMK-Horizon-New-Chart\app_settings.txt` (`lastComPort`, `lastRecordFolder` и др.);
- quick-connect UI: кнопки "Включить/Отключить" и "▶/■" на панели управления Chart 1;
- сохранение/восстановление последнего COM-порта и папки записи между сессиями;
- adapter-driven parsing приведён к current-state для файлов и COM;
- `LWD_4TX` поддерживает различение legacy raw layout и canonical 240-byte frames;
- `DataModule` получил range-analysis seam:
  - `DataRangeMetadataSummary`
  - `TryDescribeRangeMetadata(...)`
  - `DataRangeExportAnalysis`
  - `TryAnalyzeExportRange(...)`
- range export остаётся canonical `.bin`, но mixed-range ситуации уже диагностируются явно;
- `SchematicModule` синхронизирован с runtime числом передатчиков:
  - для `4Tx` `T5` недоступен;
  - пятая кривая/checkbox не показываются;
- `CurveSelectionModule` централизовал цветовую логику:
  - `T1` зелёный
  - `T2` жёлтый
  - `T3` красный
  - `T4` синий
- заметки работают через `NotesModule` и sidecar-модель;
- X Range / Surf UI-инварианты приведены к стабильному поведению без нежелательного auto-fill.

## Что уже считается стабильным user-visible behavior

- открытие `.bin/.biv/.DEV`;
- realtime polling по COM;
- чтение сигнатуры, метрологии и `selected_key`;
- ручной выбор кривых и пресеты;
- dual-axis и temperature X-axis;
- заметки и hit-test marker-ов;
- canonical range export;
- runtime `4Tx/5Tx` ограничения в схеме прибора;
- просмотр лога обмена с прибором без блокировки главного окна.

## Текущие compatibility boundaries

Наиболее важные оставшиеся границы:

- canonical storage и public compatibility surface по-прежнему построены вокруг `GP_TOOL_DATA`;
- `SaveDataRange(...)` остаётся главным live persistence boundary;
- публичные:
  - `DataModule::GetData()`
  - `DataModule::ParseRawFrame(...)`
  - `SerialModule::RequestData(GP_TOOL_DATA& ...)`
  - `SerialModule::SetDataCallback(...)`
  сохраняются как compatibility-only API;
- computer-side `SMT` остаётся legacy-compatible вычислительной веткой, а не полностью general adapter-aware подсистемой.

## Известные ограничения

- canonical `.bin` export не сохраняет исходный container/layout и не является adapter-aware export policy;
- UI визуально всё ещё использует 5-слотовую схему передатчиков, хотя runtime доступность уже capability-aware;
- computer-side `SMT` логика всё ещё опирается на legacy 4-delta-input модель;
- полное удаление canonical compatibility boundary пока не завершено и не требуется для текущего стабильного behavior.

## Что считать рекомендуемым current-state подходом

Если вы развиваете код дальше:

- для доступа к данным предпочитайте:
  - `TryGetRecord(...)`
  - `TryGetRecordMetadata(...)`
  - `GetDataView(...)`
  - `GetMeasurementView(...)`
- для парсинга предпочитайте `ParseRawFrameDetailed(...)`;
- для realtime предпочитайте `ParsedToolFrame` callback path;
- для UI логики учитывайте `ToolCapabilities.transmitterCount`, а не только фиксированные UI-массивы.

## Где искать актуальный контекст

- обзор — `Amk-Horizon-New-Chart/README.md`
- технические инварианты — `Amk-Horizon-New-Chart/TECHNICAL_REFERENCE.md`
- структуры данных — `Amk-Horizon-New-Chart/ANALYSIS_GP_TOOL_DATA.md`
- форматы файлов — `FILE_FORMATS_ANALYSIS.md`
- файловая валидация — `FILE_VALIDATION_FOR_5TX_MODEL.md`
- adapter migration / remaining boundaries — `ADAPTER_REGISTRY_IMPLEMENTATION_PLAN.md`

## Краткий вывод

Проект сейчас находится не в стадии сырой миграции, а в стадии **стабилизированного adapter-driven current-state**:

- основная архитектура уже сформирована;
- user-visible поведение стабильно;
- оставшиеся задачи в основном связаны не с аварийным рефакторингом, а с аккуратным сужением compatibility boundary и дальнейшей эволюцией export / API policy.
