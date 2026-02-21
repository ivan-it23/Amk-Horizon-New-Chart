# Документация: работа с файлами метрологии

## 1. Общие сведения

Файлы метрологии хранят калибровочные и конфигурационные данные индукционных зондов. Формат — бинарный (`.bin`). Структура файла зависит от типа зонда.

### Поддерживаемые типы зондов

| Код | Константа | Тип |
|-----|-----------|-----|
| 351 | CARTOGRAPH_ | Картограф (4Tx) |
| 359 | CARTOGRAPH_LWD_4Tx_ | Картограф LWD 4Tx |
| 241 | LWD_4Tx_ | LWD 4Tx |
| 151 | AUTONOM_5Tx_ | Автоном 5Tx |
| 152 | AUTONOM_5Tx_SDR_ | Автоном 5Tx SDR |

---

## 2. Алгоритм открытия файла метрологии (ОТКРЫТЬ_ФАЙЛ)

### 2.1. Последовательность действий

1. **Диалог выбора файла**  
   Пользователь нажимает кнопку «ОТКРЫТЬ_ФАЙЛ». Открывается `OpenFileDialog` с фильтром `*.bin`. Выбранный путь сохраняется в глобальную переменную `Metrology`, заголовок окна обновляется.

2. **Определение типа зонда**  
   Программа читает первые 4 байта файла (поле `signature`):

   ```cpp
   metro_in.open(Metrology, ios::binary);
   metro_in.read((char*)&first_read_signature, sizeof(first_read_signature));
   ```

   Из `signature` извлекается тип зонда:

   ```
   buff = (first_read_signature << 12) >> 12   // обнуление 12 старших бит
   sonde_type = buff / 1000
   ```

   Старшие 12 бит `signature` отбрасываются; младшие разряды кодируют тип, число передатчиков, модификацию и номер.

3. **Настройка интерфейса**  
   Вызывается `sells_load(sonde_type)`: для CARTOGRAPH показываются панели инклинометра и пресетов, для LWD_4Tx и AUTONOM_5Tx — скрываются.

4. **Чтение структуры метрологии**  
   Файл открывается повторно и читается соответствующая структура:

   - **CARTOGRAPH / CARTOGRAPH_LWD_4Tx**: `METROLOGY_CARTOGRAPH`
   - **LWD_4Tx**: `METROLOGY_4Tx`
   - **AUTONOM_5Tx / AUTONOM_5Tx_SDR**: `METROLOGY_5Tx`

   ```cpp
   metro_in.read((char*)&current_metro_struct, sizeof(current_metro_struct));
   ```

5. **Преобразование и заполнение формы**  
   - `transform_metro()` приводит данные к общему виду `METROLOGY_GP`
   - `fill_the_form()` заполняет элементы интерфейса значениями из структуры

6. **Завершение**  
   Устанавливается `metro_is_readed_from_sonde = true`.

---

## 3. Алгоритм сохранения файла метрологии (ЗАПИСАТЬ_В_ФАЙЛ)

### 3.1. Последовательность действий

1. **Сбор данных из формы**  
   Вызывается `fill_the_struct()`, которая читает значения из полей и формирует структуру `METROLOGY_CARTOGRAPH`, `METROLOGY_4Tx` или `METROLOGY_5Tx` в зависимости от `sonde_type`.

2. **Диалог сохранения**  
   Открывается `SaveFileDialog` с фильтром `*.bin`. Пользователь выбирает путь сохранения.

3. **Запись в файл**  
   ```cpp
   metro_out.open(Metrology_out, ios::binary);
   metro_out.write((char*)&current_metro_struct, sizeof(current_metro_struct));
   metro_out.close();
   ```

   Записывается вся структура метрологии в бинарном виде.

---

## 4. Описание полей интерфейса и их связь с данными

### 4.1. Блок идентификации (нижняя часть формы)

#### Поле «ТИП» (comboBox_type)

