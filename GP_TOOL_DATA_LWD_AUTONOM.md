# Документация по логике `GP_TOOL_DATA` для `LWD` и `AUTONOM`

## Область документа

Этот документ описывает **только** ту логику старого проекта `TEST_SONDE_KARTOGRAF_2.0`, которая относится к переносу в новую структуру:

```cpp
struct GP_TOOL_DATA {
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

Рассматриваются **только приборы**:

- `LWD_4Tx`
- `AUTONOM_5Tx`
- `AUTONOM_5Tx_SDR`

`CARTOGRAPH` и связанная с ним логика здесь **не рассматриваются**, кроме редких мест, где он мешает или ограничивает общую реализацию.

---

## 1. Что является ближайшим источником данных для `GP_TOOL_DATA`

В старом проекте рабочая визуализация строится не напрямую по `GP_TOOL_DATA`, а по структуре `GP_DATA`.

Код из `variable.h`:

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

struct _5Tx_DATA {// 240 byte
    uint32_t signature;
    uint32_t condition;
    uint32_t frame;
    float temperature;
    float rho_smt[2][5];// УЭС, рассчитанные на контроллере [400, 2000][T1-T5]
    float phase_smt[2][5];// симметризованные фазы [400, 2000][T1-T5]
    float AM_RX_1[2][5];// амплитуды на первом приемнике [400, 2000][T1-T5]
    float ZERO_AM_RX_1[2];
    float AM_RX_2[2][5];// амплитуды на втором приемнике[400, 2000][T1 - T5]
    float ZERO_AM_RX_2[2];// амплитуды на втором приемнике [400, 2000] при молчащих передатчиках
    float DELTA_PH[2][5];// сырая разница фаз
    float ZERO_dPH[2];// разница фаз молчащих передатчиков;
};

struct GP_DATA {
    uint32_t signature;
    uint32_t condition;
    uint32_t frame;
    float temperature;
    float rho_smt[2][5];// УЭС, рассчитанные на контроллере [400, 2000][T1-T5]
    float phase_smt[2][5];// симметризованные фазы [400, 2000][T1-T5]
    float AM_RX_1[2][5];// амплитуды на первом приемнике [400, 2000][T1-T5]
    float ZERO_AM_RX_1[2]; // амплитуды на первом приемнике [400, 2000] при молчащих передатчиках
    float AM_RX_2[2][5];// амплитуды на втором приемнике[400, 2000][T1 - T5]
    float ZERO_AM_RX_2[2];// амплитуды на втором приемнике [400, 2000] при молчащих передатчиках
    float DELTA_PH[2][5];// сырая разница фаз [400, 2000][T1 - T5]
    float ZERO_dPH[2];// разница фаз молчащих передатчиков;
    //uint8_t out_arr[8];
    //uint32_t selected_key;
};
```

### Вывод

С точки зрения старого проекта, логический путь такой:

- файл / COM-порт
- `_4Tx_DATA` или `_5Tx_DATA`
- `transform_data(...)`
- `GP_DATA`
- отображение в `do_chart(...)`

То есть для переноса в новый проект `GP_TOOL_DATA` логично строить **поверх старой связки** `_4Tx_DATA | _5Tx_DATA -> GP_DATA`.

---

## 2. Важное ограничение: ваша `GP_TOOL_DATA` не совпадает 1 в 1 со старым форматом

Это нужно зафиксировать сразу.

### 2.1. Для `LWD_4Tx`

Старый проект реально содержит:

- `rho_smt[2][4]` -> 8 значений
- `phase_smt[2][4]` -> 8 значений
- `AM_RX_1[2][4]` -> 8 значений
- `AM_RX_2[2][4]` -> 8 значений
- `PH_RX_1[2][4]` -> 8 значений
- `PH_RX_2[2][4]` -> 8 значений
- `ZERO_AM_RX_1[2]`, `ZERO_AM_RX_2[2]`
- `ZERO_PH_RX_1[2]`, `ZERO_PH_RX_2[2]`

То есть для `LWD_4Tx` поля `Ro_smt[8]` и `PH_smt[8]` совпадают естественно.

### 2.2. Для `AUTONOM_5Tx`

Старый проект реально содержит:

- `rho_smt[2][5]` -> 10 значений
- `phase_smt[2][5]` -> 10 значений
- `AM_RX_1[2][5]` -> 10 значений
- `AM_RX_2[2][5]` -> 10 значений
- **нет** отдельных `PH_RX_1[2][5]` и `PH_RX_2[2][5]`
- есть только `DELTA_PH[2][5]`

### 2.3. Практический вывод для переноса

Ваша новая структура:

- хорошо подходит под `LWD_4Tx` по `Ro_smt[8]` / `PH_smt[8]`
- хорошо подходит под `AUTONOM_5Tx` по `AM_RX_1[10]` / `AM_RX_2[10]`
- **не является прямой копией** ни `_4Tx_DATA`, ни `_5Tx_DATA`
- для `AUTONOM_5Tx` в старом проекте **нет подтверждённого источника** для отдельных массивов `PH_RX_1[10]` / `PH_RX_2[10]`; есть только `DELTA_PH[2][5]`

Если в новом проекте нужно сохранить именно `PH_RX_1[10]` и `PH_RX_2[10]`, то по старому коду:

- для `LWD_4Tx` это возможно
- для `AUTONOM_5Tx` это **не подтверждается**, потому что в старой полезной структуре есть только `DELTA_PH`

---

## 3. Как читаются `.bin`, `.biv`, `.DEV`

### 3.1. Определение типа прибора и количества кадров

Код из `variable.cpp`:

