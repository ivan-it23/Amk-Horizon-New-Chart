# Опрос прибора по COM и работа с метрологией для `_5Tx_DATA` и GP-структуры

## Назначение документа

Этот документ описывает, как в старом проекте `TEST_SONDE_KARTOGRAF_2.0` реализованы:

- определение типа прибора по `COM`
- периодический опрос прибора
- приём кадров `_5Tx_DATA`
- преобразование `_5Tx_DATA` в обобщённую GP-структуру
- чтение метрологии из прибора
- запись метрологии в прибор

Документ подготовлен как референс для переноса логики в новую версию проекта.

---

# 1. Какие структуры участвуют в логике

## 1.1. Сырая структура, приходящая от 5Tx-прибора

В старом проекте при опросе `AUTONOM_5Tx` / `AUTONOM_5Tx_SDR` из прибора читается именно `_5Tx_DATA`:

```cpp
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
```

## 1.2. Унифицированная внутренняя структура приложения

После чтения данных из файла или из прибора приложение работает с обобщённым форматом `GP_DATA`:

```cpp
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
};
```

## 1.3. Плоская GP-структура, близкая к новой модели

В старом проекте есть ещё одна структура, которая концептуально близка к твоему новому `GP_TOOL_DATA`:

```cpp
struct GP_TOOL_DATA_5_TX {
 uint32_t signature;
 uint32_t condition;
 uint32_t frame;
 float temperature;
 float Ro_smt[10];// УЭС, рассчитанные на контроллере T1_400 ... T5_2000
 float PH_smt[10];// симметризованные фазы T1_400 ... T5_2000
 float AM_RX_1[12]; // T1_400 ... T5_2000 + Tsilent_400 + Tsilent_2000
 float AM_RX_2[12]; // T1_400 ... T5_2000 + Tsilent_400 + Tsilent_2000
 float DELTA_PH[12];// T1_400 ... T5_2000 + Tsilent_400 + Tsilent_2000
};
```

## 1.4. Ключевой вывод

В старом проекте **из прибора напрямую читается `_5Tx_DATA`**, а **GP-представление не читается напрямую по COM**, а формируется локально внутри приложения.

То есть фактическая схема такая:

- прибор -> `_5Tx_DATA`
- приложение -> `transform_data(...)`
- внутреннее представление -> `GP_DATA`
- дальше уже графики, обработка, экспорт

---

# 2. Команды протокола, используемые при опросе и метрологии

В `variable.h` определены основные команды:

```cpp
#define AUTONOM_5Tx_ 151
#define  AUTONOM_5Tx_SDR_ 152
#define  LWD_4Tx_ 241

#define КEY (0x01 << 4)
#define CMD_GET_DATA 0x01 | КEY
#define CMD_GET_METROLOGY 0x03 | КEY
#define CMD_SET_METROLOGY 0x02 | КEY
#define CMD_GET_SIGNATURE 0x06 | КEY

#define CMD_READ_EE 0x65
#define CMD_WRITE_EE 0x66
#define CMD_WORK 0x67
```

Для 5Tx-ветки важны именно:

- `CMD_GET_SIGNATURE` — запрос сигнатуры прибора
- `CMD_GET_DATA` — запрос одного кадра измерений
- `CMD_GET_METROLOGY` — чтение метрологии
- `CMD_SET_METROLOGY` — запись метрологии

---

# 3. Общая логика запуска опроса

Опрос запускается из `MyForm::toolStripButton_on_off_Click(...)`.

## 3.1. Что делает код при включении

- закрывает порт, если он был открыт
- настраивает `PortName`, `BaudRate`, `ReadTimeout`, `WriteTimeout`
- открывает порт
- отправляет `CMD_GET_SIGNATURE`
- получает сигнатуру и определяет тип прибора через `get_sonde_id(...)`
- читает метрологию из прибора
- включает таймер `Timer_get_data_from_sonde`

Фрагмент логики:

