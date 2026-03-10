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


---

# ЧАСТЬ 2: ЧТЕНИЕ ДАННЫХ С ПРИБОРА В РЕАЛЬНОМ ВРЕМЕНИ

## 11. ЭМУЛЯТОР ПРИБОРА (emul__1_.ino)

Эмулятор реализован на Arduino и имитирует работу реального прибора.

### 11.1 Структура данных эмулятора

```cpp
struct AllData_P {
    uint32_t signature;      // 0x31443454 ("1D4T")
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

**Структура идентична GP_TOOL_DATA!**

### 11.2 Инициализация данных

```cpp
void setup() {
    Serial.begin(125000);  // Скорость COM-порта: 125000 бод
    
    petr.signature = 0x31443453;  // Сигнатура "1D4S"
    petr.frame = 0x30;
    
    for(int i = 0; i < 8; i++){
        petr.PH_smt[i] = 0.01*i;
        petr.Ro_smt[i] = 1.1*i;
        petr.AM_RX_1[i] = 50+i;
        petr.AM_RX_2[i] = 60+i;
        petr.PH_RX_1[i] = 0.1;
        petr.PH_RX_2[i] = 0.2;
    }
}
```

### 11.3 Обработка команд

```cpp
void loop() {
    if (Serial.available() > 3) {
        Serial.readBytes(buff, 4);
        
        // Команда 0x67 (CMD_WORK) - запрос данных
        if(buff[0]==0x67 || buff[1]==0x67 || buff[2]==0x67 || buff[3]==0x67){
            // Генерация тестовых данных
            petr.signature = 0x31443454;  // "1D4T"
            
            for(int i = 0; i < 8; i++){
                petr.PH_smt[i] = 100/mG;
                petr.Ro_smt[i] = 10*(i+1);
                petr.AM_RX_1[i] = 100*(100+i + petr.frame);
                petr.AM_RX_2[i] = 100*(200+i + petr.frame);
                petr.PH_RX_1[i] = 0;
            }
            
            // Разность фаз для тестирования
            petr.PH_RX_2[0] = -100.0/mG;  // TX1 @ 400 Гц
            petr.PH_RX_2[1] = 100.0/mG;   // TX2 @ 400 Гц
            petr.PH_RX_2[2] = -100.0/mG;  // TX3 @ 400 Гц
            petr.PH_RX_2[3] = 100.0/mG;   // TX4 @ 400 Гц
            petr.PH_RX_2[4] = -200.0/mG;  // TX1 @ 2000 Гц
            petr.PH_RX_2[5] = 200.0/mG;   // TX2 @ 2000 Гц
            petr.PH_RX_2[6] = -200.0/mG;  // TX3 @ 2000 Гц
            petr.PH_RX_2[7] = 200.0/mG;   // TX4 @ 2000 Гц
            
            petr.frame++;  // Инкремент номера кадра
            
            // Отправка данных
            send_out(0x67, (char*)(&petr), sizeof(AllData_P));
        }
        
        // Команда 0x65 (CMD_READ_EE) - запрос метрологии
        if(buff[0]==0x65 || buff[1]==0x65 || buff[2]==0x65 || buff[3]==0x65){
            send_out_metro(0x65, (char*)(&METRO), sizeof(METROLOGY));
        }
    }
}
```

---

## 12. ПРОТОКОЛ ОБМЕНА ДАННЫМИ

### 12.1 Команды

| Код | Константа | Описание |
|-----|-----------|----------|
| 0x67 | CMD_WORK | Запрос данных измерений |
| 0x65 | CMD_READ_EE | Чтение EEPROM (метрология) |
| 0x66 | CMD_WRITE_EE | Запись EEPROM |
| 0x61 | CMD_ERAM | Чтение ERAM |
| 0xFD | CMD_TURBO | Турбо режим |

### 12.2 Формат пакета (отправка от эмулятора)

```
┌─────────┬──────────────┬──────────────┬─────────┐
│ CMD (1) │ Reserved(12) │ DATA (240)   │ CRC (2) │
└─────────┴──────────────┴──────────────┴─────────┘
```

**Функция отправки в эмуляторе:**
```cpp
int send_out(uint8_t CMD, uint8_t* DATA, uint8_t length) {
    uint8_t packet_to_send[length + 15];
    
    packet_to_send[0] = CMD;
    // packet_to_send[1-12] = 0 (зарезервировано)
    
    for (int i = 0; i < length; i++) {
        packet_to_send[i+13] = DATA[i];
    }
    
    uint16_t CRC_res = CRC16(packet, length + 13);
    packet_to_send[length + 13] = CRC_res;        // Младший байт CRC
    packet_to_send[length + 14] = CRC_res >> 8;   // Старший байт CRC
    
    Serial.write(packet_to_send, length + 15);
}
```

**Общий размер пакета:** 1 + 12 + 240 + 2 = **255 байт**

### 12.3 Формат пакета (отправка от ПК)

```
┌─────────┬──────────────┬─────────┐
│ CMD (1) │ DATA (N)     │ CRC (2) │
└─────────┴──────────────┴─────────┘
```

**Функция отправки в ПК:**
```cpp
int send_to_tool(uint8_t CMD, uint8_t* DATA, uint8_t length) {
    uint8_t packet[length + 3];
    cli::array<byte>^ packet_to_send = gcnew cli::array<byte>(length + 3);
    
    packet[0] = CMD;
    packet_to_send[0] = CMD;
    
    for (int i = 1; i < length + 1; i++) {
        packet[i] = DATA[i - 1];
        packet_to_send[i] = DATA[i - 1];
    }
    
    uint16_t CRC_res = CRC16(packet, length + 1);
    packet_to_send[length + 1] = CRC_res;        // Младший байт
    packet_to_send[length + 2] = CRC_res >> 8;   // Старший байт
    
    serialPort1->Write(packet_to_send, 0, length + 3);
}
```

### 12.4 CRC16 (Modbus)

```cpp
uint16_t CRC16(const uint8_t *nData, uint16_t wLength) {
    static const uint16_t wCRCTable[] = {
        0X0000, 0XC0C1, 0XC181, 0X0140, ...
    };
    
    uint8_t nTemp;
    uint16_t wCRCWord = 0xFFFF;
    
    while (wLength--) {
        nTemp = *nData++ ^ wCRCWord;
        wCRCWord >>= 8;
        wCRCWord ^= wCRCTable[nTemp];
    }
    
    return wCRCWord;
}
```

---

## 13. ЧТЕНИЕ ДАННЫХ В РЕАЛЬНОМ ВРЕМЕНИ (ПК)

### 13.1 Два режима работы

**Режим 1: AUTONOM (через DLL)**
- Использует AMK_1DDS_AUTONOM.dll
- Таймер: `timer_Sonde_ON_OFF` (интервал 2000 мс)
- Функция: `AMK_GetData(ComPort, &P)`

**Режим 2: LWD (через COM-порт напрямую)**
- Использует System::IO::Ports::SerialPort
- Таймер: `aTimer` (System.Timers.Timer, интервал 2000 мс)
- Функции: `send_to_tool()` / `receive_from_tool()`

### 13.2 Режим AUTONOM (timer_Sonde_ON_OFF_Tick)

```cpp
private: System::Void timer_Sonde_ON_OFF_Tick(System::Object^ sender, System::EventArgs^ e) {
    extern string Dataout;
    extern bool sonde_on_off;
    
    busy = true;
    GP_TOOL_DATA P = { 0, };
    
    if (ComPort != NULL) {
        // Получение данных через DLL
        if (!AMK_GetData(ComPort, &P)) {
            // Ошибка - нет данных
            No_Data = 1;
            this->button_ON_OFF->BackColor = Color::Red;
        }
        else {
            // Данные получены успешно
            
            // Мигание индикатора
            if (blink == 0) { 
                blink = 1; 
                this->button_ON_OFF->BackColor = Color::Red; 
            }
            else { 
                blink = 0; 
                this->button_ON_OFF->BackColor = DefaultBackColor; 
            }
            
            // Проверка сигнатуры и запись в файл
            if (first_sign_after_on == P.signature) {
                fout = new ofstream;
                fout->open(Dataout.c_str(), ios_base::app | ios_base::binary);
                fout->write((char*)&P, sizeof(P));  // Запись 240 байт
                fout->flush();
                fout->close();
                delete fout;
                autonom_points_cntr++;
            }
            
            // Вывод отладочной информации
            cout << " SGN " << P.signature << endl;
            cout << " frame " << P.frame << endl;
            cout << "TEMP " << P.temperature << endl;
            // ... вывод всех данных
        }
    }
    
    busy = false;
}
```

### 13.3 Режим LWD (OnTimer)

```cpp
private:void OnTimer(System::Object^ source, ElapsedEventArgs^ e) {
    GP_TOOL_DATA *P = new GP_TOOL_DATA;
    const uint8_t length = 252;
    uint8_t DATA_IN[length] = { 0, };
    
    // Настройка COM-порта
    serialPort1->PortName = gcnew String(Com_Name.c_str());
    serialPort1->ReadTimeout = 1000;
    serialPort1->WriteTimeout = 1000;
    serialPort1->BaudRate = 125000;
    serialPort1->Open();
    
    // Отправка команды запроса данных
    uint8_t *DATA = new uint8_t;
    *DATA = length;
    send_to_tool_(serialPort1, CMD_WORK, DATA, 1);
    
    Sleep(100);  // Ожидание ответа
    
    // Получение данных
    int result = receive_from_tool_(serialPort1, CMD, DATA_IN, length);
    serialPort1->Close();
    
    if (result == 0) {
        // Данные начинаются с 12-го байта (после заголовка)
        P = (GP_TOOL_DATA*)(DATA_IN + 12);
        
        // Проверка сигнатуры
        if (P->signature == 0x31443453 || P->signature == 0x31443454) {
            // Запись в файл
            fout = new ofstream;
            fout->open(Dataout_LWD.c_str(), ios_base::app | ios_base::binary);
            fout->write((char*)P, sizeof(GP_TOOL_DATA));
            fout->flush();
            fout->close();
            delete fout;
        }
    }
}
```

### 13.4 Инициализация потока чтения

```cpp
// В конструкторе MyForm:
Thread^ Read_Port_Thread = gcnew Thread(gcnew ThreadStart(this, &MyForm::Read_Port));
Read_Port_Thread->Start();