```cpp
void define_sonde_type(DATA_FILE_PARAM *param) {
    int signature_for_pallete;
    uint32_t buff;
    int pre;
    int post;
    uint32_t sign;
    uint32_t struct_size;

    if (param->extension == ".bin" || param->extension == ".biv") {
        pre = 0; post = 0;
    }
    if (param->extension == ".DEV") {
        pre = 11;
        post = 21;
    }
    cout << " pre " << pre << " post " << post << endl;

    param->fin->seekg(pre);
    param->fin->read((char*)&sign, sizeof(uint32_t));
    param->fin->seekg(0, ios::end);
    buff = sign << 12;      buff = buff >> 12;
    signature_for_pallete = buff / 1000;
    cout << "signature_for_pallete " << signature_for_pallete << endl;

    if (signature_for_pallete == LWD_4Tx_ && (param->extension == ".bin" || param->extension == ".biv")) {
        struct_size = sizeof(_4Tx_DATA);
        cout << "LWD_4Tx_ .bin " << struct_size << " bytes struct size" << endl;
    }
    if (signature_for_pallete == LWD_4Tx_ &&  param->extension == ".DEV") {
        struct_size = (sizeof(_4Tx_DATA)) + pre;
        cout << "LWD_4Tx_ .DEV " << struct_size << " bytes struct size" << endl;
    }
    if ((signature_for_pallete == AUTONOM_5Tx_ || signature_for_pallete == AUTONOM_5Tx_SDR_) && (param->extension == ".bin" || param->extension == ".biv")) {
        struct_size = sizeof(_4Tx_DATA);
        cout << "AUTONOM_5Tx_ .bin " << struct_size << " bytes struct size" << endl;
    }
    if ((signature_for_pallete == AUTONOM_5Tx_ || signature_for_pallete == AUTONOM_5Tx_SDR_) && param->extension == ".DEV") {
        struct_size = (sizeof(_4Tx_DATA)) + pre;
        cout << "AUTONOM_5Tx_ .DEV " << struct_size << " bytes struct size" << endl;
    }

    param->sonde_type = signature_for_pallete;
    param->signature = sign;
    if (signature_for_pallete == CARTOGRAPH_ && param->extension == ".DEV")
        param->file_frames = (int(param->fin->tellg()) - struct_size)/ (2*struct_size);
    else
        param->file_frames = int(param->fin->tellg()) / struct_size;
    cout << "file_frames " << std::dec << param->file_frames << endl;
}
```

### 3.2. Что делает этот код

- `.bin` и `.biv` читаются как последовательность кадров без префикса
- `.DEV` читается со смещением `pre = 11`
- тип прибора определяется по `signature_for_pallete`
- количество кадров считается по размеру структуры

### 3.3. Важное наблюдение по коду

В `define_sonde_type(...)` для `AUTONOM_5Tx_*` используется:

```cpp
struct_size = sizeof(_4Tx_DATA);
```

Но фактическое чтение кадров для `AUTONOM_5Tx_*` в `read_blok(...)` выполняется как `_5Tx_DATA`.

Это **подтверждённое расхождение в старом коде**. Для переноса логики чтения кадров нужно ориентироваться на `read_blok(...)`, потому что именно он реально десериализует кадры.

### 3.4. Фактическое чтение кадра из файла

Код из `variable.cpp`:

```cpp
// если файл LWD_4Tx_ сформирован в режиме информации
if (param->sonde_type == LWD_4Tx_ && (param->extension == ".bin" || param->extension == ".biv")) {
    _4Tx_DATA _4Tx_data;
    param->fin->seekg(n * sizeof(_4Tx_DATA));
    param->fin->read((char*)&_4Tx_data, sizeof(_4Tx_DATA));
    result = _4Tx_data;
}
// если файл LWD_4Tx_ получен из памяти
if (param->sonde_type == LWD_4Tx_ && param->extension == ".DEV") {
    _4Tx_DATA _4Tx_data;
    uint8_t *buff = new uint8_t[12];
    param->fin->seekg(n * (sizeof(_4Tx_DATA) + 11));
    param->fin->read((char*)buff, 11);
    param->fin->read((char*)&_4Tx_data, sizeof(_4Tx_DATA));
    result = _4Tx_data;
}

// если файл AUTONOM_5Tx_ или AUTONOM_5Tx_SDR сформирован в режиме информации
if ((param->sonde_type == AUTONOM_5Tx_ || param->sonde_type == AUTONOM_5Tx_SDR_) && (param->extension == ".bin" || param->extension == ".biv")) {
    _5Tx_DATA _5Tx_data;
    param->fin->seekg(n * sizeof(_5Tx_DATA));
    param->fin->read((char*)&_5Tx_data, sizeof(_5Tx_DATA));
    result = _5Tx_data;
}
// если файл AUTONOM_5Tx_ или AUTONOM_5Tx_SDR получен из памяти
if ((param->sonde_type == AUTONOM_5Tx_ || param->sonde_type == AUTONOM_5Tx_SDR_) && param->extension == ".DEV") {
    _5Tx_DATA _5Tx_data;
    uint8_t *buff = new uint8_t[12];
    param->fin->seekg(n * (sizeof(_5Tx_DATA) + 11));
    param->fin->read((char*)buff, 11);
    param->fin->read((char*)&_5Tx_data, sizeof(_5Tx_DATA));
    result = _5Tx_data;
}
```

### 3.5. Вывод по форматам файлов

- **`.bin`** -> покадровое чтение структуры подряд
- **`.biv`** -> для `LWD`/`AUTONOM` обрабатывается так же, как `.bin`
- **`.DEV`** -> у каждого кадра есть префикс `11` байт, который пропускается

---

## 4. Как файл попадает в `gp_data`

Код из `MyForm.h`:

```cpp
openFileDialog->Filter = "bin files   (*.bin; *.ifn; *.biv; *.DEV; *.las;)|*.bin; *.ifn; *.biv; *.DEV; *.las;";

if (openFileDialog->ShowDialog() == System::Windows::Forms::DialogResult::OK) {
    param.Data_file_name = (char*)Marshal::StringToHGlobalAnsi(openFileDialog->FileName).ToPointer();
    Data_file_name = param.Data_file_name;
    param.extension = (char*)Marshal::StringToHGlobalAnsi(Path::GetExtension(openFileDialog->FileName)).ToPointer();
    res = true;
}

fin.open(Data_file_name, ios::binary);
define_sonde_type(&param);
fin.close();

sonde_type = param.sonde_type;
global_signature = param.signature;

fin.open(Data_file_name, ios::binary);
for (int n = 0; n < param.file_frames; n++) {
    variant<_4Tx_DATA, _5Tx_DATA, ALL_DATA > current_tool_data_buff;
    GP_DATA gp_data_buff;
    ALL_DATA all_data_buff;
    current_tool_data_buff = read_blok(&param, n);
    gp_data_buff = transform_data(current_tool_data_buff, &all_data_buff);
    gp_data.push_back(gp_data_buff);
}
```

### Вывод

Файловая цепочка старая и простая:

