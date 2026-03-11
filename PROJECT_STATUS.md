# AMK Horizon New Chart — Project Status

## Текущее состояние
- Приложение использует adapter-driven parsing pipeline с canonical materialization в `GP_TOOL_DATA` (`240` байт).
- Поддержаны файловые контейнеры: `.bin`, `.biv`, `.DEV`.
- При открытии файла выполняется явная валидация: расширение, сигнатура, поддержка контейнера адаптером и кратность физическому размеру кадра.
- Поддерживаемые сигнатуры данных: `AUTONOM_5Tx (151)`, `AUTONOM_5Tx_SDR (152)`, `LWD_4Tx (241)`.
- COM-протокол в актуальном формате: `[CMD:1][LEN:2][PAYLOAD][CRC16:2]`.
- Используемые команды: `0x11`, `0x12`, `0x13`, `0x16`, `0x17`.

## Актуальная архитектурная логика
- Основной pipeline сейчас выглядит так:
  - `File/COM -> ToolDataAdapterRegistry -> ParsedToolFrame -> GP_TOOL_DATA + ToolRecordMetadata -> ToolDataAccessView / MeasurementView -> chart/UI logic`
- Основной realtime in-tree path использует:
  - `SerialModule::RequestData(ParsedToolFrame& ...)`
  - `SerialModule::SetParsedDataCallback(...)`
  - `ChartManager -> DataModule::AddParsedData(...)`
- Верхний слой постепенно переведён на `MeasurementView` и `ToolDataAccessView` вместо прямого широкого доступа к raw storage.

## Состояние метрологии (AUTONOM 5Tx)
- Форма метрологии работает как немодальное окно.
- Реализованы и проверены кнопки:
  - открыть/сохранить метрологию в `.bin`,
  - прочитать из зонда,
  - записать в зонд,
  - записать `selected_key`.
- Исправлено декодирование/сборка `signature` (legacy-совместимый формат, `number` до `999`).
- Поле `D, мм` удалено из UI формы метрологии; `D_sonde_mm` сохранено в структуре для сервисного использования.

## Последние стабильные улучшения
- Realtime ingest переведён на `ParsedToolFrame` / metadata-backed path без изменения поведения UI.
- В `DataModule` и `SerialModule` локализованы compatibility conversion seams между `ParsedToolFrame` и `GP_TOOL_DATA`.
- `CurveSelectionModule` и `SchematicModule` получили серию safe-step локализаций вокруг SMT preset handling и selection-driven curve assembly.
- Range export flow локализован и подготовлен к будущей adapter-aware export policy без изменения текущего canonical `.bin` поведения.
- Документация Phase 7 и compatibility boundaries приведена в актуальное состояние.
- Реализована асинхронная подготовка тяжёлых UI/data path-ов через `AsyncTaskQueue`, что убрало заметные подвисания при загрузке и части COM-операций.
- Добавлены многоуровневые кеши для больших данных: snapshot cache в `ChartManager`, derived-series cache в `DataModule`, screen-space polyline cache в `ChartModule`.
- Для больших наборов точек внедрены LOD/downsampling, throttling `WM_MOUSEMOVE`, уменьшение лишних redraw и корректная обработка `NaN` в рендере.
- Исправлена синхронизация zoom/draw/coordinate mapping для `.DEV` и повторного zoom на больших файлах.
- Исправлено позиционирование заметок от перекрестия; для пресетов разности фаз отключена некорректная привязка заметки к ближайшей кривой.

## Известные ограничения
- `GP_TOOL_DATA` всё ещё остаётся основной canonical storage/API shape, поэтому полное отделение format-specific behavior ещё не завершено.
- `SaveDataRange(...)` остаётся главным live canonical persistence boundary и по-прежнему пишет canonical `.bin`.
- Публичные `DataModule::GetData()`, `DataModule::ParseRawFrame(...)`, `SerialModule::RequestData(GP_TOOL_DATA& ...)` и `SerialModule::SetDataCallback(...)` сохраняются как compatibility-only surface-ы.
- Верхний UI и часть SMT-логики всё ещё сохраняют 5Tx-centric contract.
- Во время цикла измерения прибор может отвечать с задержкой (порядка ~600 мс по методичке).
- После открытия COM первый запрос к Arduino-эмулятору иногда требует ретрая (reset из-за DTR/RTS).

## Статус текущей миграции
- Phase 7 adapter/compatibility narrowing практически завершена в текущем scope.
- Текущая оценка завершённости scoped migration plan: примерно `97-100%`.
- Основной remaining work сейчас — не очередной безопасный code refactor, а formal compatibility classification публичных GP-level API и будущие policy/API решения.

## Контактные точки в коде
- Файловая валидация, загрузка, canonical storage и export seams: `Amk-Horizon-New-Chart/DataModule.cpp`
- Структуры данных, range-analysis и compatibility-facing public API: `Amk-Horizon-New-Chart/DataModule.h`
- Adapter registry и per-format parsing: `Amk-Horizon-New-Chart/ToolDataAdapters.h/.cpp`
- Stable access abstractions: `Amk-Horizon-New-Chart/ToolDataAccess.h`
- Протокол COM, polling, parsed callbacks и запись потока: `Amk-Horizon-New-Chart/SerialModule.cpp`
- UI orchestration и live ingest flow: `Amk-Horizon-New-Chart/ChartManager.cpp`
- SMT/preset UI binding и selection logic: `Amk-Horizon-New-Chart/SchematicModule.cpp`, `Amk-Horizon-New-Chart/CurveSelectionModule.cpp`
