# Документация логики метрологии TEST_SONDE_KARTOGRAF_2.0

## Обзор

Форма метрологии (`MetroForm.h` / `MyForm1` в namespace `AIRTEMP10`) обеспечивает редактирование и обмен калибровочными данными с индукционными зондами. Основные кнопки взаимодействия с прибором:

| Кнопка | Обработчик | Назначение |
|--------|------------|------------|
| **Записать в зонд** | `buttonWrite_to_sonde_Click` | Запись файла метрологии в EEPROM прибора |
| **Записать команду** | `button_set_out_Click` | Запись настроек телесистемы (select_key) в EEPROM |
| **Записать в зонд** (пресеты) | `button_set_of_preset_to_sonde_Click` | Запись пресетов (SET_OF_PRESETS) в прибор |
| **Прочитать из зонда** | `buttonRead_from_sonde_Click` | Чтение метрологии из прибора |
| **Прочитать из зонда** (пресеты) | `button_set_of_preset_from_sonde_Click` | Чтение пресетов из прибора |

---

## 1. Кнопка «Записать в зонд» (метрология)

### Расположение
- **Файл:** `AIR_TEMP_1.0/MetroForm.h`
- **Кнопка:** `button5`
- **Обработчик:** `buttonWrite_to_sonde_Click`

### Алгоритм работы

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ 1. Пользователь нажимает «Записать в зонд»                                    │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ 2. MessageBox: "УВЕРЕНЫ?" (OK/Cancel)                                        │
│    - Cancel → выход без действий                                              │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │ OK
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ 3. Проверка sonde_on_off (выбран ли COM-порт)                                 │
│    - false → MessageBox "Выберите порт!"                                      │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │ true
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ 4. fill_the_struct() — сбор структуры метрологии из полей формы               │
│    Результат: variant<METROLOGY_CARTOGRAPH, METROLOGY_4Tx, METROLOGY_5Tx>    │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ 5. metro_check = true (блокировка параллельных операций)                     │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ 6. Ожидание: while (busy_read_from_sonde == true) { ; }                       │
│    Sleep(2000)                                                                │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ 7. serialPort1->PortName = Com_Name (из Conf.txt)                             │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ 8. write_metro_to_tool(serialPort1, current_metro_struct, &write_status)     │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ 9. metro_check = false                                                       │
│    MessageBox: "ФАЙЛ МЕТРОЛОГИИ ЗАПИСАН В ПРИБОР!" / "НЕ БЫЛ ЗАПИСАН"        │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Функция `write_metro_to_tool` (variable.cpp)

Логика зависит от типа зонда (`sonde_type`):

#### CARTOGRAPH (351) и AUTONOM_5Tx (151)

1. `COM->Open()` — BaudRate 125000, таймауты 2000 ms  
2. Определение размера структуры через `std::visit`:
   - `METROLOGY_CARTOGRAPH` → `sizeof(METROLOGY_CARTOGRAPH)`
   - `METROLOGY_5Tx` → `sizeof(METROLOGY_5Tx)`
3. `memcpy(DATA_OUT, &current_metro_struct, size)`
4. `send_to_tool(COM, CMD_SET_METROLOGY, DATA_OUT, size)` — команда `0x12` (0x02 | KEY)
5. `Sleep(500)`
6. `receive_from_tool(COM, CMD, &D0, &income_length)` — ожидание ответа
7. `write_ok = true`, если `receive_from_tool` вернул 0
8. `COM->Close()`

#### LWD_4Tx (241)

1. Используется низкоуровневый протокол: `send_to_tool_` / `receive_from_tool_`
2. Формирование пакета:
   - `DATA_OUT[0] = 0x00`, `DATA_OUT[1] = 0x04` — адрес EEPROM
   - `memcpy(DATA_OUT + 2, &current_metro_struct, 240)` — 240 байт метрологии
3. `send_to_tool_(COM, CMD_WRITE_EE, DATA_OUT, 242)` — команда `0x66`
4. `Sleep(150)`
5. `receive_from_tool_(COM, CMD, &DATA_IN, 0)` — подтверждение
6. `COM->Close()`

---

## 2. Кнопка «Записать команду»

### Расположение
- **Файл:** `AIR_TEMP_1.0/MetroForm.h`
- **Кнопка:** `button_set_out`
- **Обработчик:** `button_set_out_Click`

