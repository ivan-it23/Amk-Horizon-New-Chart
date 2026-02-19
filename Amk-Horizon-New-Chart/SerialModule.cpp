#include "SerialModule.h"
#include <shellapi.h>   // Для ShellExecuteW
#include <setupapi.h>
#include <devguid.h>
#include <commdlg.h>    // Для GetSaveFileName
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <sstream>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comdlg32.lib")

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
    , metrologyAutoRetryCount(0)
    , packetsReceived(0)
    , packetsErrors(0)
    , lastFrameNumber(0)
    , isRecordingEnabled(true)  // v2.15.0: Запись включена по умолчанию
    , recordedFrames(0)
{
    ZeroMemory(&metrology, sizeof(metrology));
    InitDefaultRecordFolder();  // v2.15.0: Инициализация папки по умолчанию
}

SerialModule::~SerialModule() {
    StopPolling();
    CloseRecordFile();  // v2.15.0: Закрываем файл записи
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
    CloseRecordFile();  // v2.15.0: Закрываем файл записи при отключении
    
    if (hSerial != INVALID_HANDLE_VALUE) {
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
    }
    
    hasMetrology = false;
    metrologyAutoRetryCount = 0;
    currentDeviceId.clear();  // v2.15.0: Сбрасываем ID прибора
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
        
        // v2.15.3: Если метрология не была получена при подключении (например, Arduino reset из-за DTR/RTS),
        // пробуем запросить её здесь ограниченное число раз, чтобы имя файла записи не оставалось AMK_Unknown_...
        if (isRecordingEnabled && !hasMetrology && metrologyAutoRetryCount < 4) {
            METROLOGY_DATA tmp = {};
            (void)RequestMetrology(tmp); // результат не критичен: данные продолжают поступать
            metrologyAutoRetryCount++;
        }

        // v2.15.0: Записываем данные в файл
        if (isRecordingEnabled) {
            WriteDataToFile(data);
        }
        
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

    std::lock_guard<std::mutex> lock(ioMutex);
    
    // Очищаем буферы перед запросом
    PurgeBuffers();

    // Новый протокол: CMD + uint16 length + payload + CRC16
    // Запрос данных: CMD_DATA (0x11), payload пустой
    if (!SendPacket(SerialCommands::CMD_DATA, nullptr, 0)) {
        return SerialResult::CONNECTION_FAILED;
    }

    uint8_t cmd = 0;
    std::vector<uint8_t> payload;
    if (!ReceivePacket(cmd, payload)) {
        return SerialResult::TIMEOUT;
    }

    if (cmd != SerialCommands::CMD_DATA) {
        return SerialResult::INVALID_RESPONSE;
    }

    if (payload.size() != sizeof(GP_TOOL_DATA)) {
        return SerialResult::INVALID_RESPONSE;
    }

    memcpy(&outData, payload.data(), sizeof(GP_TOOL_DATA));
    return SerialResult::SUCCESS;
}

SerialResult SerialModule::RequestMetrology(METROLOGY_DATA& outData) {
    if (!IsConnected()) {
        return SerialResult::NOT_CONNECTED;
    }

    std::lock_guard<std::mutex> lock(ioMutex);
    PurgeBuffers();
    if (!SendPacket(SerialCommands::CMD_GET_METROLOGY, nullptr, 0)) {
        return SerialResult::CONNECTION_FAILED;
    }

    uint8_t cmd = 0;
    std::vector<uint8_t> payload;
    if (!ReceivePacket(cmd, payload)) {
        return SerialResult::TIMEOUT;
    }

    if (cmd != SerialCommands::CMD_GET_METROLOGY) {
        return SerialResult::INVALID_RESPONSE;
    }

    if (payload.size() != sizeof(METROLOGY_DATA)) {
        return SerialResult::INVALID_RESPONSE;
    }

    memcpy(&outData, payload.data(), sizeof(METROLOGY_DATA));

    memcpy(&metrology, &outData, sizeof(METROLOGY_DATA));
    hasMetrology = true;
    HandleMetrologyUpdated();

    if (onMetrologyReceived) {
        onMetrologyReceived(outData);
    }

    return SerialResult::SUCCESS;
}