- открыть файл
- определить тип и число кадров `define_sonde_type(...)`
- для каждого кадра вызвать `read_blok(...)`
- привести кадр к общей форме `transform_data(...)`
- сложить результат в `gp_data`

---

## 5. Как `_4Tx_DATA` и `_5Tx_DATA` превращаются в рабочий `GP_DATA`

### 5.1. Преобразование `LWD_4Tx -> GP_DATA`

Код из `variable.cpp`:

```cpp
if constexpr (is_same_v<T, _4Tx_DATA>) {
    gp_data.signature = current.signature;
    gp_data.condition = current.condition;
    gp_data.frame = current.frame;
    gp_data.temperature = current.temperature;
    int N_Tx = 4;
    for (int freq = 0; freq < 2; freq++) {
        gp_data.ZERO_AM_RX_1[freq] = current.ZERO_AM_RX_1[freq];
        gp_data.ZERO_AM_RX_2[freq] = current.ZERO_AM_RX_2[freq];
        for (int Tx = 0; Tx < N_Tx; Tx++) {
            gp_data.phase_smt[freq][Tx] = current.phase_smt[freq][Tx];
            gp_data.rho_smt[freq][Tx] = current.rho_smt[freq][Tx];
            gp_data.AM_RX_1[freq][Tx] = current.AM_RX_1[freq][Tx];
            gp_data.AM_RX_2[freq][Tx] = current.AM_RX_2[freq][Tx];

            float signal = current.PH_RX_2[freq][Tx] - current.PH_RX_1[freq][Tx];
            if (fabs(signal) < 40) {
                while (signal >= PI) { signal -= 2 * PI; }
                while (signal <= -PI) { signal += 2 * PI; }
            }
            else { signal = 0.0; }
            gp_data.DELTA_PH[freq][Tx] = signal;
        }
    }
}
```

### Что это значит

Для `LWD_4Tx` в рабочий `GP_DATA` переносятся:

- `signature`
- `condition`
- `frame`
- `temperature`
- `phase_smt`
- `rho_smt`
- `AM_RX_1`
- `AM_RX_2`
- `ZERO_AM_RX_1`
- `ZERO_AM_RX_2`

Сырые фазы `PH_RX_1` и `PH_RX_2` **не сохраняются** в `GP_DATA`, но из них вычисляется:

```cpp
DELTA_PH = PH_RX_2 - PH_RX_1
```

с последующей нормализацией к диапазону `[-PI, +PI]`.

### 5.2. Преобразование `AUTONOM_5Tx -> GP_DATA`

Код из `variable.cpp`:

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

### Что это значит

Для `AUTONOM_5Tx`:

- симметризованные фазы и УЭС приходят уже готовыми с прибора
- амплитуды обоих приёмников приходят уже готовыми
- сырая фазовая информация представлена как `DELTA_PH`
- отдельных `PH_RX_1` и `PH_RX_2` в рабочем формате нет

### 5.3. Главный вывод для переноса в `GP_TOOL_DATA`

Если новая структура должна содержать:

```cpp
float PH_RX_1[10];
float PH_RX_2[10];
```

то по подтверждённому старому коду:

- для `LWD_4Tx` их можно брать из `_4Tx_DATA`
- для `AUTONOM_5Tx` в старой модели есть только `DELTA_PH`, а не раздельные фазы приёмников

---

## 6. Как работает `COM`-порт

## 6.1. Включение прибора и определение типа

Код из `MyForm.h`:

```cpp
serialPort1->Close();
serialPort1->PortName = gcnew String(Com_Name.c_str());
Sleep(100);
serialPort1->BaudRate = baudrate;
serialPort1->ReadTimeout = 3500;
serialPort1->WriteTimeout = 2000;
serialPort1->ReadBufferSize = 10000;
Console::WriteLine(serialPort1->PortName);
try { serialPort1->Open(); }
catch (...) { stop = 1; }

if (!stop) {
    serialPort1->DiscardOutBuffer();
    Sleep(100);
    send_to_tool(serialPort1, CMD_GET_SIGNATURE, &D0, 0);
    Sleep(2000);
    uint8_t CMD;  uint16_t readed_length = 0;  uint8_t DATA_IN[10] = { 0, };
    int result_receive_from_sonde = receive_from_tool(serialPort1, CMD, DATA_IN, &readed_length);
    serialPort1->Close();

    if (result_receive_from_sonde != 0) {
        MessageBox::Show("ID НЕ ПОЛУЧЕН, БУДЕТ УСТАНОВЛЕН ТИП LWD_4Tx!");
        toolStripComboBox_sonde_type->SelectedItem = "LWD_4Tx";
        sonde_type = LWD_4Tx_;
    }

    if (result_receive_from_sonde == 0) {
        uint32_t signature = *(uint32_t*)(DATA_IN);
        cout << "signature_0 " << std::hex << signature << endl;
        id = get_sonde_id(signature);
        cout << "id.type_0 " << std::dec << id.type << endl;

        else if (id.type == LWD_4Tx_) {
            toolStripComboBox_sonde_type->SelectedItem = "LWD_4Tx";
            sonde_type = LWD_4Tx_;
        }
        else if (id.type == AUTONOM_5Tx_) {
            toolStripComboBox_sonde_type->SelectedItem = "AUTONOM_5Tx";
            sonde_type = AUTONOM_5Tx_;
        }
        else if (id.type == AUTONOM_5Tx_SDR_) {
            toolStripComboBox_sonde_type->SelectedItem = "AUTONOM_5Tx_SDR";
            sonde_type = AUTONOM_5Tx_SDR_;
        }
    }
}
```

### Что это значит

Старый проект сначала:

- открывает `COM`
- посылает `CMD_GET_SIGNATURE`
- по ответу определяет тип прибора
- если сигнатуру получить не удалось, насильно выбирает `LWD_4Tx`

---

## 6.2. Чтение метрологии перед запуском онлайн-чтения

Сразу после определения типа вызывается:

```cpp
current_metrology_struct = read_metro_from_tool(serialPort1, sonde_type, &read_status);
if (read_status == true) {
    MessageBox::Show("ФАЙЛ МЕТРОЛОГИИ  СЧИТАН ИЗ ПРИБОРА!");
}
else
    MessageBox::Show("ФАЙЛ МЕТРОЛОГИИ НЕ БЫЛ СЧИТАН !!!");

serialPort1->Close();
sonde_on_off = true;

if (sonde_type == CARTOGRAPH_ ||sonde_type == AUTONOM_5Tx_SDR_)Timer_get_data_from_sonde->Interval = 2000;
else Timer_get_data_from_sonde->Interval = 1000;

Timer_get_data_from_sonde->Enabled = true;
```

