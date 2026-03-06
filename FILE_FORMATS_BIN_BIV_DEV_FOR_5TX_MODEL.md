# Поддержка `.bin`, `.biv`, `.DEV` в модели `_5Tx_DATA`

## Цель

Этот документ описывает перенос чтения файлов старого приложения в новую архитектуру, где внутренняя модель данных только одна — `_5Tx_DATA`.

Рассматриваются только два типа приборов:

- `AUTONOM_5Tx = 151`
- `AUTONOM_5Tx_SDR = 152`
- `LWD_4Tx = 241`

## Целевая структура

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

## Главный вывод

- `.biv` в старом приложении читается **точно так же, как `.bin`**
- `.DEV` для `Автономный` и `LWD` — это полезная структура с **11 байтами служебного префикса**
- `AUTONOM_5Tx` можно читать почти напрямую в `_5Tx_DATA`
- `LWD_4Tx` нужно читать как `_4Tx_DATA`, а затем преобразовывать в `_5Tx_DATA`

## Где находится логика в старом проекте

- `MyForm.h` → `data_Click(...)`
- `variable.cpp` → `define_sonde_type(...)`
- `variable.cpp` → `read_blok(...)`
- `variable.cpp` → `transform_data(...)`

## Определение типа прибора

Старый код не сравнивает сигнатуру напрямую. Он делает:

```text
if ext in [.bin, .biv]: pre = 0
if ext == .DEV: pre = 11

seek(pre)
read uint32_t sign
buff = (sign << 12) >> 12
signature_for_pallete = buff / 1000
```

Коды приборов:

- `241` → `LWD_4Tx`
- `151` → `AUTONOM_5Tx`
- `152` → `AUTONOM_5Tx_SDR`

### Важное правило

В новой версии обязательно нужно повторить:

- чтение сигнатуры с корректного смещения
- нормализацию `((sign << 12) >> 12) / 1000`

Иначе тип прибора может определяться неверно.

## Поддержка `.biv`

В старом коде `.biv` идёт в тех же условиях, что и `.bin`:

- одинаковое чтение сигнатуры
- одинаковый расчёт размера кадра
- одинаковое чтение полезной структуры

### Практическое правило переноса

В новой версии:

- `.biv` должен использовать **тот же reader**, что и `.bin`
- отдельная логика не нужна

---

# 1. Автономный прибор (`AUTONOM_5Tx`, `AUTONOM_5Tx_SDR`)

## `.bin`

Файл содержит последовательность `_5Tx_DATA`.

```text
offset = n * sizeof(_5Tx_DATA)
read sizeof(_5Tx_DATA)
```

## `.biv`

Идентичен `.bin`.

```text
offset = n * sizeof(_5Tx_DATA)
read sizeof(_5Tx_DATA)
```

## `.DEV`

Файл содержит последовательность блоков вида:

```text
[11 bytes service] + [_5Tx_DATA]
```

Чтение:

```text
offset = n * (11 + sizeof(_5Tx_DATA))
skip 11
read sizeof(_5Tx_DATA)
```

## Как переносить в новую версию

Для `Автономный` можно использовать прямой путь:

- `.bin/.biv` → читать `_5Tx_DATA` напрямую
- `.DEV` → пропускать `11` байт, затем читать `_5Tx_DATA`

### Вывод

Для `Автономный` новый reader почти тривиален. Никакой дополнительной адаптации структуры не требуется.

---

# 2. LWD (`LWD_4Tx`)

## Исходная старая структура

Старый код хранит `LWD` в `_4Tx_DATA`:

```cpp
struct _4Tx_DATA {
    uint32_t signature;
    uint32_t condition;
    float rho_smt[2][4];
    float phase_smt[2][4];
    uint32_t frame;
    float temperature;
    float AM_RX_1[2][4];
    float ZERO_AM_RX_1[2];
    float AM_RX_2[2][4];
    float ZERO_AM_RX_2[2];
    float PH_RX_1[2][4];
    float ZERO_PH_RX_1[2];
    float PH_RX_2[2][4];
    float ZERO_PH_RX_2[2];
};
```

## `.bin`

```text
offset = n * sizeof(_4Tx_DATA)
read sizeof(_4Tx_DATA)
```

## `.biv`

Идентичен `.bin`.

```text
offset = n * sizeof(_4Tx_DATA)
read sizeof(_4Tx_DATA)
```

## `.DEV`

Физический блок:

```text
[11 bytes service] + [_4Tx_DATA]
```

Чтение:

```text
offset = n * (11 + sizeof(_4Tx_DATA))
skip 11
read sizeof(_4Tx_DATA)
```

## Как переносить в новую версию

Для `LWD` нужно сделать адаптер:

```text
read _4Tx_DATA -> mapTo5Tx(_4Tx_DATA) -> _5Tx_DATA
```

То есть в новой архитектуре полезно иметь отдельную функцию вида:

```cpp
_5Tx_DATA convertLwd4TxTo5Tx(const _4Tx_DATA& src);
```

---

# Правила адаптации `LWD_4Tx -> _5Tx_DATA`

## Поля верхнего уровня

Копируются напрямую:

- `signature`
- `condition`
- `frame`
- `temperature`
- `ZERO_AM_RX_1[2]`
- `ZERO_AM_RX_2[2]`

## Массивы `rho_smt`, `phase_smt`, `AM_RX_1`, `AM_RX_2`

Для каждой частоты `freq = 0..1`:

- `T1..T4` копируются напрямую
- `T5` заполняется заглушкой

Рекомендуемое правило:

```text
target[freq][0] = source[freq][0]
target[freq][1] = source[freq][1]
target[freq][2] = source[freq][2]
target[freq][3] = source[freq][3]
target[freq][4] = 0.0f
```