SerialResult SerialModule::WriteMetrology(const METROLOGY_DATA& inData) {
    if (!IsConnected()) {
        return SerialResult::NOT_CONNECTED;
    }

    std::lock_guard<std::mutex> lock(ioMutex);
    PurgeBuffers();
    if (!SendPacket(SerialCommands::CMD_SET_METROLOGY,
                    reinterpret_cast<const uint8_t*>(&inData),
                    static_cast<uint16_t>(sizeof(METROLOGY_DATA)))) {
        return SerialResult::CONNECTION_FAILED;
    }

    uint8_t cmd = 0;
    std::vector<uint8_t> payload;
    if (!ReceivePacket(cmd, payload)) {
        return SerialResult::TIMEOUT;
    }

    if (cmd != SerialCommands::CMD_SET_METROLOGY) {
        return SerialResult::INVALID_RESPONSE;
    }

    // ACK payload can be empty; some firmwares may echo the written struct.
    METROLOGY_DATA applied = inData;
    if (!payload.empty()) {
        if (payload.size() == sizeof(METROLOGY_DATA)) {
            memcpy(&applied, payload.data(), sizeof(METROLOGY_DATA));
        } else if (payload.size() != 1) {
            return SerialResult::INVALID_RESPONSE;
        }
    }

    memcpy(&metrology, &applied, sizeof(METROLOGY_DATA));
    hasMetrology = true;
    HandleMetrologyUpdated();

    if (onMetrologyReceived) {
        onMetrologyReceived(metrology);
    }
    return SerialResult::SUCCESS;
}

SerialResult SerialModule::SetSelectedKey(uint32_t key) {
    if (!IsConnected()) {
        return SerialResult::NOT_CONNECTED;
    }

    std::lock_guard<std::mutex> lock(ioMutex);
    PurgeBuffers();
    if (!SendPacket(SerialCommands::CMD_SET_SELECTED_KEY,
                    reinterpret_cast<const uint8_t*>(&key),
                    static_cast<uint16_t>(sizeof(uint32_t)))) {
        return SerialResult::CONNECTION_FAILED;
    }

    uint8_t cmd = 0;
    std::vector<uint8_t> payload;
    if (!ReceivePacket(cmd, payload)) {
        return SerialResult::TIMEOUT;
    }

    if (cmd != SerialCommands::CMD_SET_SELECTED_KEY) {
        return SerialResult::INVALID_RESPONSE;
    }

    // ACK payload can be empty or a single status byte
    if (!payload.empty() && payload.size() != 1) {
        return SerialResult::INVALID_RESPONSE;
    }

    return SerialResult::SUCCESS;
}

SerialResult SerialModule::RequestSignature(uint32_t& outSignature) {
    if (!IsConnected()) {
        return SerialResult::NOT_CONNECTED;
    }

    std::lock_guard<std::mutex> lock(ioMutex);
    PurgeBuffers();
    if (!SendPacket(SerialCommands::CMD_GET_SIGNATURE, nullptr, 0)) {
        return SerialResult::CONNECTION_FAILED;
    }

    uint8_t cmd = 0;
    std::vector<uint8_t> payload;
    if (!ReceivePacket(cmd, payload)) {
        return SerialResult::TIMEOUT;
    }

    if (cmd != SerialCommands::CMD_GET_SIGNATURE) {
        return SerialResult::INVALID_RESPONSE;
    }

    if (payload.size() != sizeof(uint32_t)) {
        return SerialResult::INVALID_RESPONSE;
    }

    memcpy(&outSignature, payload.data(), sizeof(uint32_t));
    return SerialResult::SUCCESS;
}