### Вывод

Онлайн-режим в старом проекте стартует только после попытки чтения метрологии.

---

## 6.3. Онлайн-чтение `LWD_4Tx`

Код из `MyForm.h`:

```cpp
const uint8_t length = 252;
uint8_t *DATA = new uint8_t;
*DATA = length;
uint8_t CMD;
uint8_t DATA_IN[length] = { 0, };
_4Tx_DATA *_4Tx_data = new _4Tx_DATA;
uint32_t signature_for_pallete;
int readed_from_sonde = 1;

if (send_to_tool_(serialPort1, CMD_WORK, DATA, 1) == 1) {
    MessageBox::Show("Соединение с прибором прервано переподключите !");
}
else {
    Sleep(100);
    readed_from_sonde = receive_from_tool_(serialPort1, CMD, DATA_IN, length);
    Console::WriteLine("_4Tx_DATA received ");
}
if (readed_from_sonde == 0) {
    _4Tx_data = (_4Tx_DATA*)(DATA_IN + 12);
    uint32_t buff;
    buff = _4Tx_data->signature << 12;
    buff = buff >> 12;
    signature_for_pallete = buff / 1000;
    cout << "signature_for_pallete " << signature_for_pallete << endl;
    if (signature_for_pallete == LWD_4Tx_) {
        fout = new ofstream;
        fout->open(Dataout.c_str(), ios_base::app | ios_base::binary);
        fout->write((char*)_4Tx_data, sizeof(_4Tx_DATA));
        fout->flush();
        fout->close();
        delete fout;
    }
}
```

### Что это значит

Для `LWD_4Tx` проект:

- шлёт `CMD_WORK`
- получает пакет длиной `252`
- из пакета берёт полезную структуру `_4Tx_DATA` со смещением `+12`
- дописывает чистый `_4Tx_DATA` в бинарный временный файл `Dataout`

То есть визуализация в онлайне идёт не напрямую из ответа прибора, а через временный бинарный файл.

---

## 6.4. Онлайн-чтение `AUTONOM_5Tx`

Код из `MyForm.h`:

```cpp
send_to_tool(serialPort1, CMD_GET_DATA, &D0, 0);
Sleep(300);
readed_from_sonde = receive_from_tool(serialPort1, CMD, DATA_IN, &readed_length);

if (readed_from_sonde == 0) {
    Console::WriteLine("_5Tx_DATA received ");
    _5Tx_data = (_5Tx_DATA*)(DATA_IN);
    id = get_sonde_id(_5Tx_data->signature);
    cout << "id.type " << std::dec << id.type << endl;
    if (id.type == AUTONOM_5Tx_ || id.type == AUTONOM_5Tx_SDR_) {
        fout = new ofstream;
        fout->open(Dataout.c_str(), ios_base::app | ios_base::binary);
        fout->write((char*)_5Tx_data, sizeof(_5Tx_DATA));
        fout->flush();
        fout->close();
        delete fout;
    }
}
```

### Что это значит

Для `AUTONOM_5Tx`:

- посылается `CMD_GET_DATA`
- ответ сразу интерпретируется как `_5Tx_DATA`
- затем `_5Tx_DATA` дописывается во временный бинарный файл `Dataout`

---

## 6.5. Как онлайн-данные попадают на график

После накопления `Dataout` файл заново читается как обычный `.bin`:

```cpp
Data_file_name = Dataout.c_str();
param.extension = ".bin";

ifstream fin;
param.fin = &fin;
fin.open(Data_file_name, ios::binary);
define_sonde_type(&param);
fin.close();

sonde_type = param.sonde_type;
global_signature = param.signature;

fin.open(Data_file_name, ios::binary);
for (int n = 0; n < param.file_frames; n++) {
    variant<_4Tx_DATA, _5Tx_DATA, ALL_DATA > current_tool_data_buff;
    GP_DATA gp_data_buff;
    ALL_DATA all_data_buff;
    current_tool_data_buff = read_blok(&param, n);
    gp_data_buff = transform_data(current_tool_data_buff, &all_data_buff);
    gp_data.push_back(gp_data_buff);
}
```

### Вывод

Подтверждённая онлайн-схема старая и двухступенчатая:

- `COM` -> `_4Tx_DATA/_5Tx_DATA` -> `Dataout.bin`
- `Dataout.bin` -> `read_blok(...)` -> `transform_data(...)` -> `gp_data`

---

## 7. Как читается метрология

## 7.1. Чтение метрологии из прибора

Код из `variable.cpp`:

```cpp
variant<METROLOGY_CARTOGRAPH, METROLOGY_4Tx, METROLOGY_5Tx >
read_metro_from_tool(System::IO::Ports::SerialPort^ COM, uint32_t sonde_type, bool *read_status) {
    variant<METROLOGY_CARTOGRAPH, METROLOGY_4Tx, METROLOGY_5Tx >  metrology_struct;
    ID id;
    bool stop = false;
    bool read_ok = false;
    uint8_t D0 = 0;
    try {
        Sleep(100);
        COM->ReadTimeout = 2000;
        COM->WriteTimeout = 2000;
        COM->BaudRate = 125000;
        COM->Open();
    }
    catch (...) { stop = 1; }

    if (!stop) {
        if (sonde_type == CARTOGRAPH_ || sonde_type == AUTONOM_5Tx_ || sonde_type == AUTONOM_5Tx_SDR_) {
            send_to_tool(COM, CMD_GET_METROLOGY, &D0, 0);
            Sleep(500);
            uint8_t CMD; uint16_t readed_length = 0; uint8_t DATA_IN[1024] = { 0, };
            int result_receive_from_sonde = receive_from_tool(COM, CMD, DATA_IN, &readed_length);
            if (result_receive_from_sonde == 0) {
                id = get_sonde_id(*(uint32_t*)(DATA_IN));
                if (id.type == CARTOGRAPH_) {
                    METROLOGY_CARTOGRAPH metro;
                    memcpy(&metro, DATA_IN, sizeof(METROLOGY_CARTOGRAPH));
                    metrology_struct = metro;
                    read_ok = true;
                }
                else if (id.type == AUTONOM_5Tx_ || id.type == AUTONOM_5Tx_SDR_) {
                    METROLOGY_5Tx metro;
                    memcpy(&metro, DATA_IN, sizeof(METROLOGY_5Tx));
                    metrology_struct = metro;
                    read_ok = true;
                }
            }
        }

        if (sonde_type == LWD_4Tx_) {
            cout << "read metro from tool LWD_4Tx " << endl;
            METROLOGY_4Tx metro_4Tx;
            memset((char*)&metro_4Tx, 0x00, sizeof(metro_4Tx));
            uint8_t DATA[3];
            DATA[0] = 0x00; DATA[1] = 0x04; DATA[2] = 240;
            send_to_tool_(COM, CMD_READ_EE, DATA, 3);
            Sleep(150);
            uint8_t CMD; uint8_t DATA_IN[256] = {0,};
            int result_receive_from_sonde = receive_from_tool_(COM, CMD, DATA_IN, 240);
            if (result_receive_from_sonde == 0) {
                memcpy(&metro_4Tx, DATA_IN, 240);
                metrology_struct = metro_4Tx;
                read_ok = true;
            }
        }
    }

    *read_status = read_ok;
    COM->Close();
    return metrology_struct;
}
```

