# Поддержка `.bin`, `.biv`, `.DEV` в модели `_5Tx_DATA`

## Назначение документа

Это **краткая историческая справка** о том, как legacy-логика старого приложения соотносится с новой моделью `_5Tx_DATA`.

Документ нужен как контекст переноса, но **не является описанием текущей реализации**. Для текущего поведения приложения используйте:

- `FILE_FORMATS_ANALYSIS.md`
- `FILE_VALIDATION_FOR_5TX_MODEL.md`
- `Amk-Horizon-New-Chart/DataModule.cpp`

---

## 1. Что важно из legacy-логики

Старый проект использовал те же базовые правила контейнеров:

- `.biv` читался так же, как `.bin`
- `.DEV` содержал `11` служебных байт перед полезным кадром
- тип прибора определялся по сигнатуре, прочитанной с правильного смещения

Правило декодирования типа:

```text
toolCode = (((signature << 12) >> 12) / 1000)
```

Поддерживаемые семейства в старом проекте:

- `151` → `AUTONOM_5Tx`
- `152` → `AUTONOM_5Tx_SDR`
- `241` → `LWD_4Tx`

---

## 2. Что означает это для новой архитектуры

Целевая внутренняя структура новой архитектуры:

```cpp
struct _5Tx_DATA {
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

Практический вывод из старой логики:

- `AUTONOM_5Tx` и `AUTONOM_5Tx_SDR` могут читаться почти напрямую как `_5Tx_DATA`
- `LWD_4Tx` требует отдельного чтения `_4Tx_DATA` и адаптации в `_5Tx_DATA`

---

## 3. Что реализовано сейчас, а что нет

### Реализовано сейчас

- чтение `.bin`, `.biv`, `.DEV` как контейнеров для `GP_TOOL_DATA`
- чтение сигнатуры с корректного смещения
- adapter-driven валидация и маршрутизация по сигнатурам `151`, `152`, `241`
- canonical materialization в `GP_TOOL_DATA` после adapter parsing

### Пока не реализовано

- полное устранение canonical compatibility boundary после adapter parsing
- adapter-aware export/persistence path вместо текущего canonical `.bin`
- устранение оставшихся 5Tx-centric UI/SMT contract assumptions

То есть этот документ сохраняет **исторический контекст**, но не должен читаться как описание текущей реализации без сверки с `FILE_FORMATS_ANALYSIS.md`, `FILE_VALIDATION_FOR_5TX_MODEL.md` и `ADAPTER_REGISTRY_IMPLEMENTATION_PLAN.md`.

---

## 4. Если когда-нибудь возвращаться к поддержке `LWD_4Tx`

Минимальный план будет таким:

1. Описать структуру `_4Tx_DATA`
2. Реализовать чтение `.bin/.biv/.DEV` для `_4Tx_DATA`
3. Добавить адаптер `_4Tx_DATA -> _5Tx_DATA`
4. Определить, нужен ли вообще отдельный post-canonical storage/export path для `241`, или текущая canonical materialization остаётся допустимой compatibility моделью
5. Обновить `FILE_FORMATS_ANALYSIS.md`, `FILE_VALIDATION_FOR_5TX_MODEL.md` и `ADAPTER_REGISTRY_IMPLEMENTATION_PLAN.md`

До этого момента текущий контракт приложения остаётся простым:

валидны файлы, которые поддерживаются текущим adapter-driven parsing path и затем материализуются в canonical `GP_TOOL_DATA`

---

## 5. Источники истины

- legacy-контекст — этот документ
- текущая реализация — `FILE_FORMATS_ANALYSIS.md`, `FILE_VALIDATION_FOR_5TX_MODEL.md`, `DataModule.cpp`

Если между ними есть расхождения, для текущего поведения приложения нужно ориентироваться на код и краткие актуальные документы.