Это применяется к:

- `rho_smt`
- `phase_smt`
- `AM_RX_1`
- `AM_RX_2`

### Почему так

Потому что у `LWD_4Tx` физически нет `T5`. Если новая система требует единый формат `2 x 5`, то безопасный перенос — оставить `T5` пустым.

## Формирование `DELTA_PH` для `LWD`

В `_4Tx_DATA` поле `DELTA_PH` не хранится готовым. Старый код вычисляет его так:

```text
signal = PH_RX_2[freq][Tx] - PH_RX_1[freq][Tx]
```

Потом делает нормализацию:

- если `abs(signal) < 40`, значение сворачивается в диапазон `[-PI, PI]`
- иначе берётся `0`

Рекомендуемое правило переноса:

```pseudo
for freq in 0..1:
    for tx in 0..3:
        signal = src.PH_RX_2[freq][tx] - src.PH_RX_1[freq][tx]
        if abs(signal) < 40:
            while signal >= PI: signal -= 2*PI
            while signal <= -PI: signal += 2*PI
        else:
            signal = 0
        dst.DELTA_PH[freq][tx] = signal

    dst.DELTA_PH[freq][4] = 0
```

## Формирование `ZERO_dPH` для `LWD`

В старой структуре есть:

- `ZERO_PH_RX_1[2]`
- `ZERO_PH_RX_2[2]`

Поэтому для новой модели логично считать:

```pseudo
for freq in 0..1:
    zeroSignal = src.ZERO_PH_RX_2[freq] - src.ZERO_PH_RX_1[freq]
    if abs(zeroSignal) < 40:
        while zeroSignal >= PI: zeroSignal -= 2*PI
        while zeroSignal <= -PI: zeroSignal += 2*PI
    else:
        zeroSignal = 0
    dst.ZERO_dPH[freq] = zeroSignal
```

### Важное замечание

Это **разумный перенос в единую модель `_5Tx_DATA`**.

Старая функция `_4Tx_DATA -> GP_DATA` не прокидывала `ZERO_dPH` явно, поэтому если нужна максимально буквальная совместимость со старой визуализацией, можно временно делать:

```text
ZERO_dPH[freq] = 0
```

Но для новой архитектуры вариант с вычислением `ZERO_dPH` лучше.

---

# Практический pipeline для новой версии

## Общая схема

1. Определить расширение
2. По смещению прочитать сигнатуру
3. Определить тип прибора
4. Выбрать reader:
   - `AutonomReader`
   - `LwdReader`
5. Вернуть результат всегда в `_5Tx_DATA`

## Рекомендуемый псевдокод

```pseudo
ext = lower(fileExtension)

if ext in [".bin", ".biv"]:
    pre = 0
else if ext == ".dev":
    pre = 11
else:
    unsupported

seek(pre)
sign = readUInt32()
toolCode = (((sign << 12) >> 12) / 1000)

if toolCode in [151, 152]:
    if ext in [".bin", ".biv"]:
        raw = read<_5Tx_DATA>(n * sizeof(_5Tx_DATA))
    else:
        raw = read<_5Tx_DATA>(n * (11 + sizeof(_5Tx_DATA)) + 11)
    return raw

if toolCode == 241:
    if ext in [".bin", ".biv"]:
        raw4 = read<_4Tx_DATA>(n * sizeof(_4Tx_DATA))
    else:
        raw4 = read<_4Tx_DATA>(n * (11 + sizeof(_4Tx_DATA)) + 11)
    return convertLwd4TxTo5Tx(raw4)
```

---

# Что обязательно перенести

## Для `.biv`

- разрешить расширение `.biv`
- направить его в тот же путь, что и `.bin`

## Для `.DEV`

- читать сигнатуру с `offset = 11`
- для `Автономный` читать `_5Tx_DATA` после 11 байт
- для `LWD` читать `_4Tx_DATA` после 11 байт и преобразовывать в `_5Tx_DATA`

---

# На что обратить внимание

## 1. Регистр расширения

Старый код сравнивает `.DEV` в верхнем регистре. В новой версии лучше сделать:

- case-insensitive сравнение
- или сразу приводить расширение к lowercase

## 2. Размер кадров у `AUTONOM_5Tx`

В старом коде есть подозрительный момент: в одном месте размер кадра для `AUTONOM_5Tx` считается через `sizeof(_4Tx_DATA)`, хотя читается `_5Tx_DATA`.

Это выглядит как баг старой реализации.

Для новой версии безопаснее использовать:

- для `AUTONOM_5Tx` → `sizeof(_5Tx_DATA)`

## 3. T5 у `LWD`

Нужно явно договориться, что в единой модели:

- `T1..T4` — реальные каналы LWD
- `T5` — пустой канал-заглушка

Лучший минимум:

- заполнять `T5 = 0`
- при желании в UI не отображать `T5` для `LWD`

---

# Краткий итог

## `AUTONOM_5Tx`

- `.bin` = прямое чтение `_5Tx_DATA`
- `.biv` = то же самое, что `.bin`
- `.DEV` = `11 байт служебных + _5Tx_DATA`

## `LWD_4Tx`

- `.bin` = прямое чтение `_4Tx_DATA`
- `.biv` = то же самое, что `.bin`
- `.DEV` = `11 байт служебных + _4Tx_DATA`
- после чтения нужен адаптер `_4Tx_DATA -> _5Tx_DATA`

## Главное правило переноса

Новая версия должна возвращать `_5Tx_DATA` всегда, но путь подготовки данных зависит от типа прибора:

- `Автономный` → прямой reader
- `LWD` → reader + адаптер