### Вывод

- `AUTONOM_5Tx` / `AUTONOM_5Tx_SDR` читают метрологию через `CMD_GET_METROLOGY`
- `LWD_4Tx` читает её другим протоколом: `CMD_READ_EE`

---

## 7.2. Приведение метрологии к общей форме `METROLOGY_GP`

Код из `variable.cpp`:

```cpp
METROLOGY_GP transform_metro(variant<METROLOGY_CARTOGRAPH, METROLOGY_4Tx, METROLOGY_5Tx > current_metro_struct ) {
    METROLOGY_GP gp_metro;

    visit([&](auto&& current) {
        using T = decay_t<decltype(current)>;
        else if constexpr (is_same_v<T, METROLOGY_4Tx>) {
            gp_metro.signature = current.signature;
            gp_metro.serial = current.serial;
            gp_metro.D_sonde_mm = current.D_sonde_mm;
            int N_Tx = 4;
            for (int Tx = 0; Tx < N_Tx; Tx++) {
                gp_metro.L1[Tx] = current.L1[Tx];
                gp_metro.L2[Tx] = current.L2[Tx];
            }
            for (int freq = 0; freq < 2; freq++) {
                gp_metro.F[freq] = current.F[4*freq];
                for (int Tx = 0; Tx < N_Tx; Tx++) {
                    gp_metro.Air_zz[freq][Tx] = current.Air_zz[Tx + 4 * freq];
                    gp_metro.Air_zz_amt[freq][Tx] = current.Air_zz_amt[Tx + 4 * freq];
                }
            }
            cout << "METROLOGY_4Tx " << endl;
        }

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
            cout << "METROLOGY_5Tx " << endl;
        }
    }, current_metro_struct);

    return gp_metro;
}
```

### Что реально используется дальше

Для графиков и расчётов используются прежде всего:

- `signature`
- `F[freq]`
- `L1[Tx]`
- `L2[Tx]`
- `Air_zz[freq][Tx]`
- `Air_zz_amt[freq][Tx]`

---

## 8. Как реализованы нужные отображаемые функции

Список доступных ненаправленных кривых в `MyForm.h`:

```cpp
comboBox_choise_chart1->Items->AddRange(gcnew cli::array< System::Object^  >(12) {
    L"PH Z2-Z1",
    L"AM Z2/Z1",
    L"AM Z1",
    L"AM Z2",
    L"AM Zero ZZ",
    L"Temp RZZ",
    L"PH_SMT_from_computer",
    L"УЭС_from_computer",
    L"PH_SMT_from_sonde",
    L"УЭС_from_sonde",
    L"Погрешность УЭС",
    "FRAME",
});
```

### 8.1. Амплитуды первого приёмника: `AM Z1`

```cpp
else if (COMBOBOX->SelectedItem == "AM Z1") {
    CHART->ChartAreas["ChartArea1"]->AxisY->Title = "mV";
    for (int Tx = 0; Tx < N_Tx; Tx++) {
        CHART->Series[Tx]->Points->AddXY(N, gp_data[n].AM_RX_1[index_freq[chart_N]][Tx] * mV);
    }
}
```

### Логика

На график выводится:

```cpp
gp_data[n].AM_RX_1[freq][Tx] * mV
```

где `mV` задаётся в зависимости от типа прибора:

```cpp
if (sonde_type == CARTOGRAPH_ || sonde_type == LWD_4Tx_) {
    N_Tx = 4; mV = 0.00023842;
}

if (sonde_type == AUTONOM_5Tx_) {
    N_Tx = 5;  mV = 0.00023842;
}

if (sonde_type == AUTONOM_5Tx_SDR_) {
    N_Tx = 5; mV = 3.6;
}
```

---

### 8.2. Амплитуды второго приёмника: `AM Z2`

```cpp
else if (COMBOBOX->SelectedItem == "AM Z2") {
    CHART->ChartAreas["ChartArea1"]->AxisY->Title = "mV";
    for (int Tx = 0; Tx < N_Tx; Tx++) {
        CHART->Series[Tx]->Points->AddXY(N, gp_data[n].AM_RX_2[index_freq[chart_N]][Tx] * mV);
    }
}
```

### Логика

На график выводится:

```cpp
gp_data[n].AM_RX_2[freq][Tx] * mV
```

---

### 8.3. Отношение амплитуд: `AM Z2/Z1`

```cpp
else if (COMBOBOX->SelectedItem == "AM Z2/Z1") {
    CHART->ChartAreas["ChartArea1"]->AxisY->Title = "";
    float L1[4] = { 358, 720, 1078, 1437 }; float L2[4] = { 598, 960, 1318, 1677 };
    CHART->Series[4]->Points->AddXY(N, pow(L1[0], 3) / pow(L2[0], 3));
    CHART->Series[5]->Points->AddXY(N, pow(L1[1], 3) / pow(L2[1], 3));
    CHART->Series[6]->Points->AddXY(N, pow(L1[2], 3) / pow(L2[2], 3));
    CHART->Series[7]->Points->AddXY(N, pow(L1[3], 3) / pow(L2[3], 3));

    CHART->Series[0]->Points->AddXY(N, gp_data[n].AM_RX_2[index_freq[chart_N]][0] / gp_data[n].AM_RX_1[index_freq[chart_N]][0]);
    CHART->Series[1]->Points->AddXY(N, gp_data[n].AM_RX_1[index_freq[chart_N]][1] / gp_data[n].AM_RX_2[index_freq[chart_N]][1]);
    CHART->Series[2]->Points->AddXY(N, gp_data[n].AM_RX_2[index_freq[chart_N]][2] / gp_data[n].AM_RX_1[index_freq[chart_N]][2]);
    CHART->Series[3]->Points->AddXY(N, gp_data[n].AM_RX_1[index_freq[chart_N]][3] / gp_data[n].AM_RX_2[index_freq[chart_N]][3]);
}
```

