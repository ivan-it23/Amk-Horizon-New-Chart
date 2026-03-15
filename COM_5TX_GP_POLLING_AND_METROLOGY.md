# COM polling и метрология — current-state reference

> Историческое имя файла осталось прежним, но содержание ниже относится к **текущей реализации** `AMK-Horizon-New-Chart`, а не к старому проекту.

## Назначение документа

Этот документ фиксирует актуальный COM/метрологический контракт:

- формат пакета;
- рабочие команды;
- текущий polling path;
- место `ParsedToolFrame` в live ingest;
- роль compatibility-only GP API;
- текущую работу с `METROLOGY_DATA`.

## 1. Базовый протокол

### 1.1 Формат пакета

Текущий COM-протокол:

`[CMD:1][LEN:2][PAYLOAD][CRC16:2]`

Где:

- `CMD` — код команды;
- `LEN` — длина пакета;
- `PAYLOAD` — полезные данные;
- `CRC16` — `CRC16 Modbus`.

### 1.2 Скорость

Рабочая скорость:

- `125000` бод

## 2. Рабочие команды

В текущем `SerialModule.h` используются:

- `0x11` — `CMD_DATA`
- `0x12` — `CMD_SET_METROLOGY`
- `0x13` — `CMD_GET_METROLOGY`
- `0x16` — `CMD_GET_SIGNATURE`
- `0x17` — `CMD_SET_SELECTED_KEY`

## 3. Текущий runtime flow

### 3.1 Подключение

При подключении приложение:

1. открывает COM-порт;
2. запрашивает сигнатуру;
3. определяет `ToolDataKind` / protocol mode;
4. подготавливает polling;
5. при необходимости считывает метрологию.

### 3.2 Polling

Polling в текущем проекте управляется `SerialModule` и `ChartManager`.

Рекомендуемый live path:

- `SerialModule::RequestData(ParsedToolFrame& ...)`
- `SerialModule::SetParsedDataCallback(...)`
- `ChartManager -> DataModule::AddParsedData(...)`

### 3.3 Compatibility-only wrappers

Следующие GP-level API сохраняются, но считаются compatibility-only:

- `SerialModule::RequestData(GP_TOOL_DATA& ...)`
- `SerialModule::SetDataCallback(...)`

Они не являются предпочтительным current-state путём для развития кода.

## 4. Что реально приходит из COM

Текущее приложение **не строит live path вокруг старой `_5Tx_DATA` как публичной модели**.

Сейчас поведение такое:

- `SerialModule` получает payload;
- payload парсится через adapter-driven path;
- получается `ParsedToolFrame`;
- при необходимости извлекается canonical `GP_TOOL_DATA`.

Практический вывод:

- COM ingest уже выровнен с file-ingest архитектурой;
- файл и COM используют одну и ту же adapter-driven модель;
- различия между `4Tx` и `5Tx` проходят через adapters и `ToolCapabilities`.

## 5. Метрология

### 5.1 Структура

Текущая структура метрологии:

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

- `240` байт

### 5.2 Операции

Текущие операции `SerialModule`:

- `RequestMetrology(...)`
- `WriteMetrology(...)`
- `SetSelectedKey(...)`
- `RequestSignature(...)`

### 5.3 Практические ограничения

- метрология и её компьютер-side использование должны совпадать по `signature`;
- `FROM_COMPUTER` SMT-ветка использует метрологию только при совпадении сигнатуры;
- текущий UI и расчёты не должны предполагать, что метрология автоматически универсальна для любой модели вне adapter/capability контекста.

## 6. Ошибки, таймауты и ретраи

С точки зрения текущего поведения приложения важно учитывать:

- возможны таймауты при первом обращении к прибору;
- polling не должен ломать UI при единичных сбоях;
- метрологические запросы не должны конфликтовать с обычным обменом;
- live path должен сохранять canonical совместимость без обхода adapter parsing.

## 7. Что считать current-state истиной

Для COM и метрологии источники истины:

- `Amk-Horizon-New-Chart/SerialModule.h`
- `Amk-Horizon-New-Chart/SerialModule.cpp`
- `Amk-Horizon-New-Chart/ToolDataAdapters.h/.cpp`
- `Amk-Horizon-New-Chart/ChartManager.cpp`

## 8. Что больше не нужно считать актуальным

Не нужно считать current-state описанием:

- детали старого проекта `TEST_SONDE_KARTOGRAF_2.0`;
- прямую работу верхнего слоя через `_5Tx_DATA`;
- старые transport-specific цепочки, в которых live data сначала писались во временный файл, а потом перечитывались как основной путь.

## 9. Краткий вывод

Текущая COM-модель проекта такая:

- пакетный протокол остаётся `[CMD][LEN][PAYLOAD][CRC16]`;
- рабочая скорость — `125000`;
- live ingest уже идёт через `ParsedToolFrame`;
- adapter-driven parsing одинаково важен и для COM, и для файлов;
- GP-level COM API остаётся только compatibility surface-ом.
