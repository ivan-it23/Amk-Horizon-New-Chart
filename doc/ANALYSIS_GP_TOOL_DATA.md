# GP_TOOL_DATA / METROLOGY_DATA — актуальная структура данных проекта

Этот документ — **основная точка истины** по текущей data-модели приложения.

Он описывает:

- canonical `GP_TOOL_DATA`;
- `METROLOGY_DATA`;
- palette-основанное декодирование toolCode;
- adapter-driven parsing через `UniversalCanonicalAdapter`;
- capability metadata.

## 1. Источники истины в коде

- `ToolDataTypes.h`
- `ToolDataAdapters.h/.cpp`
- `ToolDataAccess.h`
- `DataModule.h/.cpp`
- `SerialModule.h`

## 2. Актуальный pipeline

Текущее движение данных:

`File / COM -> ToolDataAdapterRegistry -> ParsedToolFrame -> canonical GP_TOOL_DATA + ToolRecordMetadata -> ToolDataAccessView / MeasurementView -> графики / UI`

Практический смысл:

- raw layout и canonical shape — это **не одно и то же**;
- `ParsedToolFrame` — adapter-level результат парсинга;
- `GP_TOOL_DATA` — current canonical compatibility shape;
- `ToolRecordMetadata` и `ToolCapabilities` нужны для capability-aware логики верхних слоёв.

## 3. Canonical структура `GP_TOOL_DATA`

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

- `sizeof(GP_TOOL_DATA) == 240`

### 3.1 Индексация canonical shape

- `freqIdx = 0` -> `400 кГц`
- `freqIdx = 1` -> `2000 кГц`
- `txIdx = 0..4` -> `T1..T5`

Silent-каналы хранятся **отдельно**, а не как “нулевой передатчик”:

- `ZERO_AM_RX_1[2]`
- `ZERO_AM_RX_2[2]`
- `ZERO_dPH[2]`

### 3.2 Что реально читают верхние слои

- амплитуда `R1` -> `AM_RX_1[freqIdx][txIdx]`
- амплитуда `R2` -> `AM_RX_2[freqIdx][txIdx]`
- разность фаз `R2-R1` -> `DELTA_PH[freqIdx][txIdx]`
- симметризованные фазы -> `phase_smt[freqIdx][txIdx]`
- симметризованное УЭС -> `rho_smt[freqIdx][txIdx]`
- silent-амплитуды -> `ZERO_AM_RX_1`, `ZERO_AM_RX_2`
- silent-разность фаз -> `ZERO_dPH`

## 4. Palette-основанное декодирование toolCode

toolCode извлекается из signature: `toolCode = (signature << 12) >> 12` (нижние 20 бит) / 1000.

toolCode — трёхзначное число XYZ:

- **X** — тип прибора (`DeviceType`): `1` = Автоном, `2` = LWD
- **Y** — число передатчиков: `3`, `4`, `5`
- **Z** — модификация

Примеры:

- `151` → Автоном, 5 передатчиков, мод1
- `241` → LWD, 4 передатчика, мод1
- `231` → LWD, 3 передатчика, мод1

Новые toolCode поддерживаются автоматически без изменения кода.

## 5. Tool kinds и capabilities

Текущие поддерживаемые типы (определяются динамически из palette):

- `AUTONOM` (первая цифра = 1)
- `LWD` (первая цифра = 2)

### 5.1 `ToolCapabilities`

`ToolCapabilities` сейчас определяют:

- `transmitterCount`
- `hasT5`
- `hasSeparatePhaseRx`
- `hasDeltaPhase`
- `hasZeroAmplitudeRx1`
- `hasZeroAmplitudeRx2`
- `hasZeroDeltaPhase`
- `hasSondeRhoSmt`
- `hasSondePhaseSmt`
- поддержку контейнеров
- поддержку COM polling
- поддержку silent-данных

### 5.2 Почему это важно

Верхний слой больше не должен слепо считать, что данных всегда 5 TX.

Например:

- графики используют capability-aware доступ;
- схема прибора runtime-ограничивает `T5` для `4Tx`;
- список кривых не должен показывать пятый передатчик там, где его нет в данных.

## 6. Адаптеры

### 6.1 `UniversalCanonicalAdapter`

Единственный адаптер, обрабатывающий все типы приборов:

- принимает любую сигнатуру с `toolCode > 0`
- определяет `DeviceType`, `transmitterCount`, `modification` из palette
- canonical passthrough: `memcpy` без преобразований
- статические методы: `DetermineDeviceType()`, `DetermineTransmitterCount()`, `DetermineModification()`, `BuildCapabilities()`, `BuildDisplayName()`

Удалённые специализированные адаптеры:

- `Autonom5TxAdapter` — удалён, заменён `UniversalCanonicalAdapter`
- `Lwd3TxCanonicalAdapter` — удалён
- `Lwd4TxAdapter` — удалён (вместе с `LEGACY_GP_TOOL_DATA`)

### 6.2 Реестр адаптеров

`ToolDataAdapterRegistry` содержит только `UniversalCanonicalAdapter`.

## 7. Единицы и преобразования

Верхний слой переводит данные в пользовательские единицы:

- амплитуда -> `mV`
- фаза -> `mdeg`

Актуальные базовые формулы:

- амплитуда -> `value * 4000.0 / 2^24`
- фаза -> `NormalizePhase(value) * 57295.7795`

## 8. Runtime 3Tx/4Tx/5Tx следствия для UI

Capability metadata влияет не только на расчёты, но и на интерфейс:

- график не создаёт кривые для недоступных передатчиков;
- схема прибора не позволяет выбрать недоступные передатчики;
- checkbox-ы скрываются динамически по capability.

## 9. Метрология `METROLOGY_DATA`

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

- `sizeof(METROLOGY_DATA) == 240`

Ключевые поля, реально используемые приложением:

- `signature`
- `serial`
- `L1`
- `L2`
- `F`
- `Air_zz`
- `Air_zz_amt`
- `D_sonde_mm`

## 10. Что считать главным практическим выводом

- все приборы используют единую canonical структуру `GP_TOOL_DATA` (240 байт);
- `LEGACY_GP_TOOL_DATA` удалена из кодовой базы;
- тип прибора и capabilities определяются динамически из palette (toolCode);
- один `UniversalCanonicalAdapter` обрабатывает все toolCode;
- новые приборы поддерживаются автоматически без изменения кода;
- legacy COM-протокол удалён, используется только modern протокол `[CMD][LEN:2][PAYLOAD][CRC16:2]`.

---

Если какой-либо другой документ расходится с этим файлом, ориентироваться нужно на код и этот документ.