### Назначение
Запись в прибор настроек телесистемы — битовой маски `select_key`, которая определяет:
- какие каналы (УЭС, фазы, амплитуды и т.д.) передавать в телесистему;
- включение частот 400 кГц и 2000 кГц.

### Формирование select_key

```cpp
uint32_t select_key = 0x00;

// Битовая маска по 24 CheckBox (checkBox0..checkBox23)
for (int n = 0; n < 24; n++) {
    CheckBox^ CHECKBOX_CURVE = tableLayoutPanel_compress->Controls["checkBox" + n];
    if (CHECKBOX_CURVE->Checked == true)
        select_key |= (1 << n);
}

// checkBox24 → бит Work_400 (24) — работа на 400 кГц
if (checkBox24->Checked == true)
    select_key |= (1 << Work_400);  // 1 << 24

// checkBox25 → бит Work_2000 (25) — работа на 2000 кГц
if (checkBox25->Checked == true)
    select_key |= (1 << Work_2000); // 1 << 25
```

### Алгоритм работы

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ 1. Формирование select_key из состояния 26 чекбоксов                        │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ 2. Проверка sonde_on_off                                                     │
│    - false → MessageBox "Выберите порт!"                                     │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │ true
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ 3. metro_check = true                                                        │
│    while (busy_read_from_sonde) { ; }                                        │
│    Sleep(2000)                                                                │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ 4. serialPort1->Open() — PortName, BaudRate, таймауты                         │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ 5. memcpy(DATA_OUT, &select_key, sizeof(uint32_t))                           │
│    send_to_tool(COM, CMD_SET_TELESYSTEM, DATA_OUT, sizeof(uint32_t))          │
│    Команда: 0x17 (0x07 | KEY)                                                │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┘
│ 6. Sleep(150)                                                                │
│    receive_from_tool(COM, CMD, &D0, &readed_length)                           │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ 7. metro_check = false, COM->Close()                                         │
│    MessageBox: "НАСТРОЙКИ ЗАПИСАНЫ В ПРИБОР!" / "НЕ БЫЛИ ЗАПИСАНЫ"          │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 3. Кнопка «Записать в зонд» (пресеты)

### Расположение
- **Кнопка:** `button_set_of_preset_to_sonde`
- **Обработчик:** `button_set_of_preset_to_sonde_Click`

### Назначение
Запись структуры `SET_OF_PRESETS` в прибор: порядок передатчиков, минимальная скорость вращения, дата/время, режим отладки.

### Структура SET_OF_PRESETS (variable.h)

```cpp
struct SET_OF_PRESETS {
    uint16_t Signature = 0x5AA5;      // Маркер занятого места
    uint16_t TBTIndex;                // Индекс Tx_by_turn
    uint16_t MinRotSpeed;             // Мин. скорость для направленных измерений
    uint8_t  DebugSession;            // Режим отладки
    uint8_t  RotSpeed;                // Скорость при эмуляции
    uint64_t DateTime;                // Unix timestamp
};
```

### Алгоритм

1. Сбор данных из формы:
   - `TBTIndex` ← `comboBoxTBTindex->SelectedIndex`
   - `MinRotSpeed` ← `textBox_MinRotSpeed`
   - `DateTime` ← `Timestamp` (текущее время)
   - `DebugSession` ← `checkBox_debug_session->Checked`
   - `RotSpeed` ← `textBox_rotation` (или 0)
2. `Signature = 0x5AA5`
3. Ожидание освобождения `busy_read_from_sonde`, `Sleep(2000)`
4. Открытие COM-порта
5. `memcpy(DATA_OUT, &set_of_presets, sizeof(SET_OF_PRESETS))`
6. `send_to_tool(COM, CMD_SET_PRESETS, DATA_OUT, sizeof(SET_OF_PRESETS))` — команда `0x14` (0x04 | KEY)
7. `Sleep(150)`, `receive_from_tool(...)`
8. Закрытие порта, сообщение пользователю

---

## 4. Протокол обмена send_to_tool / receive_from_tool

### Формат пакета (send_to_tool)