### Логика

Для `4Tx` отношение считается не одинаково для всех передатчиков:

- `T1`: `AM_RX_2 / AM_RX_1`
- `T2`: `AM_RX_1 / AM_RX_2`
- `T3`: `AM_RX_2 / AM_RX_1`
- `T4`: `AM_RX_1 / AM_RX_2`

Это подтверждённая старая логика отображения.

Для `5Tx` отдельной подтверждённой ветки здесь нет.

---

### 8.4. Разность фаз: `PH Z2-Z1`

```cpp
if (COMBOBOX->SelectedItem == "PH Z2-Z1") {
    CHART->ChartAreas["ChartArea1"]->AxisY->Title = "mDeg";
    for (int Tx = 0; Tx < N_Tx; Tx++) {
        CHART->Series[Tx]->Points->AddXY(N, gp_data[n].DELTA_PH[index_freq[chart_N]][Tx] * mG);
    }
    CHART->Series[10]->Points->AddXY(N, 0);
}
```

### Логика

На графике показывается `gp_data.DELTA_PH`.

Источник `DELTA_PH` зависит от прибора:

- `LWD_4Tx`: вычисляется как `PH_RX_2 - PH_RX_1`
- `AUTONOM_5Tx`: читается напрямую из `current.DELTA_PH`

---

### 8.5. Симметризованные фазы с прибора: `PH_SMT_from_sonde`

```cpp
else if (COMBOBOX->SelectedItem == "PH_SMT_from_sonde") {
    CHART->ChartAreas["ChartArea1"]->AxisY->Title = "mDeg";
    for (int Tx = 0; Tx < N_Tx; Tx++) {
        CHART->Series[Tx]->Points->AddXY(N, gp_data[n].phase_smt[index_freq[chart_N]][Tx] * mG);
    }
}
```

### Логика

Показывается то, что уже лежит в `gp_data.phase_smt`, то есть фазовая симметризация, рассчитанная не на ПК, а пришедшая в составе кадра прибора.

---

### 8.6. УЭС с прибора: `УЭС_from_sonde`

```cpp
else if (COMBOBOX->SelectedItem == "УЭС_from_sonde") {
    CHART->ChartAreas["ChartArea1"]->AxisY->Title = "Om.m";
    for (int Tx = 0; Tx < N_Tx; Tx++) {
        CHART->Series[Tx]->Points->AddXY(N, gp_data[n].rho_smt[index_freq[chart_N]][Tx]);
    }
}
```

### Логика

Показывается `gp_data.rho_smt`, то есть УЭС, уже рассчитанные контроллером прибора.

---

### 8.7. Температура: `Temp RZZ`

```cpp
else if (COMBOBOX->SelectedItem == "Temp RZZ") {
    CHART->ChartAreas["ChartArea1"]->AxisY->Title = "Celsium";
    if (gp_data[n].temperature < -50 || gp_data[n].temperature > 250)
        gp_data[n].temperature = 0;
    CHART->Series[3]->Points->AddXY(N, gp_data[n].temperature);
}
```

### Логика

Температура берётся напрямую из:

```cpp
gp_data[n].temperature
```

При значениях вне диапазона `[-50, 250]` она обнуляется.

Дополнительно температура используется для подписи текущего значения:

```cpp
if(param.file_frames - 3 > 0)
    label_T->Text = "T " + gp_data[param.file_frames - 3].temperature.ToString("F1") + "C";
if (gp_data[param.file_frames - 3].temperature > 125)  {
    PlaySound("C:\\EXP\\beep.Wav", NULL, SND_FILENAME);
}
```

---

### 8.8. Молчащие передатчики: `AM Zero ZZ`

```cpp
else if (COMBOBOX->SelectedItem == "AM Zero ZZ") {
    CHART->ChartAreas["ChartArea1"]->AxisY->Title = "mkV";
    float buff0_msd = gp_data[n].ZERO_AM_RX_1[index_freq[chart_N]] * mV*1000.0f;
    float buff1_msd = gp_data[n].ZERO_AM_RX_2[index_freq[chart_N]] * mV*1000.0f;
    sko_summ2_am_zero_zz[0] += pow((buff0_msd - sko_summ_am_zero_zz[0]), 2);
    sko_summ2_am_zero_zz[1] += pow((buff1_msd - sko_summ_am_zero_zz[1]), 2);
    CHART->Series[0]->Points->AddXY(N, buff0_msd);
    CHART->Series[1]->Points->AddXY(N, buff1_msd);
}
```

Перед этим по выборке рассчитывается среднее:

```cpp
if (COMBOBOX->SelectedItem == "AM Zero ZZ" || COMBOBOX->SelectedItem == "AM Zero ZX") {
    for (int n = min_n[chart_N]; n < max_n[chart_N]; n++) {
        sko_summ_am_zero_zz[0] += gp_data[n].ZERO_AM_RX_1[index_freq[chart_N]] * mV*1000.0f;
        sko_summ_am_zero_zz[1] += gp_data[n].ZERO_AM_RX_2[index_freq[chart_N]] * mV*1000.0f;
    }
    sko_summ_am_zero_zz[0] /= N_;
    sko_summ_am_zero_zz[1] /= N_;
}
```

### Логика

Для молчащих передатчиков в старом ненаправленном режиме отображаются именно:

- `ZERO_AM_RX_1[freq]`
- `ZERO_AM_RX_2[freq]`

в пересчёте в `mkV`.

---

### 8.9. Симметризованные фазы с компьютера: `PH_SMT_from_computer`