// Функция Read_Port:
public: System::Void Read_Port() { 
    aTimer = gcnew System::Timers::Timer(2000);  // Интервал 2 секунды
    
    if(checkBox_plateutest->Checked == true)
        aTimer->Elapsed += gcnew ElapsedEventHandler(this, &MyForm::OnTimer_);
    else
        aTimer->Elapsed += gcnew ElapsedEventHandler(this, &MyForm::OnTimer);
    
    aTimer->AutoReset = true;
    aTimer->Enabled = true;
}
```

---

## 14. ОБНОВЛЕНИЕ ГРАФИКОВ В РЕАЛЬНОМ ВРЕМЕНИ

### 14.1 Процесс обновления

1. **Таймер срабатывает** каждые 2 секунды
2. **Данные получаются** с прибора (через DLL или COM-порт)
3. **Данные записываются** во временный файл (Dataout или Dataout_LWD)
4. **Пользователь нажимает** "Временный файл" или данные обновляются автоматически
5. **Файл перечитывается** функцией `файлСкважиныToolStripMenuItem_Click`
6. **Графики перерисовываются** функцией `do_chart()`

### 14.2 Функция обновления (renew)

```cpp
private: System::Void renew(System::Object^ sender, System::EventArgs^ e) {
    extern bool sonde_on_off;
    extern bool on_off_1dds;
    extern string Dataout;
    
    if (sonde_on_off == true) {
        Data = Dataout.c_str();
        extension = ".bin";
    }
    if (on_off_1dds == true) {
        Data = Dataout_LWD.c_str();
        extension = ".bin";
    }
    
    // Чтение файла
    ifstream fin;
    fin.open(Data, ios::binary);
    
    for (int n = 0; n < Co; n++) {
        DAT S;
        S.GP = READ_BLOK_AllData_1DDS(fin, n);
        DATA1[n] = S;
        DATA_MA[n] = S;
        NOICE[n] = S;
        SKO[n] = S;
    }
    fin.close();
    
    // Перерисовка графиков
    do_chart(1);
    do_chart(2);
}
```

---

## 15. ПОЛНАЯ СХЕМА ПОТОКА ДАННЫХ В РЕАЛЬНОМ ВРЕМЕНИ

```
┌─────────────────────────────────────────────────────────────────┐
│                         ПРИБОР / ЭМУЛЯТОР                       │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │  AllData_P / GP_TOOL_DATA (240 байт)                    │   │
│  │  - signature, condition, frame, temperature             │   │
│  │  - Ro_smt[8], PH_smt[8]                                 │   │
│  │  - AM_RX_1[10], AM_RX_2[10], PH_RX_1[10], PH_RX_2[10]  │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
                              │
                              │ COM-порт (125000 бод)
                              │ Протокол: CMD + DATA + CRC16
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                              ПК                                  │
│                                                                  │
│  ┌──────────────────┐    ┌──────────────────────────────────┐  │
│  │ Таймер (2 сек)   │───▶│ OnTimer() / timer_Sonde_ON_OFF   │  │
│  └──────────────────┘    │                                   │  │
│                          │ 1. send_to_tool(CMD_WORK)         │  │
│                          │ 2. receive_from_tool()            │  │
│                          │ 3. P = (GP_TOOL_DATA*)(DATA_IN)   │  │
│                          └──────────────────────────────────┘  │
│                                        │                        │
│                                        ▼                        │
│                          ┌──────────────────────────────────┐  │
│                          │ Запись в файл                     │  │
│                          │ fout->write((char*)&P, sizeof(P)) │  │
│                          │ Dataout.bin / Dataout_LWD.bin     │  │
│                          └──────────────────────────────────┘  │
│                                        │                        │
│                                        ▼                        │
│                          ┌──────────────────────────────────┐  │
│                          │ Чтение файла                      │  │
│                          │ READ_BLOK_AllData_1DDS()          │  │
│                          │ DATA1[n].GP = P                   │  │
│                          └──────────────────────────────────┘  │
│                                        │                        │
│                                        ▼                        │
│                          ┌──────────────────────────────────┐  │
│                          │ Обработка данных                  │  │
│                          │ - Разность фаз (PH_RX_2 - PH_RX_1)│  │
│                          │ - Скользящее среднее (W=20)       │  │
│                          │ - Вычисление шума                 │  │
│                          └──────────────────────────────────┘  │
│                                        │                        │
│                                        ▼                        │
│                          ┌──────────────────────────────────┐  │
│                          │ Отрисовка графиков                │  │
│                          │ do_chart(1), do_chart(2)          │  │
│                          │ CHART->Series[i]->Points->AddXY() │  │
│                          └──────────────────────────────────┘  │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 16. КЛЮЧЕВЫЕ ПАРАМЕТРЫ ДЛЯ РЕАЛИЗАЦИИ

