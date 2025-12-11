#include "SerialModule.h"
#include <setupapi.h>
#include <devguid.h>
#include <algorithm>

#pragma comment(lib, "setupapi.lib")

// CRC16 таблица (Modbus) - идентична эмулятору
const uint16_t SerialModule::CRC16_TABLE[256] = {
    0X0000, 0XC0C1, 0XC181, 0X0140, 0XC301, 0X03C0, 0X0280, 0XC241,
    0XC601, 0X06C0, 0X0780, 0XC741, 0X0500, 0XC5C1, 0XC481, 0X0440,
    0XCC01, 0X0CC0, 0X0D80, 0XCD41, 0X0F00, 0XCFC1, 0XCE81, 0X0E40,
    0X0A00, 0XCAC1, 0XCB81, 0X0B40, 0XC901, 0X09C0, 0X0880, 0XC841,
    0XD801, 0X18C0, 0X1980, 0XD941, 0X1B00, 0XDBC1, 0XDA81, 0X1A40,
    0X1E00, 0XDEC1, 0XDF81, 0X1F40, 0XDD01, 0X1DC0, 0X1C80, 0XDC41,
    0X1400, 0XD4C1, 0XD581, 0X1540, 0XD701, 0X17C0, 0X1680, 0XD641,
    0XD201, 0X12C0, 0X1380, 0XD341, 0X1100, 0XD1C1, 0XD081, 0X1040,
    0XF001, 0X30C0, 0X3180, 0XF141, 0X3300, 0XF3C1, 0XF281, 0X3240,
    0X3600, 0XF6C1, 0XF781, 0X3740, 0XF501, 0X35C0, 0X3480, 0XF441,
    0X3C00, 0XFCC1, 0XFD81, 0X3D40, 0XFF01, 0X3FC0, 0X3E80, 0XFE41,
    0XFA01, 0X3AC0, 0X3B80, 0XFB41, 0X3900, 0XF9C1, 0XF881, 0X3840,
    0X2800, 0XE8C1, 0XE981, 0X2940, 0XEB01, 0X2BC0, 0X2A80, 0XEA41,
    0XEE01, 0X2EC0, 0X2F80, 0XEF41, 0X2D00, 0XEDC1, 0XEC81, 0X2C40,
    0XE401, 0X24C0, 0X2580, 0XE541, 0X2700, 0XE7C1, 0XE681, 0X2640,
    0X2200, 0XE2C1, 0XE381, 0X2340, 0XE101, 0X21C0, 0X2080, 0XE041,
    0XA001, 0X60C0, 0X6180, 0XA141, 0X6300, 0XA3C1, 0XA281, 0X6240,
    0X6600, 0XA6C1, 0XA781, 0X6740, 0XA501, 0X65C0, 0X6480, 0XA441,
    0X6C00, 0XACC1, 0XAD81, 0X6D40, 0XAF01, 0X6FC0, 0X6E80, 0XAE41,
    0XAA01, 0X6AC0, 0X6B80, 0XAB41, 0X6900, 0XA9C1, 0XA881, 0X6840,
    0X7800, 0XB8C1, 0XB981, 0X7940, 0XBB01, 0X7BC0, 0X7A80, 0XBA41,
    0XBE01, 0X7EC0, 0X7F80, 0XBF41, 0X7D00, 0XBDC1, 0XBC81, 0X7C40,
    0XB401, 0X74C0, 0X7580, 0XB541, 0X7700, 0XB7C1, 0XB681, 0X7640,
    0X7200, 0XB2C1, 0XB381, 0X7340, 0XB101, 0X71C0, 0X7080, 0XB041,
    0X5000, 0X90C1, 0X9181, 0X5140, 0X9301, 0X53C0, 0X5280, 0X9241,
    0X9601, 0X56C0, 0X5780, 0X9741, 0X5500, 0X95C1, 0X9481, 0X5440,
    0X9C01, 0X5CC0, 0X5D80, 0X9D41, 0X5F00, 0X9FC1, 0X9E81, 0X5E40,
    0X5A00, 0X9AC1, 0X9B81, 0X5B40, 0X9901, 0X59C0, 0X5880, 0X9841,
    0X8801, 0X48C0, 0X4980, 0X8941, 0X4B00, 0X8BC1, 0X8A81, 0X4A40,
    0X4E00, 0X8EC1, 0X8F81, 0X4F40, 0X8D01, 0X4DC0, 0X4C80, 0X8C41,
    0X4400, 0X84C1, 0X8581, 0X4540, 0X8701, 0X47C0, 0X4680, 0X8641,
    0X8201, 0X42C0, 0X4380, 0X8341, 0X4100, 0X81C1, 0X8081, 0X4040
};