```cpp
else if (COMBOBOX->SelectedItem == "PH_SMT_from_computer") {
    CHART->ChartAreas["ChartArea1"]->AxisY->Title = "mDeg";
    if (metro_is_readed_from_sonde == true && metrology_GP_struct.signature == global_signature) {
        GP_DATA gp_data_buff = gp_data[n];
        uint16_t condition = get_condition(&gp_data_buff, &metrology_GP_struct, (uint16_t)index_freq[chart_N], mV);
        simmetry(&gp_data_buff, &metrology_GP_struct, (uint16_t)index_freq[chart_N], condition);
        for (int Tx = 0; Tx < N_Tx; Tx++) {
            CHART->Series[Tx]->Points->AddXY(N, gp_data_buff.phase_smt[index_freq[chart_N]][Tx] * mG);
        }
    }
}
```

### Логика

Алгоритм такой:

- взять текущий `gp_data[n]`
- определить работоспособность передатчиков через `get_condition(...)`
- пересчитать симметризованные фазы на компьютере через `simmetry(...)`
- отобразить `gp_data_buff.phase_smt`

### Важное ограничение

Функции `get_condition(...)` и `simmetry(...)` написаны для **4 передатчиков**.
Это видно по коду и по жёстким обращениям к `T1..T4`.

Следовательно, эта логика подтверждена как рабочая для `LWD_4Tx`.
Для `AUTONOM_5Tx` отдельной подтверждённой 5-канальной симметризации в старом коде не найдено.

---

### 8.10. УЭС с компьютера: `УЭС_from_computer`

```cpp
else if (COMBOBOX->SelectedItem == "УЭС_from_computer") {
    CHART->ChartAreas["ChartArea1"]->AxisY->Title = "Om.m";
    if (sonde_type == CARTOGRAPH_) {
        if (metro_is_readed_from_sonde == true && metrology_GP_struct.signature == global_signature) {
            GP_DATA gp_data_buff = gp_data[n];
            uint16_t condition = get_condition(&gp_data_buff, &metrology_GP_struct, (uint16_t)(index_freq[chart_N]), mV);
            simmetry(&gp_data_buff, &metrology_GP_struct, (uint16_t)index_freq[chart_N], condition);

            for (int Tx = 0; Tx < N_Tx; Tx++) {
                RO_ARG(&metrology_GP_struct, &gp_data_buff, Tx, (uint16_t)index_freq[chart_N]);
                CHART->Series[Tx]->Points->AddXY(N, gp_data_buff.rho_smt[index_freq[chart_N]][Tx]);
            }
        }
    }
}
```

### Важнейший факт

Хотя пункт `УЭС_from_computer` есть в меню ненаправленных кривых, в самом коде он обёрнут в условие:

```cpp
if (sonde_type == CARTOGRAPH_)
```

То есть по **подтверждённому старому коду** этот режим **не реализован для `LWD_4Tx` и `AUTONOM_5Tx`**.

Если переносить строго старую логику, то для `LWD/Автономный` в старом проекте есть:

- `PH_SMT_from_computer` частично
- `УЭС_from_computer` для них **не доведён до рабочего отображения**

---

## 9. Как работают расчётные функции на компьютере

### 9.1. Определение живых передатчиков: `get_condition(...)`

Код из `variable.cpp`:

```cpp
uint16_t get_condition(struct GP_DATA *Data, struct METROLOGY_GP *metro, uint16_t freq, float mV )
{
    float Tx_amp_val = 0.0;
    uint16_t condition = 0;
    //Tx1:
    Tx_amp_val = (Data->AM_RX_2[freq][T1])*mV; //Tx1->Rx2
    if (Tx_amp_val > metro->Air_zz_amt[freq][T1])
        condition |= (1 << (3 - T1));
    //Tx2:
    Tx_amp_val = (Data->AM_RX_1[freq][T2])*mV; //Tx2->Rx1
    if (Tx_amp_val > metro->Air_zz_amt[freq][T2])
        condition |= (1 << (3 - T2));
    //Tx3:
    Tx_amp_val = (Data->AM_RX_2[freq][T3])*mV; //Tx3->Rx2
    if (Tx_amp_val > metro->Air_zz_amt[freq][T3])
        condition |= (1 << (3 - T3));
    //Tx4:
    Tx_amp_val = (Data->AM_RX_1[freq][T4])*mV; //Tx4->Rx1
    if (Tx_amp_val > metro->Air_zz_amt[freq][T4])
        condition |= (1 << (3 - T4));

    return condition;
}
```

### Логика

Работоспособность передатчиков определяется по амплитуде сигнала на дальнем приёмнике и сравнению с метрологическим порогом `Air_zz_amt`.

Это тоже код на 4 передатчика.

---

### 9.2. Симметризация фаз: `simmetry(...)`

Код из `variable.cpp`:

```cpp
uint16_t simmetry(struct GP_DATA *Data, struct METROLOGY_GP *metro, uint16_t freq, uint16_t condition) {
    uint16_t avaria = condition;
    float K[4][4] = { 0.0, };
    float d_PH[4] = { 0.0, };
    for (int Tx = 0; Tx < 4; Tx++) {
        d_PH[Tx] = Data->DELTA_PH[freq][Tx];
    }

    for (int n = 0; n < 4; n++) {
        if (n & 1)  d_PH[n] -= ((float)(metro->Air_zz[freq][n])) / 57297.0;
        else  d_PH[n] = -d_PH[n] + ((float)(metro->Air_zz[freq][n])) / 57297.0;
    }

    uint16_t formula = formula_simmetry(K, avaria);
    for (int n = 0; n < 4; n++) {
        Data->phase_smt[freq][n] = K[n][0] * d_PH[0] + K[n][1] * d_PH[1] + K[n][2] * d_PH[2] + K[n][3] * d_PH[3];
    }
    return formula;
}
```

### Логика

Симметризация на компьютере выполняется так:

- взять `DELTA_PH`
- нормировать к воздуху через `Air_zz`
- выбрать матрицу симметризации `K`
- записать результат в `phase_smt`

Матрица `K` выбирается по числу рабочих передатчиков в `formula_simmetry(...)`.

---

### 9.3. Расчёт УЭС по симметризованной фазе: `RO_ARG(...)`

Код из `variable.cpp`:

