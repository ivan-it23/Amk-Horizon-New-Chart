# `FROM_COMPUTER` для фаз и УЭС — current-state summary + historical notes

## Важно

Ниже в файле сохранён большой historical разбор старого проекта.  
Current-state логика в `AMK-Horizon-New-Chart` должна трактоваться через `CurveSelectionModule`, `MeasurementView` и canonical `GP_TOOL_DATA`.

## Current-state кратко

В текущем проекте:

- `FROM_SONDE` использует готовые `phase_smt` и `rho_smt` из canonical данных;
- `FROM_COMPUTER` остаётся **legacy-compatible branch** внутри `CurveSelectionModule`;
- вычисление опирается на `DELTA_PH`, метрологию и legacy коэффициенты;
- эта ветка не является полноценным generic SMT solver.

## Актуальные ограничения

- вход legacy-компьютерной ветки фактически ориентирован на 4 delta-входа `T1..T4`;
- выход может формировать до 5 SMT-значений;
- использование возможно только при наличии метрологии и совпадении `signature`;
- для runtime/UI нужно учитывать `ToolCapabilities`, а не только фиксированные 5 UI-слотов.

## Где current-state истина

- `Amk-Horizon-New-Chart/CurveSelectionModule.cpp`
- `TECHNICAL_REFERENCE.md`
- `ANALYSIS_GP_TOOL_DATA.md`

## Historical notes below

Ниже оставлен старый разбор исходного происхождения формул и коэффициентов. Он полезен как reference по legacy-математике, но не как полное описание текущего приложения.

---

## 2) Где в UI переключаются режимы и что реально выполняется

В `do_chart(...)` доступны режимы:

- `PH_SMT_from_computer`
- `УЭС_from_computer`
- `PH_SMT_from_sonde`
- `УЭС_from_sonde`

См. список и ветки выбора: @AIR_TEMP_1.0/MyForm.h#3319-3386.

```cpp
// PH_SMT_from_computer
GP_DATA gp_data_buff = gp_data[n];
uint16_t condition = get_condition(&gp_data_buff, &metrology_GP_struct, (uint16_t)index_freq[chart_N], mV);
simmetry(&gp_data_buff, &metrology_GP_struct, (uint16_t)index_freq[chart_N], condition);
for (int Tx = 0; Tx < N_Tx; Tx++) {
    CHART->Series[Tx]->Points->AddXY(N, gp_data_buff.phase_smt[index_freq[chart_N]][Tx] * mG);
}

// УЭС_from_computer
GP_DATA gp_data_buff = gp_data[n];
uint16_t condition = get_condition(&gp_data_buff, &metrology_GP_struct, (uint16_t)(index_freq[chart_N]), mV);
simmetry(&gp_data_buff, &metrology_GP_struct, (uint16_t)index_freq[chart_N], condition);
for (int Tx = 0; Tx < N_Tx; Tx++) {
    RO_ARG(&metrology_GP_struct, &gp_data_buff, Tx, (uint16_t)index_freq[chart_N]);
    CHART->Series[Tx]->Points->AddXY(N, gp_data_buff.rho_smt[index_freq[chart_N]][Tx]);
}

// PH_SMT_from_sonde / УЭС_from_sonde
for (int Tx = 0; Tx < N_Tx; Tx++) {
    CHART->Series[Tx]->Points->AddXY(N, gp_data[n].phase_smt[index_freq[chart_N]][Tx] * mG);
    CHART->Series[Tx]->Points->AddXY(N, gp_data[n].rho_smt[index_freq[chart_N]][Tx]);
}
```

### 2.1 `from_sonde` (без пересчёта)

Логика простая: берутся уже рассчитанные контроллером значения:

- `gp_data[n].phase_smt[freq][Tx]`
- `gp_data[n].rho_smt[freq][Tx]`

См. @AIR_TEMP_1.0/MyForm.h#3365-3369 и @AIR_TEMP_1.0/MyForm.h#3381-3386.

### 2.2 `from_computer` (пересчёт в приложении)

