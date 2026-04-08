# GP_TOOL_DATA / LEGACY_GP_TOOL_DATA / METROLOGY_DATA — актуальная структура данных проекта

Этот документ — **основная точка истины** по текущей data-модели приложения.

Он описывает:

- canonical `GP_TOOL_DATA`;
- legacy raw layout `LEGACY_GP_TOOL_DATA`;
- `METROLOGY_DATA`;
- adapter-driven parsing;
- capability metadata;
- текущие границы между raw layout и canonical in-memory shape.

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

## 4. Legacy raw layout `LEGACY_GP_TOOL_DATA`

```cpp
struct LEGACY_GP_TOOL_DATA {
    uint32_t signature;
    uint32_t condition;
    float Ro_smt[8];
    float PH_smt[8];
    uint32_t frame;
    float temperature;
    float AM_RX_1[10];
    float AM_RX_2[10];
    float PH_RX_1[10];
    float PH_RX_2[10];
};
```

Размер:

- `sizeof(LEGACY_GP_TOOL_DATA) == 240`

Этот layout важен **не как основной формат проекта**, а как raw-представление части старых `LWD_4TX` данных.

## 5. Tool kinds и capabilities

Текущие поддерживаемые типы:

- `AUTONOM_5TX`
- `AUTONOM_5TX_SDR`
- `LWD_4TX`

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

### 6.1 `Autonom5TxAdapter`

Назначение:

- `AUTONOM_5TX`
- `AUTONOM_5TX_SDR`

Особенности:

- `transmitterCount = 5`
- `hasT5 = true`
- raw frame парсится напрямую как canonical `GP_TOOL_DATA`

### 6.2 `Lwd4TxAdapter`

Назначение:

- `LWD_4TX`

Особенности:

- `transmitterCount = 4`
- `hasT5 = false`
- raw legacy layout materialize-ится в canonical `GP_TOOL_DATA`

## 7. Важная деталь по `LWD_4TX`: старые и новые 240-байтные файлы

`LWD_4TX` — особый случай.

Сейчас приложение умеет работать с двумя вариантами 240-байтных кадров:

- **legacy raw `LEGACY_GP_TOOL_DATA`**
- **canonical `GP_TOOL_DATA`**

### 7.1 Как различение происходит в коде

`DataModule::ParseRawFrameDetailed(...)` для `LWD_4TX` использует эвристику:

- сначала смотрит, не содержатся ли в “пятом слоте” canonical view ненулевые значения;
- затем делает plausibility check по `frame` и `temperature`;
- если canonical трактовка выглядит плохо, а legacy выглядит правдоподобно, кадр идёт через `Lwd4TxAdapter`;
- иначе допускается canonical passthrough.

### 7.2 Почему это нужно

Это позволяет одновременно поддерживать:

- старые legacy `4Tx` файлы;
- уже сохранённые canonical `4Tx` файлы;
- обычные `5Tx` сценарии.

## 8. Как legacy `LWD_4TX` materialize-ится в canonical shape

### 8.1 Индексация legacy flat-массивов

Для active TX:

- `0..3` -> `400 кГц`, `T1..T4`
- `4..7` -> `2000 кГц`, `T1..T4`

Для silent:

- `8` -> silent `400 кГц`
- `9` -> silent `2000 кГц`

Линейный индекс:

```cpp
linearIndex = (freqIdx == 0) ? txIdx : (4 + txIdx);
```

где `txIdx = 0..3`.

### 8.2 Mapping

Адаптер materialize-ит legacy raw frame в canonical shape так:

- `rho_smt[freqIdx][txIdx] = Ro_smt[linearIndex]`
- `phase_smt[freqIdx][txIdx] = PH_smt[linearIndex]`
- `AM_RX_1[freqIdx][txIdx] = AM_RX_1[linearIndex]`
- `AM_RX_2[freqIdx][txIdx] = AM_RX_2[linearIndex]`
- `ZERO_AM_RX_1[freqIdx] = AM_RX_1[8 + freqIdx]`
- `ZERO_AM_RX_2[freqIdx] = AM_RX_2[8 + freqIdx]`

Разность фаз строится по раздельным фазам приёмников:

- `DELTA_PH[freqIdx][txIdx] = Normalize(PH_RX_2 - PH_RX_1)`, если `abs(diff) < 40`
- иначе `0`

Silent `ZERO_dPH` строится аналогично по слотам `8 + freqIdx`.

### 8.3 Что важно для верхнего слоя

После materialization верхние слои **не должны** обращаться к raw `PH_RX_1` / `PH_RX_2`.

Они должны мыслить через:

- `ToolDataAccessView`
- `MeasurementView`
- canonical `GP_TOOL_DATA`

## 9. Единицы и преобразования

Верхний слой переводит данные в пользовательские единицы:

- амплитуда -> `mV`
- фаза -> `mdeg`

Актуальные базовые формулы:

- амплитуда -> `value * 4000.0 / 2^24`
- фаза -> `NormalizePhase(value) * 57295.7795`

## 10. Runtime `4Tx/5Tx` следствия для UI

Capability metadata влияет не только на расчёты, но и на интерфейс:

- график не должен создавать 5-ю кривую для `4Tx`;
- схема прибора не должна позволять выбрать `T5` для `4Tx`;
- список checkbox-ов в окне схемы не должен включать пятую кривую для `4Tx`.

Это уже часть **реальной текущей реализации**, а не только желаемое поведение.

## 11. Метрология `METROLOGY_DATA`

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

## 12. Что считать главным практическим выводом

### 12.1 Что **неверно**

Неверно считать, что:

- `GP_TOOL_DATA` = raw on-disk / on-wire format;
- `LWD_4TX` всегда означает один и тот же бинарный layout;
- UI всегда должен показывать одинаковое число TX независимо от capabilities.

### 12.2 Что **верно**

Верно считать, что:

- `GP_TOOL_DATA` — canonical in-memory compatibility shape;
- actual raw parsing идёт через адаптеры;
- capability metadata определяет runtime поведение графиков и части UI;
- `LWD_4TX` требует различать legacy raw layout и canonical 240-byte frame.

---

Если какой-либо другой документ расходится с этим файлом, ориентироваться нужно на код и этот документ.