```cpp
if (sonde_on_off == 0) {
    serialPort1->Close();
    serialPort1->PortName = gcnew String(Com_Name.c_str());
    Sleep(100);
    serialPort1->BaudRate = baudrate;
    serialPort1->ReadTimeout = 3500;
    serialPort1->WriteTimeout = 2000;
    serialPort1->ReadBufferSize = 10000;
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

        if (result_receive_from_sonde == 0) {
            uint32_t signature = *(uint32_t*)(DATA_IN);
            id = get_sonde_id(signature);

            if (id.type == AUTONOM_5Tx_) {
                toolStripComboBox_sonde_type->SelectedItem = "AUTONOM_5Tx";
                sonde_type = AUTONOM_5Tx_;
            }
            else if (id.type == AUTONOM_5Tx_SDR_) {
                toolStripComboBox_sonde_type->SelectedItem = "AUTONOM_5Tx_SDR";
                sonde_type = AUTONOM_5Tx_SDR_;
            }
        }

        Sleep(300);
        current_metrology_struct = read_metro_from_tool(serialPort1, sonde_type, &read_status);

        sonde_on_off = true;

        if (sonde_type == CARTOGRAPH_ || sonde_type == AUTONOM_5Tx_SDR_)
            Timer_get_data_from_sonde->Interval = 2000;
        else
            Timer_get_data_from_sonde->Interval = 1000;

        Timer_get_data_from_sonde->Enabled = true;
    }
}
```

## 3.2. Что здесь важно для переноса

- Тип прибора определяется **до начала основного опроса**.
- Для 5Tx после определения типа вызывается чтение метрологии.
- Основной опрос запускается не циклом `while`, а периодическим таймером.
- При выключении опроса таймер просто отключается.

---

# 4. Как запускается периодический опрос

Таймер создаётся один раз в `Read_Port()`:

```cpp
public:  System::Void  Read_Port() {
    Timer_get_data_from_sonde = gcnew System::Timers::Timer(2000);
    Timer_get_data_from_sonde->Elapsed += gcnew ElapsedEventHandler(this, &MyForm::timer_get_data_from_sonde);
    Timer_get_data_from_sonde->AutoReset = true;
    Timer_get_data_from_sonde->Enabled = false;
}
```

Дальше при каждом срабатывании таймера вызывается `timer_get_data_from_sonde(...)`.

---

# 5. Как именно опрашивается `AUTONOM_5Tx` / `AUTONOM_5Tx_SDR`

## 5.1. Подготовка порта перед каждым чтением

В обработчике таймера порт открывается на каждый цикл опроса:

```cpp
if (sonde_on_off == true && metro_check == false) {
    busy_read_from_sonde = true;
    try {
        serialPort1->PortName = gcnew String(Com_Name.c_str());
        Sleep(100);
        serialPort1->ReadTimeout = 2000;
        serialPort1->WriteTimeout = 2000;
        if (sonde_type == CARTOGRAPH_)
            serialPort1->ReadBufferSize = 10000;
        else if (sonde_type == LWD_4Tx_ || sonde_type == AUTONOM_5Tx_ || AUTONOM_5Tx_SDR_ == 152)
            serialPort1->ReadBufferSize = 2048;
        serialPort1->BaudRate = baudrate;
        serialPort1->Open();
    }
    catch (...) { stop = true; }
```

## 5.2. Запрос одного кадра `_5Tx_DATA`

Для 5Tx-ветки логика очень прямая:

- отправить `CMD_GET_DATA`
- дождаться ответа
- прочитать пакет через `receive_from_tool(...)`
- трактовать полезную нагрузку как `_5Tx_DATA`
- проверить тип через `signature`
- сохранить бинарный кадр в файл

Фрагмент из `timer_get_data_from_sonde(...)`:

```cpp
else if (sonde_type == AUTONOM_5Tx_ || sonde_type == AUTONOM_5Tx_SDR_) {
    uint8_t CMD; uint8_t DATA[2] = { 0x05 ,0x00 };
    uint8_t DATA_IN[2048] = { 0, }; uint16_t readed_length = 0;
    _5Tx_DATA *_5Tx_data = new _5Tx_DATA;

    int readed_from_sonde = 1;

    send_to_tool(serialPort1, CMD_GET_DATA, &D0, 0);

    Sleep(300);
    readed_from_sonde = receive_from_tool(serialPort1, CMD, DATA_IN, &readed_length);

    if (readed_from_sonde == 0) {
        _5Tx_data = (_5Tx_DATA*)(DATA_IN);
        id = get_sonde_id(_5Tx_data->signature);
        if (id.type == AUTONOM_5Tx_ || id.type == AUTONOM_5Tx_SDR_) {
            fout = new ofstream;
            fout->open(Dataout.c_str(), ios_base::app | ios_base::binary);
            fout->write((char*)_5Tx_data, sizeof(_5Tx_DATA));
            fout->flush();
            fout->close();
            delete fout;
        }
    }
}
```

## 5.3. Ключевой вывод по опросу 5Tx

Старая реализация предполагает, что полезная нагрузка ответа на `CMD_GET_DATA` для 5Tx уже соответствует бинарному layout структуры `_5Tx_DATA`.

Иначе говоря:

- нет промежуточного парсинга полей по одному
- нет побайтной распаковки каждой метрики вручную
- пакет просто приводится к `_5Tx_DATA*`

Это означает, что в новой версии проекта критично сохранить:

- тот же бинарный порядок полей
- те же размеры типов
- ту же упаковку структуры
- совместимость по endian и выравниванию

---

# 6. Как устроен пакетный протокол `send_to_tool` / `receive_from_tool`

## 6.1. Отправка пакета

Современная ветка протокола для 5Tx использует `send_to_tool(...)`:

```cpp
int send_to_tool(System::IO::Ports::SerialPort^ COM, uint8_t CMD, uint8_t* DATA, uint16_t length) {
    bool stop = false;
    uint8_t *packet = new uint8_t[length + 5];
    cli::array<Byte>^ packet_to_send = gcnew cli::array<Byte>(length + 5);
    packet[0] = CMD;
    packet_to_send[0] = CMD;

    uint16_t lenght_to_send = length + 5;
    memcpy(&packet[1], &(lenght_to_send), 2);
    packet_to_send[1] = packet[1];
    packet_to_send[2] = packet[2];

    for (int i = 3; i < length + 3; i++) {
        packet[i] = DATA[i - 3];
        packet_to_send[i] = DATA[i - 3];
    }

    uint16_t CRC_res = CRC16(packet, length + 3);
    packet_to_send[length + 3] = CRC_res;
    packet_to_send[length + 4] = CRC_res >> 8;

    COM->DiscardInBuffer();
    COM->DiscardOutBuffer();
    try {
        COM->Write(packet_to_send, 0, length + 5);
    }
    catch (...) { stop = 1; }

    if (!stop) {
        delete packet;
        delete packet_to_send;
        return 0;
    }
    return 1;
}
```

### Формат пакета

Пакет имеет вид:

- `byte 0` — `CMD`
- `byte 1..2` — полная длина пакета
- `byte 3..` — payload
- `последние 2 байта` — `CRC16`

## 6.2. Приём пакета

```cpp
int receive_from_tool(System::IO::Ports::SerialPort^ COM, uint8_t CMD, uint8_t* DATA, uint16_t *readed_length) {
    int  stop = 0;
    int rest_length_for_read = 0;
    int buff_length = 2048;
    uint8_t *packet = new uint8_t[buff_length];
    cli::array<Byte>^ packet_to_receive = gcnew cli::array<Byte>(buff_length);

    try {
        COM->Read(packet_to_receive, 0, 3);
    }
    catch (...) { stop = 1; }

    if (!stop) {
        for (int i = 0; i < 3; i++)
            packet[i] = packet_to_receive[i];
        rest_length_for_read = *(uint16_t *) &(packet[1]) - 3;
    }
    else {
        COM->DiscardInBuffer();
        COM->DiscardOutBuffer();
        readed_length = 0;
        delete packet;
        delete packet_to_receive;
        return 1;
    }

    Sleep(300);
    int readed = 0;
    try {
        readed = COM->Read(packet_to_receive, 3, rest_length_for_read);
    }
    catch (...) { stop = 1; }

    if (!stop) {
        for (int i = 3; i < rest_length_for_read + 3; i++)
            packet[i] = packet_to_receive[i];

        COM->DiscardInBuffer();
        COM->DiscardOutBuffer();

        if (CRC16(packet, rest_length_for_read + 1) == *(uint16_t *) &(packet[rest_length_for_read + 1])
            && rest_length_for_read == readed) {
            CMD = packet[0];
            for (int i = 0; i < rest_length_for_read - 2; i++)
                DATA[i] = packet_to_receive[i + 3];
            *readed_length = readed - 2;
            delete packet;
            delete packet_to_receive;
            return 0;
        }
    }

    COM->DiscardInBuffer();
    COM->DiscardOutBuffer();
    delete packet;
    delete packet_to_receive;
    return 2;
}
```