SerialModule::SerialModule()
    : hSerial(INVALID_HANDLE_VALUE)
    , portName(L"")
    , baudRate(125000)
    , readTimeout(1000)
    , writeTimeout(1000)
    , state(ConnectionState::DISCONNECTED)
    , timerId(0)
    , pollInterval(2000)
    , callbackWindow(nullptr)
    , hasMetrology(false)
    , packetsReceived(0)
    , packetsErrors(0)
    , lastFrameNumber(0)
{
    ZeroMemory(&metrology, sizeof(metrology));
}

SerialModule::~SerialModule() {
    StopPolling();
    Disconnect();
}

// === Управление подключением ===

SerialResult SerialModule::Connect(const std::wstring& port, DWORD baud) {
    // Если уже подключены - отключаемся
    if (hSerial != INVALID_HANDLE_VALUE) {
        Disconnect();
    }
    
    SetState(ConnectionState::CONNECTING);
    
    portName = port;
    baudRate = baud;
    
    // Формируем имя порта для CreateFile (\\.\COMx)
    std::wstring fullPortName = L"\\\\.\\" + port;
    
    // Открываем COM-порт
    hSerial = CreateFileW(
        fullPortName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,                          // Эксклюзивный доступ
        nullptr,                    // Без атрибутов безопасности
        OPEN_EXISTING,
        0,                          // Синхронный режим
        nullptr
    );
    
    if (hSerial == INVALID_HANDLE_VALUE) {
        DWORD winError = ::GetLastError();  // Явно вызываем Windows API
        if (winError == ERROR_FILE_NOT_FOUND) {
            SetState(ConnectionState::CONNECTION_ERROR, L"COM-порт не найден: " + port);
            return SerialResult::PORT_NOT_FOUND;
        }
        else if (winError == ERROR_ACCESS_DENIED) {
            SetState(ConnectionState::CONNECTION_ERROR, L"COM-порт занят: " + port);
            return SerialResult::PORT_BUSY;
        }
        else {
            SetState(ConnectionState::CONNECTION_ERROR, L"Ошибка открытия порта: " + std::to_wstring(winError));
            return SerialResult::CONNECTION_FAILED;
        }
    }
    
    // Настраиваем параметры порта
    DCB dcb = { 0 };
    dcb.DCBlength = sizeof(DCB);
    
    if (!GetCommState(hSerial, &dcb)) {
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
        SetState(ConnectionState::CONNECTION_ERROR, L"Ошибка получения состояния порта");
        return SerialResult::CONNECTION_FAILED;
    }
    
    dcb.BaudRate = baudRate;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fBinary = TRUE;
    dcb.fParity = FALSE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    
    if (!SetCommState(hSerial, &dcb)) {
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
        SetState(ConnectionState::CONNECTION_ERROR, L"Ошибка настройки параметров порта");
        return SerialResult::CONNECTION_FAILED;
    }
    
    // Настраиваем таймауты
    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = readTimeout;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = writeTimeout;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    
    if (!SetCommTimeouts(hSerial, &timeouts)) {
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
        SetState(ConnectionState::CONNECTION_ERROR, L"Ошибка настройки таймаутов");
        return SerialResult::CONNECTION_FAILED;
    }
    
    // Очищаем буферы
    PurgeBuffers();
    
    SetState(ConnectionState::CONNECTED);
    ResetStatistics();
    
    return SerialResult::SUCCESS;
}

void SerialModule::Disconnect() {
    StopPolling();
    
    if (hSerial != INVALID_HANDLE_VALUE) {
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
    }
    
    hasMetrology = false;
    SetState(ConnectionState::DISCONNECTED);
}

bool SerialModule::IsConnected() const {
    return state == ConnectionState::CONNECTED && hSerial != INVALID_HANDLE_VALUE;
}

ConnectionState SerialModule::GetState() const {
    return state;
}

std::wstring SerialModule::GetLastErrorMessage() const {
    return lastError;
}

// === Управление опросом ===

void SerialModule::StartPolling(HWND hWnd, DWORD intervalMs) {
    if (timerId != 0) {
        StopPolling();
    }
    
    callbackWindow = hWnd;
    pollInterval = intervalMs;
    
    timerId = SetTimer(hWnd, IDT_SERIAL_POLL, pollInterval, nullptr);
}

void SerialModule::StopPolling() {
    if (timerId != 0 && callbackWindow != nullptr) {
        KillTimer(callbackWindow, timerId);
        timerId = 0;
    }
}

bool SerialModule::IsPolling() const {
    return timerId != 0;
}

void SerialModule::OnTimer() {
    if (!IsConnected()) {
        return;
    }
    
    GP_TOOL_DATA data;
    SerialResult result = RequestData(data);
    
    if (result == SerialResult::SUCCESS) {
        packetsReceived++;
        lastFrameNumber = data.frame;
        
        if (onDataReceived) {
            onDataReceived(data);
        }
    }
    else {
        packetsErrors++;
    }
}