### 16.1 Настройки COM-порта

```cpp
serialPort1->BaudRate = 125000;      // Скорость
serialPort1->ReadTimeout = 1000;     // Таймаут чтения (мс)
serialPort1->WriteTimeout = 1000;    // Таймаут записи (мс)
```

### 16.2 Интервал опроса

```cpp
aTimer = gcnew System::Timers::Timer(2000);  // 2 секунды
// или
timer_Sonde_ON_OFF->Interval = 2000;
```

### 16.3 Сигнатуры данных

| Сигнатура | Hex | Описание |
|-----------|-----|----------|
| "1D4S" | 0x31443453 | Стандартный формат |
| "1D4T" | 0x31443454 | Альтернативный формат |

### 16.4 Размеры данных

| Параметр | Размер |
|----------|--------|
| GP_TOOL_DATA | 240 байт |
| Пакет от прибора | 255 байт (1+12+240+2) |
| Пакет от ПК | 4 байта (1+1+2) |

---

## 17. РЕКОМЕНДАЦИИ ДЛЯ НОВОЙ РЕАЛИЗАЦИИ

1. **Сохраните структуру GP_TOOL_DATA** - она является стандартом обмена данными

2. **Используйте тот же протокол CRC16** - для совместимости с существующими приборами

3. **Реализуйте два режима работы:**
   - Через DLL (для автономных приборов)
   - Через COM-порт напрямую (для LWD)

4. **Используйте асинхронный таймер** для опроса прибора (2 секунды - оптимальный интервал)

5. **Записывайте данные во временный файл** - это позволяет:
   - Сохранять историю измерений
   - Перечитывать данные для обновления графиков
   - Восстанавливать данные после сбоя

6. **Проверяйте сигнатуру** перед обработкой данных

7. **Используйте скользящее среднее** (W=20) для сглаживания шума

8. **Нормализуйте фазы** в диапазон [-π, π] перед отображением
