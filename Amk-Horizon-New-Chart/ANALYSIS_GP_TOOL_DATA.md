# GP_TOOL_DATA / METROLOGY_DATA — актуальные структуры, canonical shape и протокол

Этот документ — **короткая “точка истины”** по структурам данных прибора и тому, как они используются в приложении.

Источник истины в коде:
- `DataModule.h`: `GP_TOOL_DATA` (+ `static_assert(sizeof(GP_TOOL_DATA) == 240)`)
- `SerialModule.h`: `METROLOGY_DATA` (+ `static_assert(sizeof(METROLOGY_DATA) == 240)`)
- `ToolDataAdapters.h/.cpp`: `ParsedToolFrame`, `ToolDataAdapterRegistry`, adapter-driven parsing
- `ToolDataAccess.h`: `ToolDataAccessView`, `MeasurementView`, общий compatibility seam `ParsedToolFrame -> GP_TOOL_DATA`
- методические материалы: `Методичка-фото-1.png`, `Методичка-фото-2.png`

---

## 0) Как эти структуры используются сейчас

Текущий pipeline данных в коде:

`File/COM -> ToolDataAdapterRegistry -> ParsedToolFrame -> canonical GP_TOOL_DATA + ToolRecordMetadata -> ToolDataAccessView / MeasurementView -> графики / UI`

Важно понимать текущее разделение ролей:

- `ParsedToolFrame` — это adapter-level результат парсинга, который несёт `kind`, `capabilities` и `canonicalData`
- `GP_TOOL_DATA` — это не единственная «сырой формат прибора» истина, а текущая canonical compatibility shape для хранения, расчётов и части публичных API
- `ToolRecordMetadata` хранится рядом с canonical записью и даёт верхним слоям capability-aware контекст
- публичные `DataModule::ParseRawFrame(...)`, `DataModule::GetData()`, `SerialModule::RequestData(GP_TOOL_DATA& ...)` и `SerialModule::SetDataCallback(...)` сейчас следует трактовать как compatibility-only surface-ы

---

## 1) Структура данных измерений `GP_TOOL_DATA` (240 байт)

```cpp
// DataModule.h
#pragma pack(push, 1)
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
#pragma pack(pop)
```

### Индексация
- **Частоты**: `freqIdx = 0 → 400 kHz`, `freqIdx = 1 → 2000 kHz`
- **Передатчики**: `txIdx = 0..4 → T1..T5`
- **Молчащий режим**: это отдельные поля `ZERO_AM_RX_*[2]`, `ZERO_dPH[2]` (не “T0 в массиве”)

Примечание по текущей архитектуре:

- часть входных данных уже может приходить не из нативной 5Tx raw-структуры, а из адаптера (`LWD_4Tx`) с последующей canonical materialization в `GP_TOOL_DATA`
- поэтому верхним слоям нельзя безоговорочно считать `GP_TOOL_DATA` синонимом «исходный on-disk/on-wire layout»

### Что из чего строится на графиках (логика DataModule)
- **Амплитуда RX1**: `AM_RX_1[freqIdx][txIdx]`
- **Амплитуда RX2**: `AM_RX_2[freqIdx][txIdx]`
- **Разность фаз (R2−R1)**: `DELTA_PH[freqIdx][txIdx]`
- **Отношение амплитуд (RX2/RX1)**: `AM_RX_2 / AM_RX_1`
- **Симметризованные данные**:
  - `phase_smt[freqIdx][txIdx]`
  - `rho_smt[freqIdx][txIdx]`
- **Молчащий режим**:
  - RX1: `ZERO_AM_RX_1[freqIdx]`
  - RX2: `ZERO_AM_RX_2[freqIdx]`
  - \(\Delta\varphi\): `ZERO_dPH[freqIdx]`

### Единицы и преобразования
В коде выполняются преобразования для отображения в физических единицах:
- **амплитуды → мВ**: `value * 4000.0 / 2^24`
- **фазы (рад) → мдег**: `NormalizePhase(value) * 57295.7795`

---

## 2) Метрология `METROLOGY_DATA` (240 байт)

```cpp
// SerialModule.h
#pragma pack(push, 1)
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
#pragma pack(pop)
```

### Что использует приложение
- **серийный номер**: `serial`
- **диаметр зонда (мм)**: `D_sonde_mm`
- **калибровочные поля формы метрологии**: `L1/L2/F/Air_zz/Air_zz_amt`

Эти поля используются для:
- отображения информации в диалоге подключения
- **именования файла записи**: `AMK_[serial]_[D_sonde_mm]_[YYYYMMDD]_[HHMMSS].bin`
- чтения/редактирования/записи в форме метрологии AUTONOM 5Tx

Примечание по UI:
- поле `D, мм` удалено из формы метрологии,
- но `D_sonde_mm` сохраняется в структуре и продолжает использоваться в сервисных сценариях (например, имя файла записи потока).

---

## 3) Форматы файлов `.bin/.biv/.DEV`

- `.bin` — последовательность `GP_TOOL_DATA` (240 байт на кадр)
- `.biv` — идентичен `.bin`
- `.DEV` — **11 байт служебных + 240 байт данных** = 251 байт на кадр  
  Приложение заголовок пропускает. По методичке эти 11 байт добавляются для внешней совместимости.

С точки зрения текущего кода:

- файловая валидация и выбор структуры идут через `signature` и `ToolDataAdapterRegistry`
- поддерживаются сигнатуры `151`, `152` и `241`
- после adapter parsing файл всё ещё материализуется в canonical `GP_TOOL_DATA` + `ToolRecordMetadata`
- canonical `.bin` export остаётся текущим compatibility persistence contract

Подробности и схемы: `..\FILE_FORMATS_ANALYSIS.md`.

---

## 4) Протокол COM (по методичке)

### UART
- 125000 бод

### Формат пакета
`[CMD:1][LEN:2][PAYLOAD:(LEN-5)][CRC16:2]`, где `LEN` — общее число байт в пакете.

CRC16 (Modbus) считается по `CMD+LEN+PAYLOAD` (первые `LEN-2` байт).

### Команды
- `0x11` — запрос измерений (`GP_TOOL_DATA`, 240 байт)
- `0x13` — запрос метрологии (`METROLOGY_DATA`, 240 байт)
- `0x16` — запрос сигнатуры (`uint32_t`, 4 байта)
- `0x12` — запись метрологии (квитанция; используется кнопкой "Записать в зонд")
- `0x17` — запись `selected_key` (квитанция; используется кнопкой "Записать команду")

С точки зрения runtime path:

- основной realtime in-tree flow сейчас использует `RequestData(ParsedToolFrame& ...)` и `SetParsedDataCallback(...)`
- GP-level перегрузки `RequestData(GP_TOOL_DATA& ...)` и `SetDataCallback(...)` остаются compatibility wrapper-ами

### Тайминги (почему нужны ретраи)
В методичке указано, что цикл измерения порядка **600 мс** — в этот момент прибор может не отвечать. Также Arduino-эмулятор часто перезагружается при открытии COM из-за DTR/RTS, поэтому первый запрос метрологии может таймаутиться — в приложении предусмотрены повторные попытки.