- **Источник**: младшие 12 бит `signature`
- **Формула при чтении**:
  ```
  buff = (signature << 12) >> 12
  tool_type = buff / 100000
  comboBox_type->SelectedIndex = tool_type - 1
  ```
- **Значения**: 0 — АВТОНОМ, 1 — LWD, 2 — КАРТОГРАФ
- **При записи**: `tool_type = comboBox_type->SelectedIndex + 1`, участвует в формировании `signature`

---

#### Поле «N_Tx» (comboBox_N_Tx)

- **Источник**: `signature`
- **Формула при чтении**:
  ```
  tool_N_Tx = (buff % 100000) / 10000
  comboBox_N_Tx->SelectedIndex = tool_N_Tx - 3
  ```
- **Значения**: 0 — 3 Tx, 1 — 4 Tx, 2 — 5 Tx
- **При записи**: `tool_N_Tx = comboBox_N_Tx->SelectedIndex + 3`

---

#### Поле «МОД» (comboBox_mod)

- **Источник**: `signature`
- **Формула при чтении**:
  ```
  tool_mod = (buff % 10000) / 1000
  comboBox_mod->SelectedIndex = tool_mod - 1
  ```
- **При записи**: `tool_mod = comboBox_mod->SelectedIndex + 1`

---

#### Поле «номер» (textBox_number)

- **Источник**: `signature`
- **Формула при чтении**:
  ```
  tool_number = buff % 1000
  textBox_number->Text = tool_number.ToString()
  ```
- **При записи**: `tool_number = Int64::Parse(textBox_number->Text)`, входит в младшие разряды `signature`

---

#### Поле «Серийный на кожухе» (textBoxSer)

- **Источник**: `serial` структуры метрологии
- **При чтении**: `textBoxSer->Text = metro.serial.ToString()`
- **При записи**: `metro.serial = Int32::Parse(textBoxSer->Text)`
- **Тип**: `uint32_t` (все типы зондов)

---

#### Поле «signature32 разр. бинарный» (textBox_sign32)

- **Источник**: `signature` (32 бита)
- **При чтении**:
  ```cpp
  std::stringstream ss;
  ss << std::bitset<32>(metro.signature);
  textBox_sign32->Text = ss.str()
  ```
- **Формат**: строка из 32 символов «0» и «1»
- **При записи**: не редактируется напрямую; формируется из ТИП, N_Tx, МОД, номер

---

#### Поле «signature for pallete» (textBox_sign_for_pallete)

- **Источник**: `signature`
- **Формула при чтении**:
  ```
  buff = (signature << 12) >> 12
  signature_for_pallete = buff / 1000
  textBox_sign_for_pallete->Text = signature_for_pallete.ToString()
  ```
- **Назначение**: идентификатор «палитры» (тип + модификация) для выбора конфигурации
- **При записи**: вычисляется из `signature`

---

### 4.2. Геометрия зонда

#### Поля L1_mm (textBoxL1T1 … textBoxL1T5)

- **Источник**: `L1[Tx]` — расстояние до первого приёмника (мм)
- **При чтении**: `TEXTBOX_L1->Text = metro.L1[Tx].ToString()`
- **При записи**: `metro.L1[Tx] = Int64::Parse(TEXTBOX_L1->Text)`
- **Типы**:
  - CARTOGRAPH: `uint32_t L1[4]` (T1–T4)
  - LWD_4Tx: `uint16_t L1[8]` (T1–T4, дублируются в L1[4]–L1[7])
  - AUTONOM_5Tx: `uint16_t L1[5]` (T1–T5)

---

#### Поля L2_mm (textBoxL2T1 … textBoxL2T5)

- **Источник**: `L2[Tx]` — расстояние до второго приёмника (мм)
- **При чтении**: `TEXTBOX_L2->Text = metro.L2[Tx].ToString()`
- **При записи**: `metro.L2[Tx] = Int64::Parse(TEXTBOX_L2->Text)`
- **Типы**: аналогично `L1`

---

### 4.3. Частоты