// === Команды прибора ===

SerialResult SerialModule::RequestData(GP_TOOL_DATA& outData) {
    if (!IsConnected()) {
        return SerialResult::NOT_CONNECTED;
    }
    
    // Очищаем буферы перед запросом
    PurgeBuffers();
    
    // Отправляем команду CMD_WORK (0x67)
    // Формат: [CMD:1] [length:1] [CRC:2] = 4 байта
    uint8_t cmdData = 252;  // Ожидаемая длина ответа
    if (!SendCommand(SerialCommands::CMD_WORK, &cmdData, 1)) {
        return SerialResult::CONNECTION_FAILED;
    }
    
    // Ждём немного перед чтением
    Sleep(100);
    
    // Читаем ответ
    // Формат ответа: [CMD:1] [Reserved:12] [GP_TOOL_DATA:240] [CRC:2] = 255 байт
    const size_t RESPONSE_SIZE = 255;
    uint8_t buffer[RESPONSE_SIZE];
    size_t bytesRead = 0;
    
    if (!ReceivePacket(buffer, RESPONSE_SIZE, bytesRead)) {
        return SerialResult::TIMEOUT;
    }
    
    if (bytesRead < RESPONSE_SIZE) {
        return SerialResult::INVALID_RESPONSE;
    }
    
    // Проверяем CRC (CRC считается от CMD до конца данных, без самого CRC)
    if (!ValidateCRC(buffer, RESPONSE_SIZE - 2)) {
        return SerialResult::CRC_ERROR;
    }
    
    // Данные начинаются с 13-го байта (после CMD + 12 зарезервированных)
    GP_TOOL_DATA* pData = reinterpret_cast<GP_TOOL_DATA*>(buffer + 13);
    
    // Проверяем сигнатуру
    if (!ValidateSignature(pData->signature)) {
        return SerialResult::INVALID_SIGNATURE;
    }
    
    // Копируем данные
    memcpy(&outData, pData, sizeof(GP_TOOL_DATA));
    
    return SerialResult::SUCCESS;
}

SerialResult SerialModule::RequestMetrology(METROLOGY_DATA& outData) {
    if (!IsConnected()) {
        return SerialResult::NOT_CONNECTED;
    }
    
    // Очищаем буферы перед запросом
    PurgeBuffers();
    
    // Отправляем команду CMD_READ_EE (0x65)
    if (!SendCommand(SerialCommands::CMD_READ_EE, nullptr, 0)) {
        return SerialResult::CONNECTION_FAILED;
    }
    
    // Ждём немного перед чтением
    Sleep(100);
    
    // Читаем ответ
    // Формат ответа: [CMD:1] [METROLOGY_DATA:256] [CRC:2] = 259 байт
    const size_t RESPONSE_SIZE = 259;
    uint8_t buffer[RESPONSE_SIZE];
    size_t bytesRead = 0;
    
    if (!ReceivePacket(buffer, RESPONSE_SIZE, bytesRead)) {
        return SerialResult::TIMEOUT;
    }
    
    if (bytesRead < RESPONSE_SIZE) {
        return SerialResult::INVALID_RESPONSE;
    }
    
    // Проверяем CRC
    if (!ValidateCRC(buffer, RESPONSE_SIZE - 2)) {
        return SerialResult::CRC_ERROR;
    }
    
    // Данные начинаются с 1-го байта (после CMD)
    METROLOGY_DATA* pData = reinterpret_cast<METROLOGY_DATA*>(buffer + 1);
    
    // Копируем данные
    memcpy(&outData, pData, sizeof(METROLOGY_DATA));
    
    // Кэшируем метрологию
    memcpy(&metrology, &outData, sizeof(METROLOGY_DATA));
    hasMetrology = true;
    
    if (onMetrologyReceived) {
        onMetrologyReceived(outData);
    }
    
    return SerialResult::SUCCESS;
}

// === Callbacks ===

void SerialModule::SetDataCallback(std::function<void(const GP_TOOL_DATA&)> callback) {
    onDataReceived = callback;
}

void SerialModule::SetMetrologyCallback(std::function<void(const METROLOGY_DATA&)> callback) {
    onMetrologyReceived = callback;
}

void SerialModule::SetStateCallback(std::function<void(ConnectionState, const std::wstring&)> callback) {
    onStateChanged = callback;
}

// === Статистика ===

void SerialModule::ResetStatistics() {
    packetsReceived = 0;
    packetsErrors = 0;
    lastFrameNumber = 0;
}

// === Вспомогательные методы ===

