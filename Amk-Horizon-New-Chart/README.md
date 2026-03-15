# AMK-Horizon-New-Chart

> Актуально для текущей ветки `main`.
>
> Этот файл — **главная точка входа в проект**. Если нужен быстрый и надёжный контекст, начинайте с него, затем переходите к `TECHNICAL_REFERENCE.md`, `ANALYSIS_GP_TOOL_DATA.md` и `../PROJECT_STATUS.md`.

## Что это за проект

`AMK-Horizon-New-Chart` — Win32 C++ приложение для загрузки, онлайн-опроса, визуализации и анализа данных геофизических приборов семейства `AUTONOM_5Tx`, `AUTONOM_5Tx_SDR` и `LWD_4Tx`.

Приложение умеет:

- открывать файлы `.bin`, `.biv`, `.DEV`;
- опрашивать прибор по COM;
- строить графики по нескольким типам данных;
- работать с немодальной схемой прибора и пресетами кривых;
- читать/писать метрологию для поддерживаемых сценариев;
- хранить заметки, привязанные к конфигурации графика.

## Какие документы считать опорными

Используйте эти документы как **current-state reference**:

- `README.md` — обзор проекта и актуальный набор возможностей;
- `TECHNICAL_REFERENCE.md` — технические инварианты UI, графиков, координат, модулей и расчётов;
- `ANALYSIS_GP_TOOL_DATA.md` — текущая точка истины по `GP_TOOL_DATA`, `LEGACY_GP_TOOL_DATA`, `METROLOGY_DATA`, адаптерам и canonical shape;
- `../PROJECT_STATUS.md` — краткий статус архитектуры, последних стабильных улучшений и оставшихся compatibility boundaries;
- `../FILE_FORMATS_ANALYSIS.md` — как реально читаются `.bin/.biv/.DEV`;
- `../FILE_VALIDATION_FOR_5TX_MODEL.md` — что именно валидируется при открытии файла.

Остальные Markdown-файлы в корне репозитория теперь приведены к актуальному состоянию, но часть из них носит **историко-справочный** характер и используется как дополнительный контекст, а не как основной источник истины.

## Актуальная архитектура

Проект построен вокруг `ChartManager`, который координирует модули:

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

### Актуальный pipeline данных

Текущее движение данных в проекте:

`File / COM -> ToolDataAdapterRegistry -> ParsedToolFrame -> canonical GP_TOOL_DATA + ToolRecordMetadata -> ToolDataAccessView / MeasurementView -> chart / UI logic`

Ключевые свойства этой модели:

- вход по файлам и COM уже маршрутизируется через `ToolDataAdapterRegistry`;
- `ParsedToolFrame` несёт `kind`, `capabilities` и `canonicalData`;
- `DataModule` хранит записи как `GP_TOOL_DATA` плюс `ToolRecordMetadata`;
- верхние слои постепенно опираются на `ToolDataAccessView` и `MeasurementView`;
- canonical `GP_TOOL_DATA` пока остаётся основной compatibility storage/API shape.

## Поддерживаемые структуры и форматы

### Поддерживаемые сигнатуры / типы данных

- `AUTONOM_5TX (151)`
- `AUTONOM_5TX_SDR (152)`
- `LWD_4TX (241)`

### Поддерживаемые контейнеры

- `.bin`
- `.biv`
- `.DEV`

Физический размер кадра:

- `.bin/.biv` — `240` байт полезной нагрузки;
- `.DEV` — `11` байт заголовка + `240` байт payload = `251` байт на физический кадр.

## Главные пользовательские возможности

### Графики и навигация

- один или два графика одновременно;
- одинарная и двойная Y-ось;
- zoom по X и Y;
- скроллинг;
- отображение по `FRAME_NUMBER` и по `TEMPERATURE`;
- режим `Surf`;
- корректная работа на больших наборах данных;
- заметки, привязанные к конфигурации и экранной геометрии графика.

### Кривые и схема прибора

- ручной выбор через схему прибора;
- быстрые пресеты:
  - все амплитуды для `R1`;
  - все амплитуды для `R2`;
  - отношения амплитуд;
  - разности фаз;
  - silent-амплитуды;
  - silent-разность фаз;
  - температура;
  - `УЭС`;
  - `Симм. фазы`;
- выбор частоты `400 кГц / 2000 кГц`;
- копирование конфигурации между графиками;
- автоматическое обновление цветов кривых.

### Runtime `4Tx/5Tx` поддержка в UI

Текущее поведение UI важно понимать правильно:

- визуальная схема по-прежнему использует 5 фиксированных слотов `T1..T5`;
- **но доступность передатчиков теперь определяется по текущим данным** через `ToolCapabilities.transmitterCount`;
- для `4Tx` сценариев:
  - `T5` в схеме показывается как недоступный;
  - выбрать `T5` нельзя;
  - в списке доступных кривых не появляется пятая кривая / пятый checkbox;
- для `5Tx` поведение остаётся полным.