#### Поля F_1_kHz, F_2_kHz (textBoxFT1, textBoxFT2)

- **Источник**: `F[freq]` — частоты в кГц (обычно 400 и 2000)
- **При чтении**: `TEXTBOX_F->Text = metro.F[freq].ToString()`
- **При записи**: `metro.F[freq] = Int64::Parse(TEXTBOX_F->Text)`
- **Типы**:
  - CARTOGRAPH, 5Tx: `uint32_t F[2]` или `uint16_t F[2]`
  - LWD_4Tx: `uint16_t F[8]` (F[0], F[4] для двух частот)

---

### 4.4. Калибровка «воздуха» (ненаправленные измерения)

#### Поля AIR_X_F_Y (textBoxAIRT1 … textBoxAIRT10)

- **Источник**: `Air_zz[freq][Tx]` — калибровочные значения в воздухе
- **Сопоставление**:
  - textBoxAIRT1–5: частота 400 кГц, Tx 1–5
  - textBoxAIRT6–10: частота 2000 кГц, Tx 1–5
- **При чтении**:
  - Tx_air < 5: `metro.Air_zz[_400_kGz][Tx_air]`
  - Tx_air ≥ 5: `metro.Air_zz[_2000_kGz][Tx_air - 5]`
- **При записи**: обратное отображение в `Air_zz`
- **Тип**: `int32_t` (CARTOGRAPH) или `int16_t` (4Tx/5Tx)

---

#### Поля MIN_R_T_F_Y (textBox_min1 … textBox_min10)

- **Источник**: `Air_zz_amt[freq][Tx]` — минимальная амплитуда для работоспособности передатчика
- **Сопоставление**: как у AIR_X_F_Y
- **При чтении**: `TEXTBOX_MIN->Text = metro.Air_zz_amt[freq][Tx].ToString()`
- **При записи**: `metro.Air_zz_amt[freq][Tx] = Int64::Parse(TEXTBOX_MIN->Text)`
- **Тип**: `int32_t` (CARTOGRAPH) или `int16_t` (4Tx/5Tx)

---

### 4.5. Чекбоксы WORK, GAL, GAH, GPL, GPH, RoL, RoH

- **Источник**: `select_key` (24 младших бита — каналы, биты 24 и 25 — режимы частот)
- **При чтении**:
  ```
  bit = (metro.select_key >> n) & 1
  checkBox->Checked = (bit == 1)
  ```
  - `n = 0..23`: каналы (GAL, GAH, GPL, GPH, RoL, RoH)
  - `n = 24` (Work_400): WORK 400
  - `n = 25` (Work_2000): WORK 2000
- **При записи**:
  ```
  if (checkBox->Checked) metro.select_key |= (1 << n)
  ```

---

### 4.6. Флаг L_F_read_only (checkBox_read_only)

- **Назначение**: запрет редактирования (по умолчанию включён)
- **При записи**: не влияет на содержимое файла; при `Checked == true` можно блокировать кнопку «ЗАПИСАТЬ_В_ФАЙЛ» в UI

---

## 5. Структуры данных (variable.h)

### 5.1. METROLOGY_5Tx (AUTONOM 5Tx)

```cpp
struct METROLOGY_5Tx {
    uint32_t signature;      // 4 байта
    uint32_t serial;        // 4 байта
    uint16_t L1[5];         // 10 байт
    uint16_t L2[5];         // 10 байт
    uint16_t F[2];          // 4 байта
    int16_t Air_zz[2][5];   // 20 байт
    int16_t Air_zz_amt[2][5]; // 20 байт
    uint32_t D_sonde_mm;    // 4 байта
    uint8_t service[164];   // 164 байта
};
// #pragma pack(push, 1) — выравнивание 1 байт
```

### 5.2. METROLOGY_4Tx (LWD 4Tx)

