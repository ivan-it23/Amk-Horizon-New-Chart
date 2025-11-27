# Полный анализ структуры данных GP_TOOL_DATA

## 1. ОПРЕДЕЛЕНИЕ СТРУКТУРЫ

```cpp
struct GP_TOOL_DATA {
    uint32_t signature;      // Сигнатура для идентификации данных
    uint32_t condition;      // Состояние измерения (контроль качества)
    float Ro_smt[8];        // Симметризованное удельное сопротивление
    float PH_smt[8];        // Симметризованная разность фаз
    uint32_t frame;         // Номер кадра
    float temperature;      // Температура
    float AM_RX_1[10];      // Амплитуды приемника RX1
    float AM_RX_2[10];      // Амплитуды приемника RX2
    float PH_RX_1[10];      // Фазы приемника RX1
    float PH_RX_2[10];      // Фазы приемника RX2
};
```

**Размер структуры:** 240 байт

---

## 2. ИНДЕКСАЦИЯ МАССИВОВ ДАННЫХ

### 2.1 Индексы для частот и передатчиков

Массивы `AM_RX_1`, `AM_RX_2`, `PH_RX_1`, `PH_RX_2` содержат 10 элементов:

**Индексы 0-3:** Данные на частоте **400 Гц** для передатчиков TX1-TX4
**Индексы 4-7:** Данные на частоте **2000 Гц** для передатчиков TX1-TX4
**Индексы 8-9:** Данные для нулевого передатчика (TX0):
- Индекс 8: TX0 на 400 Гц
- Индекс 9: TX0 на 2000 Гц

### 2.2 Перечисления (enum) для индексации

```cpp
enum TX {
    TX0_400,   // индекс 0 (но в массивах это индекс 8)
    TX1_400,   // индекс 0 в массиве
    TX2_400,   // индекс 1 в массиве
    TX3_400,   // индекс 2 в массиве
    TX4_400,   // индекс 3 в массиве
    TX0_2000,  // индекс 5 (но в массивах это индекс 9)
    TX1_2000,  // индекс 4 в массиве
    TX2_2000,  // индекс 5 в массиве
    TX3_2000,  // индекс 6 в массиве
    TX4_2000   // индекс 7 в массиве
};
```

---

## 3. ЧТЕНИЕ ДАННЫХ ИЗ ФАЙЛА

### 3.1 Функции чтения

**Функция 1: READ_BLOK_AllData_1DDS** (для .bin файлов)
```cpp
GP_TOOL_DATA READ_BLOK_AllData_1DDS(ifstream &fin, int n) {
    GP_TOOL_DATA P;
    fin.seekg(n * sizeof(GP_TOOL_DATA));  // Переход к n-му кадру
    fin.read((char*)&P, sizeof(GP_TOOL_DATA));  // Чтение 240 байт
    return P;
}
```

**Функция 2: READ_BLOK_AllData_1DDS_Oleg** (для .DEV файлов)
```cpp
GP_TOOL_DATA READ_BLOK_AllData_1DDS_Oleg(ifstream &fin, int n) {
    GP_TOOL_DATA P;
    uint8_t *buff = new uint8_t[12];
    fin.seekg(n * (sizeof(GP_TOOL_DATA) + 11));  // Пропуск 11 байт заголовка
    fin.read((char*)buff, 11);  // Чтение заголовка
    fin.read((char*)&P, sizeof(GP_TOOL_DATA));  // Чтение данных
    return P;
}
```

**Функция 3: READ_BLOK_IFN** (для .ifn файлов)
```cpp
GP_TOOL_DATA READ_BLOK_IFN(ifstream &fin, int n) {
    GP_TOOL_DATA P;
    fin.seekg(n * 512);  // Блоки по 512 байт
    fin.seekg(64, ios_base::cur);  // Пропуск 64 байт заголовка
    fin.read((char*)&P, sizeof(P));
    // Преобразование порядка байтов (byte swap)
    return P;
}
```

### 3.2 Процесс загрузки данных

```cpp
// В MyForm.h, функция открытия файла:
for (int n = 0; n < Co; n++) {  // Co - количество кадров
    DAT S;
    S.GP = READ_BLOK_AllData_1DDS(fin, n);  // Чтение n-го кадра
    DATA1.push_back(S);      // Сохранение в вектор
    DATA_MA.push_back(S);    // Для скользящего среднего
    NOICE.push_back(S);      // Для шума
    SKO.push_back(S);        // Для СКО
}
```