```cpp
float SIGNAL(struct METROLOGY_GP *metrology, uint16_t n, uint16_t freq, float ro) {
    float L1 = metrology->L1[n] / 1000.0;
    float L2 = metrology->L2[n] / 1000.0;
    float omegamu0sigma = (0.0078957 * metrology->F[freq]) / ro;
    complex<float> ik = 1if * sqrt(1if * omegamu0sigma);
    complex<float> SGN = exp(ik*(L2 - L1)) * ((1.0f - ik * L2) / (1.0f - ik * L1));
    return arg(SGN);
}

int32_t RO_ARG(struct METROLOGY_GP *metrology, struct GP_DATA *Data, uint16_t n, uint16_t freq) {
    float Ro0;
    float epsilon_ARG = 0.0000005;
    float epsilon_INF = 0.000000005;
    float  Ro_0 = 0.01, Ro_max = 7000.0;
    bool stop_epsl_ARG = false;
    bool stop_epsl_INF = false;
    float  ro_0 = Ro_0;
    float  ro_max = Ro_max;

    do {
        float X1 = ro_0 + 0.382*(ro_max - ro_0);
        float X2 = ro_max - 0.382*(ro_max - ro_0);
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
    return  0;
}
```

### Логика

`RO_ARG(...)` решает обратную задачу: по симметризованной фазе `phase_smt` и метрологии вычисляет `rho_smt`.

---

## 10. Как маппить в новую `GP_TOOL_DATA`

Ниже приведён **подтверждённый** маппинг из старой логики.

## 10.1. Общие поля

```cpp
GP_TOOL_DATA.signature   <- gp_data.signature
GP_TOOL_DATA.condition   <- gp_data.condition
GP_TOOL_DATA.frame       <- gp_data.frame
GP_TOOL_DATA.temperature <- gp_data.temperature
```

## 10.2. Для `LWD_4Tx`

Рекомендуемый порядок укладки в линейные массивы:

- `400 kHz`: `T1, T2, T3, T4`
- `2000 kHz`: `T1, T2, T3, T4`

То есть:

```cpp
Ro_smt[0..3] = gp_data.rho_smt[0][0..3]
Ro_smt[4..7] = gp_data.rho_smt[1][0..3]

PH_smt[0..3] = gp_data.phase_smt[0][0..3]
PH_smt[4..7] = gp_data.phase_smt[1][0..3]
```

`AM_RX_1` и `AM_RX_2` в старом формате содержат только 8 рабочих значений. Если в новом формате массив длины 10, то по старому проекту подтверждены только первые 8 значений, а 2 оставшихся должны быть заполнены по решению нового проекта.

Для `PH_RX_1` и `PH_RX_2` у `LWD_4Tx` подтверждённый источник есть только в `_4Tx_DATA`:

```cpp
PH_RX_1[0..3] = _4Tx_data.PH_RX_1[0][0..3]
PH_RX_1[4..7] = _4Tx_data.PH_RX_1[1][0..3]

PH_RX_2[0..3] = _4Tx_data.PH_RX_2[0][0..3]
PH_RX_2[4..7] = _4Tx_data.PH_RX_2[1][0..3]
```

## 10.3. Для `AUTONOM_5Tx`

Подтверждённый порядок укладки:

- `400 kHz`: `T1, T2, T3, T4, T5`
- `2000 kHz`: `T1, T2, T3, T4, T5`

То есть:

```cpp
AM_RX_1[0..4] = gp_data.AM_RX_1[0][0..4]
AM_RX_1[5..9] = gp_data.AM_RX_1[1][0..4]

AM_RX_2[0..4] = gp_data.AM_RX_2[0][0..4]
AM_RX_2[5..9] = gp_data.AM_RX_2[1][0..4]
```

Но для `AUTONOM_5Tx`:

- `Ro_smt` в старом проекте содержит **10** значений, а не 8
- `PH_smt` в старом проекте содержит **10** значений, а не 8
- `PH_RX_1` / `PH_RX_2` в старом проекте **не подтверждены**, есть только `DELTA_PH[2][5]`

### Практический вывод

Если новая `GP_TOOL_DATA` должна одинаково обслуживать `LWD_4Tx` и `AUTONOM_5Tx`, то по подтверждённому старому коду структура в текущем виде:

- для `LWD_4Tx` подходит лучше
- для `AUTONOM_5Tx` не вмещает все старые `Ro_smt/PH_smt`
- для `AUTONOM_5Tx` не имеет подтверждённого источника для `PH_RX_1/PH_RX_2`

---

## 11. Краткий итог по тому, что реально подтверждено кодом

- `signature`, `condition`, `frame`, `temperature` читаются и доходят до `gp_data`
- `AM_RX_1`, `AM_RX_2` подтверждены для `LWD_4Tx` и `AUTONOM_5Tx`
- `DELTA_PH` подтверждён для `LWD_4Tx` и `AUTONOM_5Tx`
- `phase_smt` и `rho_smt` с прибора подтверждены для `LWD_4Tx` и `AUTONOM_5Tx`
- `PH_RX_1` и `PH_RX_2` как отдельные массивы подтверждены только для `LWD_4Tx`
- чтение файлов `.bin`, `.biv`, `.DEV` подтверждено
- работа через `COM` подтверждена
- чтение метрологии подтверждено
- отображение `AM Z1`, `AM Z2`, `AM Z2/Z1`, `PH Z2-Z1`, `PH_SMT_from_sonde`, `УЭС_from_sonde`, `Temp RZZ`, `AM Zero ZZ`, `FRAME` подтверждено
- `PH_SMT_from_computer` подтверждён как 4-канальная логика
- `УЭС_from_computer` в старом коде для `LWD/Автономный` **не реализован как рабочая ветка отображения**, потому что ограничен `if (sonde_type == CARTOGRAPH_)`

---

## 12. Главная рекомендация для переноса

Если переносить старую логику без домысливания, то лучше разделить источник данных так:

- для `LWD_4Tx` строить `GP_TOOL_DATA` из `_4Tx_DATA` + `GP_DATA`
- для `AUTONOM_5Tx` строить `GP_TOOL_DATA` из `_5Tx_DATA` + `GP_DATA`

Причина:

- `_4Tx_DATA` содержит раздельные `PH_RX_1/PH_RX_2`
- `_5Tx_DATA` содержит только `DELTA_PH`
- `GP_DATA` удобен для графиков, но он уже теряет часть сырой фазовой информации `LWD`

Если понадобится, следующим шагом можно сделать отдельный документ с **готовой схемой заполнения `GP_TOOL_DATA` по индексам** для `LWD_4Tx` и `AUTONOM_5Tx`, уже в виде псевдокода для нового проекта.