```cpp
struct METROLOGY_4Tx {
    uint32_t signature;
    uint32_t serial;
    uint16_t L1[8];         // дублирование T1–T4 в [4]–[7]
    uint16_t L2[8];
    uint16_t F[8];          // F[0], F[4] — частоты
    int16_t Air_zz[8];      // Tx_air 0–3: 400 кГц, 4–7: 2000 кГц (смещение -1)
    uint16_t PhaseOffsets[4];
    uint32_t D_sonde_mm;
    int16_t Air_zz_amt[8];
    uint8_t service[140];
};
```

### 5.3. METROLOGY_CARTOGRAPH

```cpp
struct METROLOGY_CARTOGRAPH {
    uint32_t signature;
    uint32_t serial;
    uint32_t L1[4];
    uint32_t L2[4];
    uint32_t L_geo[5];
    uint32_t F[2];
    int32_t Air_zz[2][5];
    int32_t Air_zz_amt[2][5];
    INC_SET inc_set;
    TURN_PRESETS turn_presets;
    uint32_t D_sonde_mm;
    uint64_t unix_data;
    REAP_CONST reap;
    uint32_t select_key;
    uint32_t work_type;
    uint32_t service[61];
};
```

---

## 6. Формула кодирования signature

### 6.1. Декодирование (чтение)

```
buff = (signature << 12) >> 12   // младшие 20 бит
tool_type    = buff / 100000     // 1=АВТОНОМ, 2=LWD, 3=КАРТОГРАФ
tool_N_Tx    = (buff % 100000) / 10000   // 3, 4 или 5
tool_mod     = (buff % 10000) / 1000     // модификация 1–9
tool_number  = buff % 1000               // номер 0–999
signature_for_pallete = buff / 1000      // тип+модификация
```

### 6.2. Кодирование (запись)

```
buff1 = (comboBox_type->SelectedIndex + 1) * 100000 +
        (comboBox_N_Tx->SelectedIndex + 3) * 10000 +
        (comboBox_mod->SelectedIndex + 1) * 1000 +
        tool_number
buff = (metro.signature >> 20) << 20   // сохранить старшие биты
metro.signature = buff | buff1
```

---

## 7. Порядок байт и выравнивание

- Используется `#pragma pack(push, 1)` для структур метрологии
- Порядок байт: little-endian (типично для x86/x64)
- Файл — последовательная запись полей структуры без заголовков

---

## 8. Обработка ошибок

- При открытии: проверка формата по `signature`; при неверном формате — сообщение «сигнатура старого образца»
- При сохранении: при отмене диалога — «Не удалось записать файл»
- Пустые поля при записи не изменяют соответствующие элементы структуры (проверка `Text != ""`)

---

## 9. Соответствие полей UI и структур (сводная таблица)

| Поле UI | Элемент | Структура | Компонент |
|---------|---------|-----------|-----------|
| T1_L1_mm … T5_L1_mm | textBoxL1T1–5 | L1[Tx] | Расстояние до приёмника 1, мм |
| T1_L2_mm … T5_L2_mm | textBoxL2T1–5 | L2[Tx] | Расстояние до приёмника 2, мм |
| F_1_kHz, F_2_kHz | textBoxFT1, textBoxFT2 | F[0], F[1] | Частоты, кГц |
| AIR_1_F_1 … AIR_5_F_2 | textBoxAIRT1–10 | Air_zz[freq][Tx] | Калибровка «воздух» |
| MIN_R2_T1_F_1 … | textBox_min1–10 | Air_zz_amt[freq][Tx] | Мин. амплитуда |
| ТИП | comboBox_type | signature (tool_type) | Тип зонда |
| N_Tx | comboBox_N_Tx | signature (tool_N_Tx) | Число передатчиков |
| МОД | comboBox_mod | signature (tool_mod) | Модификация |
| номер | textBox_number | signature (tool_number) | Номер |
| Серийный на кожухе | textBoxSer | serial | Серийный номер |
| signature32 разр. бинарный | textBox_sign32 | signature | 32 бита в двоичном виде |
| signature for pallete | textBox_sign_for_pallete | signature | buff/1000 |