---

## 4. ОБРАБОТКА ДАННЫХ

### 4.1 Вычисление разности фаз

```cpp
// В функции do_chart():
float signal[10];
for (int i = 0; i < 8; i++) {
    // Разность фаз между приемниками RX2 и RX1
    signal[i] = (DATA1[n].GP.PH_RX_2[i] - DATA1[n].GP.PH_RX_1[i]);
    
    // Нормализация фазы в диапазон [-π, π]
    if (fabs(signal[i]) < 40) {
        while (signal[i] > PI) signal[i] -= 2 * PI;
        while (signal[i] <= -PI) signal[i] += 2 * PI;
    }
    else signal[i] = 0.0;
}
DATA1[n].dfi[i] = signal[i];  // Сохранение разности фаз
```

### 4.2 Скользящее среднее (Moving Average)

```cpp
uint32_t W = 20;  // Размер окна
DAT SUMM = {0.0};
vector<DAT> DATA_BOOF(W);  // Буфер для окна

for (int i = 0; i < 10; i++) {
    // Удаление старого значения из суммы
    SUMM.GP.AM_RX_1[i] -= DATA_BOOF[w].GP.AM_RX_1[i];
    SUMM.GP.AM_RX_2[i] -= DATA_BOOF[w].GP.AM_RX_2[i];
    
    // Добавление нового значения
    DATA_BOOF[w].GP.AM_RX_1[i] = DATA1[n].GP.AM_RX_1[i];
    DATA_BOOF[w].GP.AM_RX_2[i] = DATA1[n].GP.AM_RX_2[i];
    
    SUMM.GP.AM_RX_1[i] += DATA_BOOF[w].GP.AM_RX_1[i];
    SUMM.GP.AM_RX_2[i] += DATA_BOOF[w].GP.AM_RX_2[i];
    
    // Вычисление среднего
    DATA_MA[n].GP.AM_RX_1[i] = SUMM.GP.AM_RX_1[i] / W;
    DATA_MA[n].GP.AM_RX_2[i] = SUMM.GP.AM_RX_2[i] / W;
}
```

### 4.3 Вычисление шума

```cpp
// Шум = Исходное значение - Скользящее среднее
if (n < max_n - W) {
    NOICE[n].GP.AM_RX_1[i] = DATA1[n].GP.AM_RX_1[i] - DATA_MA[n + W/2].GP.AM_RX_1[i];
    NOICE[n].GP.AM_RX_2[i] = DATA1[n].GP.AM_RX_2[i] - DATA_MA[n + W/2].GP.AM_RX_2[i];
}
```

---

## 5. ОТОБРАЖЕНИЕ НА ГРАФИКАХ

### 5.1 Константы преобразования

```cpp
const float mV_1dds = 4000.0f / pow(2.0f, 24.0f);  // Преобразование в мВ
const float mG = 1000.0f * 180.0f / PI;            // Преобразование в миллиградусы
```

### 5.2 График: "PH Z2-Z1" (Разность фаз двух приемников)

**Частота 400 Гц:**
```cpp
int k = 0;  // Смещение для 400 Гц
for (int i = 0; i < N_Tx; i++) {
    float phase_diff = (signal[i+k] * mG) - Phase_shift_chart.Phase[i];
    CHART->Series[i]->Points->AddXY(N, phase_diff);
}
```

**Частота 2000 Гц:**
```cpp
int k = 4;  // Смещение для 2000 Гц
for (int i = 0; i < N_Tx; i++) {
    float phase_diff = (signal[i+k] * mG) - Phase_shift_chart.Phase[i];
    CHART->Series[i]->Points->AddXY(N, phase_diff);
}
```

**Данные:**
- Series[0]: TX1 (разность фаз RX2 - RX1)
- Series[1]: TX2
- Series[2]: TX3
- Series[3]: TX4

### 5.3 График: "AM Z2/Z1" (Отношение амплитуд двух приемников)

**Частота 400 Гц:**
```cpp
int k = 0;
for (int i = 0; i < N_Tx; i++) {
    float ratio = DATA1[n].GP.AM_RX_2[i+k] / (DATA1[n].GP.AM_RX_1[i+k] + 0.000001);
    CHART->Series[i]->Points->AddXY(N, ratio);
}
```

**Частота 2000 Гц:**
```cpp
int k = 4;
for (int i = 0; i < N_Tx; i++) {
    float ratio = DATA1[n].GP.AM_RX_2[i+k] / (DATA1[n].GP.AM_RX_1[i+k] + 0.000001);
    CHART->Series[i]->Points->AddXY(N, ratio);
}
```