## 6.3. Практический смысл для новой реализации

Если ты переносишь только 5Tx-ветку, то тебе нужен следующий контракт:

- отправить `CMD_GET_DATA` пустым payload
- сначала считать 3 байта заголовка
- из байтов `1..2` получить длину
- дочитать остаток пакета
- проверить `CRC16`
- payload интерпретировать как `_5Tx_DATA`

---

# 7. Как `_5Tx_DATA` превращается в GP-структуру

В старом проекте это делает `transform_data(...)`.

Для ветки `_5Tx_DATA` код такой:

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

## 7.1. Вывод по преобразованию

Для 5Tx старый проект **не пересчитывает** GP-поля заново на стороне ПК, а делает почти прямое копирование:

- `signature`, `condition`, `frame`, `temperature` — копируются как есть
- массивы `rho_smt`, `phase_smt`, `AM_RX_1`, `AM_RX_2`, `DELTA_PH` — копируются как есть
- `ZERO_AM_RX_1`, `ZERO_AM_RX_2`, `ZERO_dPH` — копируются как есть

То есть для новой структуры `GP_TOOL_DATA` у тебя фактически есть два варианта:

- либо хранить данные в 2D-виде `[2][5]`, как в `_5Tx_DATA` / `GP_DATA`
- либо при приёме сразу раскладывать их в плоские массивы `10` и `12`

---

# 8. Чтение метрологии для 5Tx

Чтение метрологии вынесено в `read_metro_from_tool(...)`.

## 8.1. Логика функции

- порт открывается внутри самой функции
- для `CARTOGRAPH`, `AUTONOM_5Tx`, `AUTONOM_5Tx_SDR` используется новая ветка протокола
- отправляется `CMD_GET_METROLOGY`
- ответ принимается через `receive_from_tool(...)`
- тип структуры определяется по `signature`
- полезная нагрузка копируется в `METROLOGY_CARTOGRAPH` или `METROLOGY_5Tx`