Для каждой точки кривой (`n`) делается:

1. Копия данных точки: `GP_DATA gp_data_buff = gp_data[n]`.
2. Определение маски рабочих каналов: `condition = get_condition(...)`.
3. Расчёт симметризованных фаз: `simmetry(...)`.
4. Для УЭС: по каждому Tx вызывается `RO_ARG(...)`.
5. На график выводится результат из `gp_data_buff`.

См. вызовы: @AIR_TEMP_1.0/MyForm.h#3323-3327 и @AIR_TEMP_1.0/MyForm.h#3345-3351.

Важно: в текущем коде ветка `УЭС_from_computer` ограничена `if (sonde_type == CARTOGRAPH_)` (@AIR_TEMP_1.0/MyForm.h#3343-3344).

---

## 3) Подготовка данных: как `_5Tx_DATA` попадает в `GP_DATA`

При чтении файла/потока структура `_5Tx_DATA` преобразуется в `GP_DATA` через `transform_data(...)`.

Ключевые копирования для 5Tx:

- `phase_smt`, `rho_smt` (данные "из зонда");
- `AM_RX_1`, `AM_RX_2`;
- `DELTA_PH`;
- `ZERO_AM_RX_1/2`, `ZERO_dPH`.

См. @AIR_TEMP_1.0/variable.cpp#243-260.

```cpp
else if constexpr (is_same_v<T, _5Tx_DATA>) {
    gp_data.signature = current.signature;
    gp_data.condition = current.condition;
    gp_data.frame = current.frame;
    gp_data.temperature = current.temperature;
    int N_Tx = 5;
    for (int freq = 0; freq < 2; freq++) {
        gp_data.ZERO_AM_RX_1[freq] = current.ZERO_AM_RX_1[freq];
        gp_data.ZERO_AM_RX_2[freq] = current.ZERO_AM_RX_2[freq];
        gp_data.ZERO_dPH[freq] = current.ZERO_dPH[freq];
        for (int Tx = 0; Tx < N_Tx; Tx++) {
            gp_data.phase_smt[freq][Tx] = current.phase_smt[freq][Tx];
            gp_data.rho_smt[freq][Tx] = current.rho_smt[freq][Tx];
            gp_data.AM_RX_1[freq][Tx] = current.AM_RX_1[freq][Tx];
            gp_data.AM_RX_2[freq][Tx] = current.AM_RX_2[freq][Tx];
            gp_data.DELTA_PH[freq][Tx] = current.DELTA_PH[freq][Tx];
        }
    }
}
```

Это важно: режим `from_computer` использует в качестве входа **именно** `GP_DATA.DELTA_PH` и амплитуды `AM_RX_*`.

---

## 4) Роль метрологии

Расчёты `from_computer` выполняются только если метрология считана и сигнатуры совпали:

- `metro_is_readed_from_sonde == true`
- `metrology_GP_struct.signature == global_signature`

См. проверки в @AIR_TEMP_1.0/MyForm.h#3322-3325 и @AIR_TEMP_1.0/MyForm.h#3344-3347.

```cpp
if (metro_is_readed_from_sonde == true && metrology_GP_struct.signature == global_signature) {
    GP_DATA gp_data_buff = gp_data[n];
    uint16_t condition = get_condition(&gp_data_buff, &metrology_GP_struct, (uint16_t)index_freq[chart_N], mV);
    simmetry(&gp_data_buff, &metrology_GP_struct, (uint16_t)index_freq[chart_N], condition);
}
```

Метрология приводится к унифицированному виду `METROLOGY_GP` через `transform_metro(...)`.
Для `METROLOGY_5Tx` копируются `F`, `L1`, `L2`, `Air_zz`, `Air_zz_amt`.

См. @AIR_TEMP_1.0/variable.cpp#331-391.

```cpp
else if constexpr (is_same_v<T, METROLOGY_5Tx>) {
    gp_metro.signature = current.signature;
    gp_metro.serial = current.serial;
    gp_metro.D_sonde_mm = current.D_sonde_mm;
    int N_Tx = 5;
    for (int freq = 0; freq < 2; freq++) {
        gp_metro.F[freq] = current.F[freq];
        for (int Tx = 0; Tx < N_Tx; Tx++) {
            gp_metro.Air_zz[freq][Tx] = current.Air_zz[freq][Tx];
            gp_metro.Air_zz_amt[freq][Tx] = current.Air_zz_amt[freq][Tx];
        }
    }
    for (int Tx = 0; Tx < N_Tx; Tx++) {
        gp_metro.L1[Tx] = current.L1[Tx];
        gp_metro.L2[Tx] = current.L2[Tx];
    }
}
```

---

## 5) Формулы и шаги расчёта

## 5.1 Определение маски рабочих каналов: `get_condition(...)`

См. @AIR_TEMP_1.0/variable.cpp#589-610.

Идея: на каждом Tx проверяется амплитуда (в Вольтах после масштаба `mV`) относительно порога `Air_zz_amt` из метрологии.

```cpp
uint16_t get_condition(struct GP_DATA *Data, struct METROLOGY_GP *metro, uint16_t freq, float mV)
{
    float Tx_amp_val = 0.0;
    uint16_t condition = 0;
    //Tx1:
    Tx_amp_val = (Data->AM_RX_2[freq][T1]) * mV; //Tx1->Rx2
    if (Tx_amp_val > metro->Air_zz_amt[freq][T1])
        condition |= (1 << (3 - T1));
    //Tx2:
    Tx_amp_val = (Data->AM_RX_1[freq][T2]) * mV; //Tx2->Rx1
    if (Tx_amp_val > metro->Air_zz_amt[freq][T2])
        condition |= (1 << (3 - T2));
    //Tx3:
    Tx_amp_val = (Data->AM_RX_2[freq][T3]) * mV; //Tx3->Rx2
    if (Tx_amp_val > metro->Air_zz_amt[freq][T3])
        condition |= (1 << (3 - T3));
    //Tx4:
    Tx_amp_val = (Data->AM_RX_1[freq][T4]) * mV; //Tx4->Rx1
    if (Tx_amp_val > metro->Air_zz_amt[freq][T4])
        condition |= (1 << (3 - T4));

    return condition;
}
```

где `bit(Tk)` в коде задаётся как `1 << (3 - Tk)`.

> Примечание: в текущей реализации логика симметризации работает по 4 каналам (T1..T4).

---

## 5.2 Симметризация фаз: `simmetry(...)`

См. @AIR_TEMP_1.0/variable.cpp#677-703.

Алгоритм:

1. Берутся сырые разности фаз `d_PH[0..3] = DELTA_PH[freq][0..3]`.
2. По `condition` выбирается матрица коэффициентов `K[4][4]` через `formula_simmetry(...)`.
3. Вычисляются симметризованные фазы:

\[
\phi^{smt}_n = \sum_{j=1}^{4} K_{n,j} \cdot \Delta\phi_j, \quad n=1..4
\]

в коде:

```cpp
Data->phase_smt[freq][n] =
    K[n][0] * d_PH[0] +
    K[n][1] * d_PH[1] +
    K[n][2] * d_PH[2] +
    K[n][3] * d_PH[3];
```

См. @AIR_TEMP_1.0/variable.cpp#699-702.

```cpp
uint16_t simmetry(struct GP_DATA *Data, struct METROLOGY_GP *metro, uint16_t freq, uint16_t condition) {
    uint16_t avaria = condition;
    float K[4][4] = { 0.0, };
    float d_PH[4] = { 0.0, };

    for (int Tx = 0; Tx < 4; Tx++) {
        d_PH[Tx] = Data->DELTA_PH[freq][Tx];
    }

    uint16_t formula = formula_simmetry(K, avaria);
    for (int n = 0; n < 4; n++) {
        Data->phase_smt[freq][n] = K[n][0] * d_PH[0] + K[n][1] * d_PH[1] + K[n][2] * d_PH[2] + K[n][3] * d_PH[3];
    }
    return formula;
}
```

### Матрицы `K` (выбор по condition)

См. @AIR_TEMP_1.0/variable.cpp#614-655.

```cpp
if (condition == 0b00001111) {
    K[T1][T1] = +0.75;  K[T1][T2] = +0.50; K[T1][T3] = -0.25; K[T1][T4] = +0.00;
    K[T2][T1] = +0.25;  K[T2][T2] = +0.50; K[T2][T3] = +0.25; K[T2][T4] = +0.00;
    K[T3][T1] = +0.00;  K[T3][T2] = +0.25; K[T3][T3] = +0.50; K[T3][T4] = +0.25;
    K[T4][T1] = +0.00;  K[T4][T2] = -0.25; K[T4][T3] = +0.50; K[T4][T4] = +0.75;
    result = 0b00001111;
}
else if (condition == 0b00000000) {
    K[T1][T1] = 1.00; K[T1][T2] = 0.00; K[T1][T3] = 0.00; K[T1][T4] = 0.00;
    K[T2][T1] = 0.00; K[T2][T2] = 1.00; K[T2][T3] = 0.00; K[T2][T4] = 0.00;
    K[T3][T1] = 0.00; K[T3][T2] = 0.00; K[T3][T3] = 1.00; K[T3][T4] = 0.00;
    K[T4][T1] = 0.00; K[T4][T2] = 0.00; K[T4][T3] = 0.00; K[T4][T4] = 1.00;
    result = 0b00000000;
}
```

Примеры:

- `condition == 0b00001111` (все 4 канала рабочие):

\[
K=
\begin{bmatrix}
0.75 & 0.50 & -0.25 & 0.00\\
0.25 & 0.50 & 0.25 & 0.00\\
0.00 & 0.25 & 0.50 & 0.25\\
0.00 & -0.25 & 0.50 & 0.75
\end{bmatrix}
\]

- `condition == 0b00000000` (авария/нет валидных Tx):

\[
K = I
\]

(единичная матрица, фаза остаётся как есть).

---

## 5.3 Расчёт УЭС по симметризованной фазе: `RO_ARG(...)`

См. @AIR_TEMP_1.0/variable.cpp#555-581 и @AIR_TEMP_1.0/variable.cpp#541-547.

### 5.3.1 Прямая модель фазы от сопротивления: `SIGNAL(...)`

```cpp
float SIGNAL(struct METROLOGY_GP *metrology, uint16_t n, uint16_t freq, float ro) {
    float L1 = metrology->L1[n] / 1000.0;
    float L2 = metrology->L2[n] / 1000.0;
    float omegamu0sigma = (0.0078957 * metrology->F[freq]) / ro;
    complex<float> ik = 1if * sqrt(1if * omegamu0sigma);
    complex<float> SGN = exp(ik * (L2 - L1)) * ((1.0f - ik * L2) / (1.0f - ik * L1));
    return arg(SGN);
}
```

То есть

\[
\phi_{model}(\rho) = \arg\left(
\exp\{ik(L_2-L_1)\}\cdot\frac{1-ikL_2}{1-ikL_1}
\right),
\quad
k=i\sqrt{i\cdot(0.0078957\cdot F/\rho)}
\]

### 5.3.2 Обратная задача (подбор `rho`)

Подбирается `rho` на отрезке `[0.01, 7000]` по минимуму невязки:

\[
\varepsilon(\rho)=\phi^{smt}_{meas}-\phi_{model}(\rho)
\]

В коде используется поиск в стиле золотого сечения (точки `X1`, `X2`):

- если `|A| > |B|` -> сдвиг левой границы;
- иначе -> сдвиг правой границы;
- остановка по `epsilon_ARG` или `epsilon_INF`.

Итог:

- успешный подбор: `Ro0 = (ro_0 + ro_max)/2`;
- fallback: `Ro0 = 7200`.

Результат сохраняется в `Data->rho_smt[freq][n]`.

```cpp
int32_t RO_ARG(struct METROLOGY_GP *metrology, struct GP_DATA *Data, uint16_t n, uint16_t freq) {
    float Ro0;
    float epsilon_ARG = 0.0000005;
    float epsilon_INF = 0.000000005;
    float Ro_0 = 0.01, Ro_max = 7000.0;
    float ro_0 = Ro_0, ro_max = Ro_max;
    bool stop_epsl_ARG = false;
    bool stop_epsl_INF = false;

    do {
        float X1 = ro_0 + 0.382 * (ro_max - ro_0);
        float X2 = ro_max - 0.382 * (ro_max - ro_0);
        float A = Data->phase_smt[freq][n] - SIGNAL(metrology, n, freq, X1);
        float B = Data->phase_smt[freq][n] - SIGNAL(metrology, n, freq, X2);
        if (fabs(A) > fabs(B)) { ro_0 = X1; }
        else { ro_max = X2; }
        if (fabs(A) < epsilon_ARG || fabs(B) < epsilon_ARG) { stop_epsl_ARG = true; }
        else if (fabs(A - B) < epsilon_INF) { stop_epsl_INF = true; }
    } while (stop_epsl_ARG == false && stop_epsl_INF == false);

    if (stop_epsl_ARG == true) Ro0 = (ro_0 + ro_max) / 2;
    if (stop_epsl_INF == true) Ro0 = 7200;
    Data->rho_smt[freq][n] = Ro0;
    return 0;
}
```

---

## 6) Что важно именно для `_5Tx_DATA`

## 6.1 Что уже есть в структуре

`_5Tx_DATA` уже содержит и сырые входы (`DELTA_PH`, `AM_RX_*`), и готовые выходы контроллера (`phase_smt`, `rho_smt`).

См. @AIR_TEMP_1.0/variable.h#393-406.

## 6.2 Ограничение текущей реализации `from_computer`

Текущая пара функций `get_condition + simmetry + RO_ARG` в `variable.cpp` ориентирована на 4 канала (циклы `Tx < 4`).

См. @AIR_TEMP_1.0/variable.cpp#589-607 и @AIR_TEMP_1.0/variable.cpp#683-701.

```cpp
// get_condition: только T1..T4
Tx_amp_val = (Data->AM_RX_2[freq][T1]) * mV;
Tx_amp_val = (Data->AM_RX_1[freq][T2]) * mV;
Tx_amp_val = (Data->AM_RX_2[freq][T3]) * mV;
Tx_amp_val = (Data->AM_RX_1[freq][T4]) * mV;

// simmetry: только 4 фазы
for (int Tx = 0; Tx < 4; Tx++) {
    d_PH[Tx] = Data->DELTA_PH[freq][Tx];
}
for (int n = 0; n < 4; n++) {
    Data->phase_smt[freq][n] = K[n][0] * d_PH[0] + K[n][1] * d_PH[1] + K[n][2] * d_PH[2] + K[n][3] * d_PH[3];
}
```

То есть для 5Tx в текущем проекте "чистый" пересчёт `from_computer` для T5 в этих функциях не реализован.

## 6.3 Подсказка из кода для расширения на 5Tx

В `variable.cpp` есть закомментированный блок `simmetry_5(...)` с коэффициентами для 5 выходных каналов на каждой частоте.

См. @AIR_TEMP_1.0/variable.cpp#708-730.

```cpp
/*
void simmetry_5(PHASE *Phase_in, PHASE_5 *Phase_smt) {
    // 400 kHz
    Phase_smt->Phase[0] = 1.150 * Phase_in->Phase[T1_400] + 0.500 * Phase_in->Phase[T2_400] - 0.650 * Phase_in->Phase[T3_400] + 0.000 * Phase_in->Phase[T4_400];
    Phase_smt->Phase[1] = 0.675 * Phase_in->Phase[T1_400] + 0.500 * Phase_in->Phase[T2_400] - 0.175 * Phase_in->Phase[T3_400] + 0.000 * Phase_in->Phase[T4_400];
    Phase_smt->Phase[2] = 0.200 * Phase_in->Phase[T1_400] + 0.500 * Phase_in->Phase[T2_400] + 0.300 * Phase_in->Phase[T3_400] + 0.000 * Phase_in->Phase[T4_400];
    Phase_smt->Phase[3] = 0.000 * Phase_in->Phase[T1_400] + 0.225 * Phase_in->Phase[T2_400] + 0.500 * Phase_in->Phase[T3_400] + 0.275 * Phase_in->Phase[T4_400];
    Phase_smt->Phase[4] = 0.000 * Phase_in->Phase[T1_400] - 0.250 * Phase_in->Phase[T2_400] + 0.500 * Phase_in->Phase[T3_400] + 0.750 * Phase_in->Phase[T4_400];
}
*/
```

Это полезный ориентир для переноса в новый проект: можно сделать `simmetry_5(...)`, которая формирует `phase_smt[freq][0..4]`, после чего вызывать `RO_ARG(...)` уже для `Tx=0..4`.

---

## 7) Рекомендуемый порядок переноса в новый проект (для `_5Tx_DATA`)

1. **Нормализовать вход**: преобразовать `_5Tx_DATA` -> внутренний `GP_DATA`-подобный DTO.
2. **Проверить метрологию**: сигнатура + обязательные поля (`F`, `L1/L2`, `Air_zz_amt`).
3. **Реализовать `get_condition_5tx(...)`**:
   - минимум: сохранить логику T1..T4 из исходника;
   - отдельно определить, участвует ли T5 в маске.
4. **Реализовать `simmetry_5tx(...)`**:
   - либо использовать ваши согласованные коэффициенты 5x5/5x4;
   - либо начать с коэффициентов из закомментированного блока как baseline.
5. **Реализовать `rho_from_phase(...)` на каждый Tx**:
   - перенести `SIGNAL + RO_ARG` без изменения математики;
   - вызвать для всех целевых каналов (включая T5, если есть фазовая модель для него).
6. **Сравнение с зондом**:
   - графики `from_sonde` = данные из структуры;
   - графики `from_computer` = ваш пересчёт;
   - добавить кривую `difference` (как в исходнике с "Погрешность УЭС").

---

## 8) Минимальный псевдокод для нового проекта

```cpp
for each sample in stream:
    d = map_5tx_to_gp(sample)

    if !metrology_ready || d.signature != metrology.signature:
        continue

    cond = get_condition_5tx(d, metrology, freq, mV)
    phase_from_pc = simmetry_5tx(d.DELTA_PH[freq], cond, metrology)

    for tx in active_tx:
        rho_from_pc[tx] = invert_phase_to_rho(phase_from_pc[tx], tx, freq, metrology)

    // compare with sonde values
    phase_from_sonde = d.phase_smt[freq]
    rho_from_sonde   = d.rho_smt[freq]
```

---

## 9) Контрольные точки в коде (быстрые ссылки)

- Структура `_5Tx_DATA`: @AIR_TEMP_1.0/variable.h#393-406
- Унифицированная `GP_DATA`: @AIR_TEMP_1.0/variable.h#410-423
- Вызовы `from_computer`/`from_sonde` на графике: @AIR_TEMP_1.0/MyForm.h#3319-3386
- Трансформация `_5Tx_DATA -> GP_DATA`: @AIR_TEMP_1.0/variable.cpp#243-260
- `SIGNAL`: @AIR_TEMP_1.0/variable.cpp#541-547
- `RO_ARG`: @AIR_TEMP_1.0/variable.cpp#555-581
- `get_condition`: @AIR_TEMP_1.0/variable.cpp#589-610
- `formula_simmetry`: @AIR_TEMP_1.0/variable.cpp#614-655
- `simmetry`: @AIR_TEMP_1.0/variable.cpp#677-703
- Черновые коэффициенты `simmetry_5` (комментарий): @AIR_TEMP_1.0/variable.cpp#708-730
- Приведение метрологии к `METROLOGY_GP`: @AIR_TEMP_1.0/variable.cpp#331-391