**Данные:**
- Series[0]: TX1 (AM_RX_2 / AM_RX_1)
- Series[1]: TX2
- Series[2]: TX3
- Series[3]: TX4

### 5.4 График: "AM Z1" (Амплитуды первого приемника)

**Частота 400 Гц:**
```cpp
int k = 0;
for (int i = 0; i < N_Tx; i++) {
    float amplitude = DATA1[n].GP.AM_RX_1[i+k] * mV;
    CHART->Series[i]->Points->AddXY(N, amplitude);
}
```

**Частота 2000 Гц:**
```cpp
int k = 4;
for (int i = 0; i < N_Tx; i++) {
    float amplitude = DATA1[n].GP.AM_RX_1[i+k] * mV;
    CHART->Series[i]->Points->AddXY(N, amplitude);
}
```

**Данные:**
- Series[0]: TX1 (амплитуда RX1)
- Series[1]: TX2
- Series[2]: TX3
- Series[3]: TX4

### 5.5 График: "AM Z2" (Амплитуды второго приемника)

**Частота 400 Гц:**
```cpp
int k = 0;
for (int i = 0; i < N_Tx; i++) {
    float amplitude = DATA1[n].GP.AM_RX_2[i+k] * mV;
    CHART->Series[i]->Points->AddXY(N, amplitude);
}
```

**Частота 2000 Гц:**
```cpp
int k = 4;
for (int i = 0; i < N_Tx; i++) {
    float amplitude = DATA1[n].GP.AM_RX_2[i+k] * mV;
    CHART->Series[i]->Points->AddXY(N, amplitude);
}
```

**Данные:**
- Series[0]: TX1 (амплитуда RX2)
- Series[1]: TX2
- Series[2]: TX3
- Series[3]: TX4

### 5.6 График: "PH Z1" (Фазы первого приемника)

**Частота 400 Гц:**
```cpp
int k = 0;
for (int i = 0; i < N_Tx; i++) {
    float phase = DATA1[n].GP.PH_RX_1[i+k] * mG - Phase_shift_chart.Phase[i];
    CHART->Series[i]->Points->AddXY(N, phase);
}
```

**Частота 2000 Гц:**
```cpp
int k = 4;
for (int i = 0; i < N_Tx; i++) {
    float phase = DATA1[n].GP.PH_RX_1[i+k] * mG - Phase_shift_chart.Phase[i];
    CHART->Series[i]->Points->AddXY(N, phase);
}
```

**Данные:**
- Series[0]: TX1 (фаза RX1)
- Series[1]: TX2
- Series[2]: TX3
- Series[3]: TX4

### 5.7 График: "PH Z2" (Фазы второго приемника)

**Частота 400 Гц:**
```cpp
int k = 0;
for (int i = 0; i < N_Tx; i++) {
    float phase = DATA1[n].GP.PH_RX_2[i+k] * mG - Phase_shift_chart.Phase[i];
    CHART->Series[i]->Points->AddXY(N, phase);
}
```

**Частота 2000 Гц:**
```cpp
int k = 4;
for (int i = 0; i < N_Tx; i++) {
    float phase = DATA1[n].GP.PH_RX_2[i+k] * mG - Phase_shift_chart.Phase[i];
    CHART->Series[i]->Points->AddXY(N, phase);
}
```

**Данные:**
- Series[0]: TX1 (фаза RX2)
- Series[1]: TX2
- Series[2]: TX3
- Series[3]: TX4

### 5.8 График: "AM Zero ZZ" (Амплитуды нулевого передатчика)

**Частота 400 Гц:**
```cpp
int k = 8;  // Индекс TX0 на 400 Гц
CHART->Series[0]->Points->AddXY(N, DATA1[n].GP.AM_RX_1[k] * mV);
CHART->Series[1]->Points->AddXY(N, DATA1[n].GP.AM_RX_2[k] * mV);
```

**Частота 2000 Гц:**
```cpp
int k = 9;  // Индекс TX0 на 2000 Гц
CHART->Series[0]->Points->AddXY(N, DATA1[n].GP.AM_RX_1[k] * mV);
CHART->Series[1]->Points->AddXY(N, DATA1[n].GP.AM_RX_2[k] * mV);
```

**Данные:**
- Series[0]: RX1 (амплитуда TX0)
- Series[1]: RX2 (амплитуда TX0)