Фрагмент:

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
    }

    *read_status = read_ok;
    COM->Close();
    return metrology_struct;
}
```

## 8.2. Важное наблюдение

Для чтения метрологии функция **жёстко выставляет `COM->BaudRate = 125000`**.

Это важно: в основной ветке опроса используется глобальный `baudrate`, а внутри чтения/записи метрологии скорость выставляется отдельно.

Если в новой версии логика прибора такая же, это нужно сохранить.

---

# 9. Запись метрологии для 5Tx

Запись выполняется через `write_metro_to_tool(...)`.

## 9.1. Что делает функция

- определяет размер структуры по типу variant
- для `CARTOGRAPH` и `AUTONOM_5Tx` отправляет структуру через `CMD_SET_METROLOGY`
- ждёт подтверждение от прибора
- в случае успеха выставляет `write_status = true`

Фрагмент:

```cpp
void write_metro_to_tool(System::IO::Ports::SerialPort^ COM,
    variant<METROLOGY_CARTOGRAPH, METROLOGY_4Tx, METROLOGY_5Tx> current_metro_struct, bool *write_status) {
    bool stop = false;
    bool write_ok = false;
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
        uint32_t sonde_type;
        uint16_t current_metro_struct_size = 0;

        visit([&](auto&& current) {
            using T = decay_t<decltype(current)>;
            if constexpr (is_same_v<T, METROLOGY_CARTOGRAPH>) { sonde_type = CARTOGRAPH_; current_metro_struct_size = sizeof(current); }
            else if constexpr (is_same_v<T, METROLOGY_4Tx>) { sonde_type = LWD_4Tx_; current_metro_struct_size = sizeof(current); }
            else if constexpr (is_same_v<T, METROLOGY_5Tx>) { sonde_type = AUTONOM_5Tx_; current_metro_struct_size = sizeof(current); }
        }, current_metro_struct);

        if (sonde_type == CARTOGRAPH_ || sonde_type == AUTONOM_5Tx_) {
            uint8_t DATA_OUT[1024];
            memcpy(DATA_OUT, &current_metro_struct, current_metro_struct_size );
            send_to_tool(COM, CMD_SET_METROLOGY, DATA_OUT, current_metro_struct_size);
            Sleep(500);
            uint8_t CMD; uint16_t income_length = 0;
            if (receive_from_tool(COM, CMD, &D0, &income_length) == 0) {
                write_ok = true;
            }
        }

        *write_status = write_ok;
        COM->Close();
    }
}
```

## 9.2. Что важно для переноса

Для 5Tx запись метрологии идёт не по `CMD_WRITE_EE`, а по новой схеме:

- payload = бинарный образ `METROLOGY_5Tx`
- команда = `CMD_SET_METROLOGY`
- подтверждение = успешный ответ на `receive_from_tool(...)`

---

# 10. Практическая схема переноса в новый проект

## 10.1. Если тебе нужен только опрос `_5Tx_DATA`

Минимальная схема такая:

- открыть `COM`
- отправить `CMD_GET_SIGNATURE`
- определить `AUTONOM_5Tx` или `AUTONOM_5Tx_SDR`
- при необходимости считать метрологию через `CMD_GET_METROLOGY`
- по таймеру или циклу отправлять `CMD_GET_DATA`
- валидировать пакет по длине и `CRC16`
- payload приводить к `_5Tx_DATA`

## 10.2. Если тебе нужен сразу `GP_TOOL_DATA`

В старом проекте это лучше всего переносить так:

- сначала принять `_5Tx_DATA`
- потом выполнить локальное преобразование в GP-представление
- уже после этого раскладывать `[2][5]` в плоские массивы твоего нового `GP_TOOL_DATA`

То есть архитектурно безопаснее оставить двухшаговую схему:

- `_5Tx_DATA` как raw transport structure
- `GP_TOOL_DATA` как application/view model

## 10.3. Рекомендуемая функция преобразования для новой версии

Логика должна быть близка к старому `transform_data(...)`:

- копировать `signature`
- копировать `condition`
- копировать `frame`
- копировать `temperature`
- разложить `rho_smt[2][5]` в `Ro_smt[10]`
- разложить `phase_smt[2][5]` в `PH_smt[10]`
- разложить `AM_RX_1[2][5]`, `AM_RX_2[2][5]`
- отдельно добавить silent-каналы `ZERO_AM_RX_1`, `ZERO_AM_RX_2`, `ZERO_dPH`, если они должны присутствовать в новой плоской модели

---

# 11. Главные выводы

- В старом проекте 5Tx опрашивается по команде `CMD_GET_DATA`.
- Ответ читается через `receive_from_tool(...)` с заголовком длины и `CRC16`.
- Payload для 5Tx напрямую приводится к `_5Tx_DATA`.
- GP-структура не приходит отдельным пакетом, а формируется на ПК через `transform_data(...)`.
- Метрология для 5Tx читается через `CMD_GET_METROLOGY` и пишется через `CMD_SET_METROLOGY`.
- В функциях метрологии используется отдельный `BaudRate = 125000`.

---

# 12. Что переносить в новую версию в первую очередь

- `send_to_tool(...)`
- `receive_from_tool(...)`
- `CRC16(...)`
- определение типа прибора через `CMD_GET_SIGNATURE` и `get_sonde_id(...)`
- приём `_5Tx_DATA` по `CMD_GET_DATA`
- преобразование `_5Tx_DATA -> GP_TOOL_DATA`
- чтение и запись `METROLOGY_5Tx`