void SerialModule::HandleMetrologyUpdated() {
    std::wstring newDeviceId = GetDeviceId();
    if (!currentDeviceId.empty() && currentDeviceId != newDeviceId) {
        CloseRecordFile();
    }
    currentDeviceId = newDeviceId;

    // v2.15.3: If recording started before metrology, file could be AMK_Unknown_*.bin.
    // Rename after metrology update to include S/N and diameter.
    if (recordFilePath.empty()) return;

    const std::wstring unknownPrefix = L"AMK_Unknown_";
    size_t fileNamePos = recordFilePath.find_last_of(L"\\/");
    std::wstring fileName = (fileNamePos != std::wstring::npos) ? recordFilePath.substr(fileNamePos + 1) : recordFilePath;

    if (fileName.rfind(unknownPrefix, 0) != 0) return;

    const bool wasOpen = recordFile.is_open();
    if (wasOpen) {
        CloseRecordFile();
    }

    std::wstring tail = fileName.substr(unknownPrefix.size()); // e.g. 20260209_190815.bin
    std::wstring newFileName = L"AMK_" + std::to_wstring(metrology.serial) + L"_" + std::to_wstring(metrology.D_sonde_mm) + L"_" + tail;

    std::wstring dirPath = (fileNamePos != std::wstring::npos) ? recordFilePath.substr(0, fileNamePos) : L"";
    std::wstring newPath = dirPath.empty() ? newFileName : (dirPath + L"\\" + newFileName);

    auto pathExists = [](const std::wstring& p) -> bool {
        return GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES;
    };

    if (pathExists(newPath)) {
        size_t dot = newPath.find_last_of(L'.');
        std::wstring base = (dot != std::wstring::npos) ? newPath.substr(0, dot) : newPath;
        std::wstring ext = (dot != std::wstring::npos) ? newPath.substr(dot) : L"";

        int suffix = 1;
        std::wstring candidate;
        do {
            candidate = base + L"_" + std::to_wstring(suffix) + ext;
            suffix++;
        } while (pathExists(candidate) && suffix < 1000);
        newPath = candidate;
    }

    if (MoveFileW(recordFilePath.c_str(), newPath.c_str()) != 0) {
        // v2.16.x: also rename notes sidecar, if exists
        std::wstring oldNotesPath = recordFilePath + L".notes.jsonl";
        std::wstring newNotesPath = newPath + L".notes.jsonl";
        if (GetFileAttributesW(oldNotesPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
            std::wstring targetNotes = newNotesPath;
            if (pathExists(targetNotes)) {
                size_t dot = targetNotes.find_last_of(L'.');
                std::wstring base = (dot != std::wstring::npos) ? targetNotes.substr(0, dot) : targetNotes;
                std::wstring ext = (dot != std::wstring::npos) ? targetNotes.substr(dot) : L"";

                int suffix = 1;
                std::wstring candidate;
                do {
                    candidate = base + L"_" + std::to_wstring(suffix) + ext;
                    suffix++;
                } while (pathExists(candidate) && suffix < 1000);
                targetNotes = candidate;
            }
            (void)MoveFileW(oldNotesPath.c_str(), targetNotes.c_str());
        }

        recordFilePath = newPath;
    }

    if (wasOpen) {
        std::string narrowPath(recordFilePath.begin(), recordFilePath.end());
        recordFile.open(narrowPath, std::ios::binary | std::ios::app);
    }
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
    metrologyAutoRetryCount = 0;
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

bool SerialModule::SendPacket(uint8_t cmd, const uint8_t* data, uint16_t dataLength) {
    if (hSerial == INVALID_HANDLE_VALUE) {
        return false;
    }

    // Формат: [CMD:1][LEN:2][PAYLOAD:N][CRC:2], где LEN = N + 5
    const uint16_t totalLen = static_cast<uint16_t>(dataLength + 5);
    std::vector<uint8_t> packet(totalLen);
    packet[0] = cmd;
    memcpy(&packet[1], &totalLen, sizeof(uint16_t));

    if (data != nullptr && dataLength > 0) {
        memcpy(&packet[3], data, dataLength);
    }

    const uint16_t crc = CalculateCRC16(packet.data(), static_cast<size_t>(dataLength + 3));
    packet[3 + dataLength] = static_cast<uint8_t>(crc & 0xFF);
    packet[4 + dataLength] = static_cast<uint8_t>(crc >> 8);

    DWORD bytesWritten = 0;
    if (!WriteFile(hSerial, packet.data(), static_cast<DWORD>(packet.size()), &bytesWritten, nullptr)) {
        return false;
    }

    return bytesWritten == packet.size();
}

bool SerialModule::ReceivePacket(uint8_t& outCmd, std::vector<uint8_t>& outPayload) {
    outPayload.clear();
    outCmd = 0;

    if (hSerial == INVALID_HANDLE_VALUE) {
        return false;
    }

    uint8_t header[3] = {0};
    size_t headerRead = 0;
    while (headerRead < sizeof(header)) {
        DWORD chunk = 0;
        if (!ReadFile(hSerial, header + headerRead, static_cast<DWORD>(sizeof(header) - headerRead), &chunk, nullptr)) {
            return false;
        }
        if (chunk == 0) {
            return false;
        }
        headerRead += chunk;
    }

    const uint16_t totalLen = static_cast<uint16_t>(header[1] | (static_cast<uint16_t>(header[2]) << 8));
    if (totalLen < 5 || totalLen > 4096) {
        return false;
    }

    std::vector<uint8_t> packet(totalLen);
    packet[0] = header[0];
    packet[1] = header[1];
    packet[2] = header[2];

    size_t remaining = totalLen - 3;
    DWORD totalRead = 0;
    while (totalRead < remaining) {
        DWORD chunk = 0;
        if (!ReadFile(hSerial, packet.data() + 3 + totalRead, static_cast<DWORD>(remaining - totalRead), &chunk, nullptr)) {
            return false;
        }
        if (chunk == 0) {
            return false;
        }
        totalRead += chunk;
    }

    if (!ValidateCRC(packet.data(), packet.size())) {
        return false;
    }

    outCmd = packet[0];
    const size_t payloadLen = packet.size() - 5;
    outPayload.resize(payloadLen);
    if (payloadLen > 0) {
        memcpy(outPayload.data(), packet.data() + 3, payloadLen);
    }
    return true;
}

uint16_t SerialModule::CalculateCRC16(const uint8_t* data, size_t length) const {
    uint16_t crc = 0xFFFF;
    
    for (size_t i = 0; i < length; i++) {
        uint8_t index = static_cast<uint8_t>(data[i] ^ crc);
        crc = (crc >> 8) ^ CRC16_TABLE[index];
    }
    
    return crc;
}

bool SerialModule::ValidateCRC(const uint8_t* packet, size_t packetSize) const {
    if (packet == nullptr || packetSize < 5) {
        return false;
    }

    const size_t dataLen = packetSize - 2;
    const uint16_t calculatedCRC = CalculateCRC16(packet, dataLen);
    const uint16_t receivedCRC = packet[dataLen] | (packet[dataLen + 1] << 8);
    return calculatedCRC == receivedCRC;
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

// === Реализация методов записи в файл (v2.15.0) ===

void SerialModule::SetRecordingEnabled(bool enabled) {
    isRecordingEnabled = enabled;
    
    if (!enabled) {
        CloseRecordFile();
    }
}

void SerialModule::SetRecordFolder(const std::wstring& folderPath) {
    if (folderPath.empty()) return;
    
    // Если папка изменилась и файл открыт - закрываем его
    if (recordFolderPath != folderPath && recordFile.is_open()) {
        CloseRecordFile();
    }
    
    recordFolderPath = folderPath;
    EnsureFolderExists(recordFolderPath);
}

void SerialModule::InitDefaultRecordFolder() {
    // Получаем путь к рабочему столу через переменную окружения
    wchar_t desktopPath[MAX_PATH] = { 0 };
    
    // Метод 1: Через USERPROFILE + Desktop
    DWORD len = GetEnvironmentVariableW(L"USERPROFILE", desktopPath, MAX_PATH);
    if (len > 0 && len < MAX_PATH) {
        recordFolderPath = std::wstring(desktopPath) + L"\\Desktop\\AMK_Data";
        EnsureFolderExists(recordFolderPath);
    } else {
        // Fallback - текущая директория
        recordFolderPath = L".\\AMK_Data";
        EnsureFolderExists(recordFolderPath);
    }
}

bool SerialModule::EnsureFolderExists(const std::wstring& folderPath) {
    DWORD attrs = GetFileAttributesW(folderPath.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        // Папка не существует - создаём
        return CreateDirectoryW(folderPath.c_str(), nullptr) != 0;
    }
    return (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

std::wstring SerialModule::GetDeviceId() const {
    if (!hasMetrology || metrology.serial == 0) {
        // Если метрология не получена, используем имя порта как идентификатор
        if (!portName.empty()) {
            return portName;
        }
        return L"Unknown";
    }
    
    // Формат: SN_Diameter (например: 12345_105)
    return std::to_wstring(metrology.serial) + L"_" + std::to_wstring(metrology.D_sonde_mm);
}

std::wstring SerialModule::GenerateRecordFileName() const {
    // Формат: AMK_[SN]_[Diameter]_[YYYYMMDD]_[HHMMSS].bin
    std::wstringstream ss;
    ss << L"AMK_";
    
    if (hasMetrology) {
        ss << metrology.serial << L"_" << metrology.D_sonde_mm;
    } else {
        ss << L"Unknown";
    }
    
    // Добавляем дату и время
    std::time_t now = std::time(nullptr);
    std::tm localTime;
    localtime_s(&localTime, &now);
    
    ss << L"_" 
       << std::setfill(L'0') << std::setw(4) << (localTime.tm_year + 1900)
       << std::setw(2) << (localTime.tm_mon + 1)
       << std::setw(2) << localTime.tm_mday
       << L"_"
       << std::setw(2) << localTime.tm_hour
       << std::setw(2) << localTime.tm_min
       << std::setw(2) << localTime.tm_sec
       << L".bin";
    
    std::wstring fileName = ss.str();
    std::wstring fullPath = recordFolderPath + L"\\" + fileName;
    
    // Проверяем, существует ли файл, и добавляем суффикс если нужно
    int suffix = 1;
    while (GetFileAttributesW(fullPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
        std::wstringstream suffixSs;
        suffixSs << recordFolderPath << L"\\" << fileName.substr(0, fileName.length() - 4) 
                 << L"_" << suffix << L".bin";
        fullPath = suffixSs.str();
        suffix++;
    }
    
    return fullPath;
}

bool SerialModule::OpenRecordFile() {
    if (recordFile.is_open()) {
        return true;  // Файл уже открыт
    }
    
    // Убеждаемся, что папка существует
    EnsureFolderExists(recordFolderPath);
    
    // Генерируем имя файла
    recordFilePath = GenerateRecordFileName();
    
    // Открываем файл для бинарной записи
    std::string narrowPath(recordFilePath.begin(), recordFilePath.end());
    recordFile.open(narrowPath, std::ios::binary | std::ios::app);
    
    if (!recordFile.is_open()) {
        lastError = L"Не удалось создать файл записи: " + recordFilePath;
        return false;
    }
    
    recordedFrames = 0;
    return true;
}

void SerialModule::CloseRecordFile() {
    if (recordFile.is_open()) {
        recordFile.flush();
        recordFile.close();
    }
}

void SerialModule::WriteDataToFile(const GP_TOOL_DATA& data) {
    // Открываем файл если ещё не открыт
    if (!recordFile.is_open()) {
        if (!OpenRecordFile()) {
            return;  // Не удалось открыть файл
        }
    }
    
    // Записываем структуру данных (240 байт)
    recordFile.write(reinterpret_cast<const char*>(&data), sizeof(GP_TOOL_DATA));
    recordFile.flush();  // Сразу сбрасываем на диск для надёжности
    
    recordedFrames++;
}

std::wstring SerialModule::BrowseForFolder(HWND hWnd) {

    
    wchar_t folderPath[MAX_PATH] = { 0 };
    
    // Получаем текущую папку записи как начальную
    OPENFILENAMEW ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = folderPath;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = L"Выберите папку (создайте файл placeholder.txt)";
    ofn.lpstrFilter = L"Все файлы\0*.*\0";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    
    if (GetSaveFileNameW(&ofn)) {
        // Извлекаем путь к папке из полного пути
        std::wstring fullPath = folderPath;
        size_t lastSlash = fullPath.find_last_of(L"\\/");
        if (lastSlash != std::wstring::npos) {
            return fullPath.substr(0, lastSlash);
        }
    }
    
    return L"";
}

void SerialModule::OpenRecordFolder() const {
    if (!recordFolderPath.empty()) {
        // ShellExecute доступен через windows.h
        ShellExecuteW(nullptr, L"explore", recordFolderPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
}