std::vector<std::wstring> SerialModule::GetAvailablePorts() {
    std::vector<std::wstring> ports;
    
    // Метод 1: Перебор COM1-COM256
    for (int i = 1; i <= 256; i++) {
        std::wstring portName = L"COM" + std::to_wstring(i);
        std::wstring fullName = L"\\\\.\\" + portName;
        
        HANDLE hTest = CreateFileW(
            fullName.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr
        );
        
        if (hTest != INVALID_HANDLE_VALUE) {
            CloseHandle(hTest);
            ports.push_back(portName);
        }
        else if (::GetLastError() == ERROR_ACCESS_DENIED) {
            // Порт существует, но занят
            ports.push_back(portName);
        }
    }
    
    return ports;
}

std::wstring SerialModule::ResultToString(SerialResult result) {
    switch (result) {
        case SerialResult::SUCCESS:           return L"Успешно";
        case SerialResult::PORT_NOT_FOUND:    return L"Порт не найден";
        case SerialResult::PORT_BUSY:         return L"Порт занят";
        case SerialResult::CONNECTION_FAILED: return L"Ошибка подключения";
        case SerialResult::TIMEOUT:           return L"Таймаут";
        case SerialResult::CRC_ERROR:         return L"Ошибка CRC";
        case SerialResult::INVALID_SIGNATURE: return L"Неверная сигнатура";
        case SerialResult::INVALID_RESPONSE:  return L"Неверный ответ";
        case SerialResult::NOT_CONNECTED:     return L"Не подключено";
        default:                              return L"Неизвестная ошибка";
    }
}

// === Низкоуровневые методы ===

bool SerialModule::SendCommand(uint8_t cmd, const uint8_t* data, size_t dataLength) {
    if (hSerial == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    // Формируем пакет: [CMD:1] [DATA:N] [CRC:2]
    size_t packetSize = 1 + dataLength + 2;
    std::vector<uint8_t> packet(packetSize);
    
    packet[0] = cmd;
    
    if (data != nullptr && dataLength > 0) {
        memcpy(&packet[1], data, dataLength);
    }
    
    // Вычисляем CRC (от CMD до конца данных)
    uint16_t crc = CalculateCRC16(packet.data(), 1 + dataLength);
    packet[1 + dataLength] = static_cast<uint8_t>(crc & 0xFF);        // Младший байт
    packet[1 + dataLength + 1] = static_cast<uint8_t>(crc >> 8);      // Старший байт
    
    // Отправляем
    DWORD bytesWritten = 0;
    if (!WriteFile(hSerial, packet.data(), static_cast<DWORD>(packetSize), &bytesWritten, nullptr)) {
        return false;
    }
    
    return bytesWritten == packetSize;
}

bool SerialModule::ReceivePacket(uint8_t* buffer, size_t bufferSize, size_t& bytesRead) {
    if (hSerial == INVALID_HANDLE_VALUE || buffer == nullptr) {
        return false;
    }
    
    DWORD totalRead = 0;
    DWORD read = 0;
    
    // Читаем данные порциями
    while (totalRead < bufferSize) {
        if (!ReadFile(hSerial, buffer + totalRead, 
                     static_cast<DWORD>(bufferSize - totalRead), &read, nullptr)) {
            break;
        }
        
        if (read == 0) {
            // Таймаут или конец данных
            break;
        }
        
        totalRead += read;
    }
    
    bytesRead = totalRead;
    return totalRead > 0;
}

uint16_t SerialModule::CalculateCRC16(const uint8_t* data, size_t length) const {
    uint16_t crc = 0xFFFF;
    
    for (size_t i = 0; i < length; i++) {
        uint8_t index = static_cast<uint8_t>(data[i] ^ crc);
        crc = (crc >> 8) ^ CRC16_TABLE[index];
    }
    
    return crc;
}

bool SerialModule::ValidateCRC(const uint8_t* packet, size_t dataLength) const {
    if (packet == nullptr || dataLength < 1) {
        return false;
    }
    
    // Вычисляем CRC от данных
    uint16_t calculatedCRC = CalculateCRC16(packet, dataLength);
    
    // Читаем CRC из пакета (2 байта после данных)
    uint16_t receivedCRC = packet[dataLength] | (packet[dataLength + 1] << 8);
    
    return calculatedCRC == receivedCRC;
}

bool SerialModule::ValidateSignature(uint32_t signature) const {
    return signature == DataSignatures::SIGNATURE_1D4S || 
           signature == DataSignatures::SIGNATURE_1D4T;
}

void SerialModule::SetState(ConnectionState newState, const std::wstring& error) {
    state = newState;
    lastError = error;
    
    if (onStateChanged) {
        onStateChanged(state, error);
    }
}

void SerialModule::PurgeBuffers() {
    if (hSerial != INVALID_HANDLE_VALUE) {
        PurgeComm(hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);
    }
}