| Байт  | Содержимое   | Описание                          |
|-------|---------------|-----------------------------------|
| 0     | CMD           | Код команды                       |
| 1–2   | length        | Длина пакета (little-endian)      |
| 3..   | DATA          | Полезные данные                   |
| N-2,N-1 | CRC16       | Контрольная сумма (CRC16)         |

Общая длина: `length + 5` байт.

### CRC16
- Алгоритм: табличный CRC16 (variable.cpp, функция `CRC16`)
- Область расчёта: байты 0..(length+2), т.е. CMD + length + DATA

### receive_from_tool
1. Чтение 3 байт: CMD, длина
2. Вычисление `rest_length = length - 3`
3. Чтение оставшихся байт (`Sleep(300)` перед чтением)
4. Проверка CRC16
5. Копирование данных в `DATA`, возврат `readed_length`

---

## 5. Команды протокола (variable.h)

| Константа         | Значение    | Описание                          |
|-------------------|------------|-----------------------------------|
| KEY               | 0x10       | Базовый ключ команд               |
| CMD_GET_DATA      | 0x11       | Получить кадр измерений           |
| CMD_GET_METROLOGY | 0x13       | Чтение метрологии                 |
| CMD_SET_METROLOGY | 0x12       | Запись метрологии                 |
| CMD_GET_SIGNATURE | 0x16       | Чтение сигнатуры                  |
| CMD_GET_PRESETS   | 0x15       | Чтение пресетов                   |
| CMD_SET_PRESETS   | 0x14       | Запись пресетов                   |
| CMD_SET_TELESYSTEM| 0x17       | Запись настроек телесистемы      |
| CMD_READ_EE       | 0x65       | Чтение EEPROM (LWD_4Tx)           |
| CMD_WRITE_EE      | 0x66       | Запись EEPROM (LWD_4Tx)           |

---

## 6. Функция fill_the_struct()

Собирает структуру метрологии из полей формы. Тип структуры зависит от `sonde_type`:

- **CARTOGRAPH (351):** `METROLOGY_CARTOGRAPH` — L1/L2, Air_zz, inc_set, turn_presets, reap, select_key, work_type и т.д.
- **LWD_4Tx (241):** `METROLOGY_4Tx` — L1/L2, F, Air_zz, PhaseOffsets
- **AUTONOM_5Tx (151):** `METROLOGY_5Tx` — L1/L2, F, Air_zz для 5 передатчиков

Источники данных:
- Текстовые поля `textBoxL1T1`..`textBoxL2T5`, `textBoxFT1`, `textBoxFT2`
- `textBoxAIRT1`..`textBoxAIRT10`, `textBox_min1`..`textBox_min10`
- `comboBox_type`, `comboBox_N_Tx`, `comboBox_mod`, `textBox_number`, `textBoxSer`
- Панель инклинометра (MA, MLD, N, K, W_MIN, W_MAX и т.д.)
- Панель пресетов (`textBoxT1`..`textBoxT7`, `textBoxT1S1`..)
- CheckBox'ы сжатия (checkBox0..checkBox25)

---

## 7. Связанные кнопки

### «Записать воздух» (button_set_air)
- Обнуляет значения Air_zz в форме
- Вызывает `button5->PerformClick()` — то есть запись метрологии в зонд

### «Прочитать из зонда» (buttonRead_from_sonde)
- Вызывает `read_metro_from_tool(serialPort1, sonde_type, &read_status)`
- При успехе: `fill_the_form(current_metrology_struct, sonde_type)` и `transform_metro()` → `metrology_GP_struct`

---

## 8. Сводная таблица операций записи

| Операция           | Команда            | Данные                | Размер       |
|--------------------|--------------------|-----------------------|--------------|
| Метрология         | CMD_SET_METROLOGY  | METROLOGY_*           | sizeof(...)  |
| Пресеты            | CMD_SET_PRESETS    | SET_OF_PRESETS        | ~16 байт     |
| Телесистема        | CMD_SET_TELESYSTEM | uint32_t select_key   | 4 байта      |
| LWD EEPROM         | CMD_WRITE_EE       | addr(2) + data(240)   | 242 байта    |

---

*Документ подготовлен по состоянию кода в TEST_SONDE_KARTOGRAF_2.0. Файлы: MetroForm.h, variable.cpp, variable.h.*