### Цветовая логика кривых

В `CurveSelectionModule` действует централизованное назначение цветов через `RefreshCurveColors(...)`.

Текущие инварианты:

- цвета перераспределяются на весь набор кривых, а не выдаются локально по одной;
- для `T1..T4` закреплены приоритетные корпоративные цвета:
  - `T1` — мягкий зелёный;
  - `T2` — мягкий жёлтый;
  - `T3` — мягкий красный;
  - `T4` — мягкий синий;
- остальные цвета добираются из расширенной и процедурной палитры;
- палитра намеренно приглушена для комфортной длительной работы.

### COM и метрология

- COM-протокол использует пакет:
  - `[CMD:1][LEN:2][PAYLOAD][CRC16:2]`
- рабочие команды:
  - `0x11` — данные;
  - `0x12` — запись метрологии;
  - `0x13` — чтение метрологии;
  - `0x16` — сигнатура;
  - `0x17` — `selected_key`;
- рабочая скорость: `125000` бод;
- realtime in-tree path использует `ParsedToolFrame` и metadata-backed ingest.

## Важные текущие UI-инварианты

### `X Range` и `Surf`

Текущее поведение считается обязательным:

- поля `X Range` **не должны самопроизвольно перезаписывать пользовательский ввод** до явного применения;
- в COM polling режиме пользователь может задать только `From`, а `To` оставить пустым;
- это означает режим `From .. latest frame`, где правая граница автоматически следует за последними кадрами;
- в `Surf` поля `X Range` тоже не должны авто-подставляться поверх пользовательского ввода.

### Заметки

`NotesModule` хранит заметки как sidecar-данные, привязанные к:

- конфигурации кривых;
- типу оси X;
- индексу кадра;
- значениям левой/правой оси.

Заметки не являются отдельным историческим архивом графика — они привязаны к текущей конфигурационной модели.

## Что важно для разработчика

### Куда смотреть в коде

- `ChartManager.*` — общая orchestration логика, zoom, scroll, render pipeline, live path, export, notes;
- `DataModule.*` — file loading, canonical storage, range metadata, export seams;
- `ToolDataAdapters.*` — adapter-driven parsing и маршрутизация по структурам;
- `ToolDataAccess.h` — `ToolDataAccessView`, `MeasurementView`;
- `CurveSelectionModule.*` — сборка конфигураций кривых, SMT, цвета;
- `SchematicModule.*` — схема прибора, ручной выбор, пресеты, runtime `4Tx/5Tx` UI-ограничения;
- `SerialModule.*` — COM-протокол, polling, callbacks, метрология;
- `MetrologyModule.*` — работа с формой метрологии;
- `NotesModule.*` — загрузка/сохранение заметок и hit-test маркеров.

### Актуальные compatibility boundaries

Нужно учитывать, что:

- `GP_TOOL_DATA` всё ещё остаётся canonical storage/API shape;
- `SaveDataRange(...)` сохраняет canonical `.bin`;
- публичные:
  - `DataModule::GetData()`
  - `DataModule::ParseRawFrame(...)`
  - `SerialModule::RequestData(GP_TOOL_DATA& ...)`
  - `SerialModule::SetDataCallback(...)`
  трактуются как compatibility-only surface-ы;
- computer-side `SMT` в `FROM_COMPUTER` остаётся legacy-compatible вычислительной веткой, а не полностью adapter-aware универсальным solver-ом.

## Быстрый старт

### Сборка

1. Откройте `Amk-Horizon-New-Chart.vcxproj` в Visual Studio.
2. Выберите `Debug` или `Release`, `x64`.
3. Выполните сборку решения.

### Запуск

1. Запустите приложение.
2. Откройте файл данных или подключитесь к прибору по COM.
3. Настройте отображение через схему прибора или пресеты.
4. При необходимости включите заметки, экспорт диапазона или метрологические операции.

## Документ-карта по репозиторию

- основной обзор — `README.md`
- технические инварианты — `TECHNICAL_REFERENCE.md`
- структуры данных — `ANALYSIS_GP_TOOL_DATA.md`
- файловые контейнеры — `../FILE_FORMATS_ANALYSIS.md`
- файловая валидация — `../FILE_VALIDATION_FOR_5TX_MODEL.md`
- статус архитектуры — `../PROJECT_STATUS.md`
- adapter-driven миграция и remaining boundaries — `../ADAPTER_REGISTRY_IMPLEMENTATION_PLAN.md`
- COM и метрология — `../COM_5TX_GP_POLLING_AND_METROLOGY.md`
- canonical/legacy индексные схемы — `../GP_TOOL_DATA_INDEX_MAPPING.md`, `../GP_TOOL_DATA_LWD_AUTONOM.md`
- SMT `FROM_COMPUTER` — `../ues_phase_from_computer_5tx.md`

---

`README.md` должен оставаться коротким overview-документом про **текущую реализацию**, а не архивом исторических этапов.