### 5.9 График: "PH Zero ZZ" (Фазы нулевого передатчика)

**Частота 400 Гц:**
```cpp
int k = 8;
CHART->Series[0]->Points->AddXY(N, DATA1[n].GP.PH_RX_1[k] * mG);
CHART->Series[1]->Points->AddXY(N, DATA1[n].GP.PH_RX_1[k] * mG);
```

**Частота 2000 Гц:**
```cpp
int k = 9;
CHART->Series[0]->Points->AddXY(N, DATA1[n].GP.PH_RX_1[k] * mG);
CHART->Series[1]->Points->AddXY(N, DATA1[n].GP.PH_RX_1[k] * mG);
```

---

## 6. ПЕРЕКЛЮЧЕНИЕ ЧАСТОТ

Переключение между частотами 400 Гц и 2000 Гц осуществляется через переменные:
- `index_freq_1` для первого графика (chart1)
- `index_freq_2` для второго графика (chart2)

```cpp
int k;
if (index_freq == 0) k = 0;    // 400 Гц: индексы 0-3
if (index_freq == 1) k = 4;    // 2000 Гц: индексы 4-7
```

Для нулевого передатчика:
```cpp
if (index_freq == 0) k = 8;    // TX0 на 400 Гц
if (index_freq == 1) k = 9;    // TX0 на 2000 Гц
```

---

## 7. СТРУКТУРА ДАННЫХ В ПАМЯТИ

```
DATA1[n].GP:
├── signature (4 байта)
├── condition (4 байта)
├── Ro_smt[8] (32 байта)
├── PH_smt[8] (32 байта)
├── frame (4 байта)
├── temperature (4 байта)
├── AM_RX_1[10] (40 байт)
│   ├── [0-3]: TX1-TX4 @ 400 Гц
│   ├── [4-7]: TX1-TX4 @ 2000 Гц
│   ├── [8]: TX0 @ 400 Гц
│   └── [9]: TX0 @ 2000 Гц
├── AM_RX_2[10] (40 байт)
│   └── (та же структура)
├── PH_RX_1[10] (40 байт)
│   └── (та же структура)
└── PH_RX_2[10] (40 байт)
    └── (та же структура)

Итого: 240 байт
```

---

## 8. ПОТОК ДАННЫХ

```
Файл (.bin/.DEV/.ifn)
    ↓
READ_BLOK_AllData_1DDS() / READ_BLOK_IFN()
    ↓
GP_TOOL_DATA структура
    ↓
DATA1[n].GP (вектор всех кадров)
    ↓
Обработка:
├── Вычисление разности фаз (signal[i])
├── Скользящее среднее (DATA_MA)
└── Вычисление шума (NOICE)
    ↓
Отображение на графиках (chart1, chart2)
├── Выбор частоты (index_freq: 0=400Гц, 1=2000Гц)
├── Выбор типа графика (comboBox)
└── Отрисовка Series[0-3] для TX1-TX4
```

---

## 9. КЛЮЧЕВЫЕ ОСОБЕННОСТИ

1. **Двухчастотная система:** Все измерения проводятся на двух частотах (400 Гц и 2000 Гц)

2. **Два приемника:** RX1 и RX2 для каждого передатчика

3. **Разность фаз:** Основной параметр - разность фаз между RX2 и RX1

4. **Отношение амплитуд:** Используется для анализа затухания сигнала

5. **Скользящее среднее:** Применяется для сглаживания данных (окно W=20)

6. **Фазовая коррекция:** Вычитание Phase_shift_chart.Phase[i] для калибровки

7. **Преобразование единиц:**
   - Амплитуды: умножение на mV (милливольты)
   - Фазы: умножение на mG (миллиградусы)

8. **Нормализация фаз:** Приведение к диапазону [-π, π]

---

## 10. РЕЗЮМЕ

Структура `GP_TOOL_DATA` содержит полный набор данных одного измерительного кадра:
- 4 передатчика (TX1-TX4) + 1 нулевой (TX0)
- 2 приемника (RX1, RX2)
- 2 частоты (400 Гц, 2000 Гц)
- Амплитуды и фазы для каждой комбинации
- Предварительно рассчитанные симметризованные значения (Ro_smt, PH_smt)

Данные считываются из бинарных файлов, обрабатываются (скользящее среднее, вычисление разности фаз), и отображаются на графиках в различных представлениях в зависимости от выбранного режима визуализации.
