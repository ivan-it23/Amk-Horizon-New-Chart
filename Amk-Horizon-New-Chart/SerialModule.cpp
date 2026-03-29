#include "SerialModule.h"
#include "ToolDataAccess.h"
#include "ToolDataAdapters.h"
#include <shellapi.h>   // Для ShellExecuteW
#include <setupapi.h>
#include <devguid.h>
#include <commdlg.h>    // Для GetSaveFileName
#include <algorithm>
#include <cwctype>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <filesystem>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comdlg32.lib")

// === Отладочный лог для 4Tx (вывод.txt) ===
namespace {
void DebugLog4Tx(const char* fmt, ...) {
    (void)fmt;
}

std::string HexDump(const uint8_t* data, size_t len, size_t maxBytes = 20) {
    (void)data;
    (void)len;
    (void)maxBytes;
    return {};
}
} // namespace

namespace {
uint32_t GetCanonicalFrameNumber(const ParsedToolFrame& parsedFrame) {
    return parsedFrame.canonicalData.frame;
}

constexpr uint32_t kAutoReconnectFailureThreshold = 3;
constexpr DWORD kAutoReconnectAttemptIntervalMs = 1000;

bool ConfigureOpenedSerialHandle(HANDLE serialHandle,
                                 DWORD baudRate,
                                 DWORD readTimeout,
                                 DWORD writeTimeout,
                                 std::wstring& outError) {
    outError.clear();

    DCB dcb = { 0 };
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(serialHandle, &dcb)) {
        outError = L"Ошибка получения состояния порта";
        return false;
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

    if (!SetCommState(serialHandle, &dcb)) {
        outError = L"Ошибка настройки параметров порта";
        return false;
    }

    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = readTimeout;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = writeTimeout;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(serialHandle, &timeouts)) {
        outError = L"Ошибка настройки таймаутов";
        return false;
    }

    return true;
}

bool OpenAppendBinaryStream(std::ofstream& stream, const std::wstring& path) {
    stream.open(std::filesystem::path(path), std::ios::binary | std::ios::app);
    return stream.is_open();
}

constexpr uint8_t kLegacyLwdDataCommand = 0x67;
constexpr uint8_t kLegacyLwdMetrologyCommand = 0x65;
constexpr size_t kLegacyLwdDataPacketSize = 255u;
constexpr size_t kLegacyLwdDataPayloadOffset = 13u;
constexpr size_t kLegacyLwdMetrologyPacketSize = 243u;
constexpr size_t kLegacyLwdMetrologyPayloadOffset = 1u;

bool WaitForBytesAvailable(HANDLE hSerial, size_t expectedBytes, DWORD waitMs) {
    if (hSerial == INVALID_HANDLE_VALUE) {
        return false;
    }

    const ULONGLONG deadline = GetTickCount64() + waitMs;
    while (GetTickCount64() < deadline) {
        COMSTAT stat = {};
        DWORD errors = 0;
        if (!ClearCommError(hSerial, &errors, &stat)) {
            return false;
        }

        if (stat.cbInQue >= expectedBytes) {
            return true;
        }

        Sleep(5);
    }

    COMSTAT stat = {};
    DWORD errors = 0;
    if (!ClearCommError(hSerial, &errors, &stat)) {
        return false;
    }

    return stat.cbInQue >= expectedBytes;
}

bool ReadExactAvailableBytes(HANDLE hSerial, size_t expectedBytes, DWORD waitMs, std::vector<uint8_t>& outBuffer) {
    outBuffer.clear();
    if (hSerial == INVALID_HANDLE_VALUE) {
        return false;
    }

    outBuffer.reserve(expectedBytes);
    const ULONGLONG deadline = GetTickCount64() + waitMs;

    while (outBuffer.size() < expectedBytes) {
        COMSTAT stat = {};
        DWORD errors = 0;
        if (!ClearCommError(hSerial, &errors, &stat)) {
            return false;
        }

        if (stat.cbInQue == 0) {
            if (GetTickCount64() >= deadline) {
                break;
            }
            Sleep(2);
            continue;
        }

        const size_t remaining = expectedBytes - outBuffer.size();
        const DWORD toRead = static_cast<DWORD>(std::min<size_t>(remaining, stat.cbInQue));
        const size_t oldSize = outBuffer.size();
        outBuffer.resize(oldSize + toRead);

        DWORD chunk = 0;
        if (!ReadFile(hSerial, outBuffer.data() + oldSize, toRead, &chunk, nullptr)) {
            return false;
        }

        if (chunk == 0) {
            outBuffer.resize(oldSize);
            if (GetTickCount64() >= deadline) {
                break;
            }
            Sleep(2);
            continue;
        }

        if (chunk < toRead) {
            outBuffer.resize(oldSize + chunk);
        }
    }

    return outBuffer.size() == expectedBytes;
}

bool ReadExactBytes(HANDLE hSerial, uint8_t* buffer, size_t expectedBytes) {
    if (hSerial == INVALID_HANDLE_VALUE || buffer == nullptr) {
        return false;
    }

    size_t totalRead = 0;
    while (totalRead < expectedBytes) {
        DWORD chunk = 0;
        if (!ReadFile(hSerial, buffer + totalRead, static_cast<DWORD>(expectedBytes - totalRead), &chunk, nullptr)) {
            return false;
        }

        if (chunk == 0) {
            return false;
        }

        totalRead += chunk;
    }

    return true;
}

bool ReceiveLegacyFixedPacket(HANDLE hSerial,
                              uint8_t expectedCmd,
                              size_t totalPacketSize,
                              DWORD settleAfterHeaderMs,
                              std::vector<uint8_t>& outPacket) {
    outPacket.clear();
    if (hSerial == INVALID_HANDLE_VALUE || totalPacketSize < 5) {
        return false;
    }

    if (!WaitForBytesAvailable(hSerial, 3, 500)) {
        return false;
    }

    uint8_t header[3] = { 0, 0, 0 };
    if (!ReadExactBytes(hSerial, header, sizeof(header))) {
        return false;
    }

    if (header[0] != expectedCmd) {
        return false;
    }

    outPacket.resize(totalPacketSize);
    outPacket[0] = header[0];
    outPacket[1] = header[1];
    outPacket[2] = header[2];

    Sleep(settleAfterHeaderMs);

    const size_t remaining = totalPacketSize - sizeof(header);
    if (remaining > 0 && !WaitForBytesAvailable(hSerial, remaining, 500)) {
        outPacket.clear();
        return false;
    }

    if (!ReadExactBytes(hSerial, outPacket.data() + sizeof(header), remaining)) {
        outPacket.clear();
        return false;
    }

    return true;
}

void DispatchDataCallbacks(const ParsedToolFrame& parsedFrame,
                           const std::function<void(const ParsedToolFrame&)>& onParsedDataReceived,
                           const std::function<void(const GP_TOOL_DATA&)>& onDataReceived) {
    if (onParsedDataReceived) {
        onParsedDataReceived(parsedFrame);
    }

    if (onDataReceived) {
        GP_TOOL_DATA data = {};
        (void)TryExtractCanonicalData(parsedFrame, data);
        onDataReceived(data);
    }
}

#pragma pack(push, 1)
struct LEGACY_LWD_METROLOGY_DATA {
    uint32_t signature;
    uint32_t serial;
    uint16_t L1[8];
    uint16_t L2[8];
    uint16_t F[8];
    int16_t Air_zz[8];
    uint16_t PhaseOffsets[4];
    uint32_t D_sonde_mm;
    int16_t Air_zz_amt[8];
    uint8_t service[140];
};
#pragma pack(pop)

static_assert(sizeof(LEGACY_LWD_METROLOGY_DATA) == 240, "LEGACY_LWD_METROLOGY_DATA must be exactly 240 bytes");

bool IsLegacyLwd4TxCompatibilitySignature(uint32_t signature) {
    return ToolDataAdapterRegistry::DecodeToolCode(signature) == 241u ||
           signature == 0x3143ADD4u ||
           signature == 0x31443454u ||
           signature == 0x31444453u ||
           signature == 0x31443453u;
}

bool TryParseLegacyLwd4TxFrame(const void* rawFrame, size_t rawFrameSize, ParsedToolFrame& outFrame) {
    outFrame = {};
    if (!rawFrame || rawFrameSize != sizeof(LEGACY_GP_TOOL_DATA)) {
        return false;
    }

    uint32_t signature = 0;
    memcpy(&signature, rawFrame, sizeof(signature));
    if (!IsLegacyLwd4TxCompatibilitySignature(signature)) {
        return false;
    }

    Lwd4TxAdapter adapter;
    return adapter.ParseRawFrame(rawFrame, rawFrameSize, outFrame);
}

void ConvertLegacyLwd4TxMetrology(const LEGACY_LWD_METROLOGY_DATA& legacyData, METROLOGY_DATA& outData) {
    outData = {};
    outData.signature = legacyData.signature;
    outData.serial = legacyData.serial;
    outData.D_sonde_mm = legacyData.D_sonde_mm;

    for (int txIdx = 0; txIdx < 4; ++txIdx) {
        outData.L1[txIdx] = legacyData.L1[txIdx];
        outData.L2[txIdx] = legacyData.L2[txIdx];
    }

    for (int freqIdx = 0; freqIdx < 2; ++freqIdx) {
        outData.F[freqIdx] = legacyData.F[freqIdx * 4];
        for (int txIdx = 0; txIdx < 4; ++txIdx) {
            const int legacyIndex = txIdx + freqIdx * 4;
            const int canonicalIndex = txIdx + freqIdx * 5;
            outData.Air_zz[canonicalIndex] = legacyData.Air_zz[legacyIndex];
            outData.Air_zz_amt[canonicalIndex] = legacyData.Air_zz_amt[legacyIndex];
        }
    }
}
}

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
    , protocolMode(DeviceProtocolMode::UNKNOWN)
    , timerId(0)
    , pollInterval(2000)
    , callbackWindow(nullptr)
    , hasMetrology(false)
    , metrologyAutoRetryCount(0)
    , packetsReceived(0)
    , packetsErrors(0)
    , lastFrameNumber(0)
    , sessionFirstFrameNumber(0)
    , hasSessionFirstFrameNumber(false)
    , legacyPollInterval(1000)
    , stopLegacyPolling(false)
    , pausePolling(false)
    , consecutivePollFailures(0)
    , nextReconnectAttemptTick(0)
    , connectionSessionId(0)
    , existingRecordFilePath(L"")
    , isRecordingEnabled(true)  // v2.15.0: Запись включена по умолчанию
    , appendToExistingFile(false)
    , recordedFrames(0)
{
    ZeroMemory(&metrology, sizeof(metrology));
    lastRawDataFrame.clear();
    InitDefaultRecordFolder();  // v2.15.0: Инициализация папки по умолчанию
}

bool SerialModule::ValidateExistingRecordFile(const std::wstring& filePath, std::wstring& outError) const {
    outError.clear();

    if (filePath.empty()) {
        outError = L"Не выбран файл для дозаписи";
        return false;
    }

    DWORD attrs = GetFileAttributesW(filePath.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES || (attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        outError = L"Выбранный файл не существует";
        return false;
    }

    size_t dotPos = filePath.find_last_of(L'.');
    if (dotPos == std::wstring::npos) {
        outError = L"Файл должен иметь расширение .bin или .biv";
        return false;
    }

    std::wstring ext = filePath.substr(dotPos);
    std::transform(ext.begin(), ext.end(), ext.begin(), towlower);
    if (ext != L".bin" && ext != L".biv") {
        outError = L"Поддерживаются только файлы .bin и .biv";
        return false;
    }

    HANDLE hFile = CreateFileW(
        filePath.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        outError = L"Не удалось открыть файл для проверки";
        return false;
    }

    LARGE_INTEGER fileSize = {};
    BOOL sizeOk = GetFileSizeEx(hFile, &fileSize);
    CloseHandle(hFile);

    if (!sizeOk || fileSize.QuadPart < 0) {
        outError = L"Не удалось определить размер файла";
        return false;
    }

    if ((fileSize.QuadPart % static_cast<LONGLONG>(sizeof(GP_TOOL_DATA))) != 0) {
        outError = L"Размер файла не кратен размеру кадра GP_TOOL_DATA (240 байт)";
        return false;
    }

    return true;
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
    
    std::wstring setupError;
    if (!ConfigureOpenedSerialHandle(hSerial, baudRate, readTimeout, writeTimeout, setupError)) {
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
        SetState(ConnectionState::CONNECTION_ERROR, setupError);
        return SerialResult::CONNECTION_FAILED;
    }
    
    // Очищаем буферы
    PurgeBuffers();
    protocolMode = DeviceProtocolMode::UNKNOWN;
    consecutivePollFailures = 0;
    nextReconnectAttemptTick = 0;
    ++connectionSessionId;
    
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
    
    consecutivePollFailures = 0;
    nextReconnectAttemptTick = 0;
    connectionSessionId = 0;
    hasMetrology = false;
    metrologyAutoRetryCount = 0;
    protocolMode = DeviceProtocolMode::UNKNOWN;
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
    if (timerId != 0 || legacyPollingThread.joinable()) {
        StopPolling();
    }
    
    callbackWindow = hWnd;
    pollInterval = intervalMs;
    legacyPollInterval = intervalMs;

    {
        std::lock_guard<std::mutex> lock(pendingPollEventsMutex);
        pendingPollEvents.clear();
    }

    if (protocolMode == DeviceProtocolMode::LEGACY_4TX &&
        (legacyPollInterval == 0 || legacyPollInterval > 1000)) {
        legacyPollInterval = 1000;
    }

    DebugLog4Tx("=== StartPolling: mode=%d, interval=%lu ms ===",
                static_cast<int>(protocolMode), legacyPollInterval);
    stopLegacyPolling = false;
    pausePolling = false;
    timerId = SetTimer(hWnd, IDT_SERIAL_POLL, 50, nullptr);
    legacyPollingThread = std::thread(&SerialModule::LegacyPollingLoop, this);
}

void SerialModule::SetPollInterval(DWORD intervalMs) {
    if (intervalMs < 500) intervalMs = 500;    // минимум 0.5 сек
    if (intervalMs > 30000) intervalMs = 30000; // максимум 30 сек
    pollInterval = intervalMs;
    legacyPollInterval = intervalMs;
    if (protocolMode == DeviceProtocolMode::LEGACY_4TX &&
        (legacyPollInterval == 0 || legacyPollInterval > 1000)) {
        legacyPollInterval = 1000;
    }
}

void SerialModule::StopPolling() {
    stopLegacyPolling = true;

    if (timerId != 0 && callbackWindow != nullptr) {
        KillTimer(callbackWindow, timerId);
        timerId = 0;
    }

    if (legacyPollingThread.joinable()) {
        legacyPollingThread.join();
    }

    {
        std::lock_guard<std::mutex> lock(pendingPollEventsMutex);
        pendingPollEvents.clear();
    }
}

bool SerialModule::IsPolling() const {
    return timerId != 0;
}

void SerialModule::OnTimer() {
    if (legacyPollingThread.joinable()) {
        FlushPendingPollResults();
    }
}

void SerialModule::FlushPendingPollResults() {
    std::deque<PendingPollEvent> events;
    size_t queueSizeBefore = 0;
    {
        std::lock_guard<std::mutex> lock(pendingPollEventsMutex);
        queueSizeBefore = pendingPollEvents.size();
        events.swap(pendingPollEvents);
    }

    if (!events.empty()) {
        DebugLog4Tx("FlushPendingPollResults: processing %zu events (queue was %zu)",
                     events.size(), queueSizeBefore);
    }

    int processedCount = 0;
    int droppedCount = 0;
    for (const PendingPollEvent& event : events) {
        if (event.result == SerialResult::SUCCESS) {
            uint32_t frameNum = GetCanonicalFrameNumber(event.parsedFrame);
            uint32_t currentSession = connectionSessionId.load();
            if (event.connectionSessionId != currentSession) {
                DebugLog4Tx("FlushPendingPollResults: DROP frame=%u (session mismatch: event=%u, current=%u)",
                             frameNum, event.connectionSessionId, currentSession);
                droppedCount++;
                continue;
            }
            DebugLog4Tx("FlushPendingPollResults: processing frame=%u (session=%u)",
                         frameNum, event.connectionSessionId);
            HandleSuccessfulPoll(event.parsedFrame, event.rawPayload);
            processedCount++;
        } else {
            if (IsConnected() && event.connectionSessionId != connectionSessionId.load()) {
                DebugLog4Tx("FlushPendingPollResults: DROP failed poll (session mismatch)");
                droppedCount++;
                continue;
            }
            HandleFailedPoll(event.result);
        }
    }

    if (events.size() > 0) {
        DebugLog4Tx("FlushPendingPollResults: done, processed=%d, dropped=%d",
                     processedCount, droppedCount);
    }
}

void SerialModule::LegacyPollingLoop() {
    while (!stopLegacyPolling) {
        const DWORD sleepInterval = (protocolMode == DeviceProtocolMode::LEGACY_4TX)
            ? legacyPollInterval
            : pollInterval;

        // Polling приостановлен (метрологическая операция в процессе)
        if (pausePolling) {
            Sleep(50);
            continue;
        }

        if (!IsConnected()) {
            if (!TryReconnectCurrentPort()) {
                PendingPollEvent event;
                event.result = SerialResult::NOT_CONNECTED;
                event.connectionSessionId = connectionSessionId.load();
                {
                    std::lock_guard<std::mutex> lock(pendingPollEventsMutex);
                    pendingPollEvents.push_back(std::move(event));
                    if (pendingPollEvents.size() > 8) {
                        pendingPollEvents.pop_front();
                    }
                }

                DWORD sleptWhileDisconnected = 0;
                while (!stopLegacyPolling && sleptWhileDisconnected < sleepInterval) {
                    const DWORD step = std::min<DWORD>(50, sleepInterval - sleptWhileDisconnected);
                    Sleep(step);
                    sleptWhileDisconnected += step;
                }
                continue;
            }
        }

        PendingPollEvent event;
        event.result = SerialResult::TIMEOUT;
        event.connectionSessionId = connectionSessionId.load();

        ParsedToolFrame parsedFrame = {};
        std::vector<uint8_t> rawPayload;
        event.result = RequestData(parsedFrame, &rawPayload);
        if (event.result == SerialResult::SUCCESS) {
            event.parsedFrame = parsedFrame;
            event.rawPayload = std::move(rawPayload);
            DebugLog4Tx("LegacyPollingLoop: poll SUCCESS, frame=%u",
                         GetCanonicalFrameNumber(parsedFrame));
        } else {
            DebugLog4Tx("LegacyPollingLoop: poll FAILED, result=%d",
                         static_cast<int>(event.result));
        }

        {
            std::lock_guard<std::mutex> lock(pendingPollEventsMutex);
            size_t queueSizeBefore = pendingPollEvents.size();
            
            // Если буфер переполнен (8+ элементов), удаляем самый старый
            if (queueSizeBefore >= 8) {
                uint32_t droppedFrame = GetCanonicalFrameNumber(pendingPollEvents.front().parsedFrame);
                SerialResult droppedResult = pendingPollEvents.front().result;
                pendingPollEvents.pop_front();
                queueSizeBefore--;
                DebugLog4Tx("LegacyPollingLoop: QUEUE OVERFLOW! Dropped frame=%u (result=%d)",
                             droppedFrame, static_cast<int>(droppedResult));
            }
            
            // Сохраняем frame number до move
            uint32_t addedFrame = (event.result == SerialResult::SUCCESS) 
                ? GetCanonicalFrameNumber(event.parsedFrame) 
                : 0;
            
            pendingPollEvents.push_back(std::move(event));
            
            if (addedFrame > 0) {
                DebugLog4Tx("LegacyPollingLoop: queued frame=%u (queue: %zu -> %zu)",
                             addedFrame, queueSizeBefore, pendingPollEvents.size());
            }
        }

        DWORD slept = 0;
        while (!stopLegacyPolling && slept < sleepInterval) {
            const DWORD step = std::min<DWORD>(50, sleepInterval - slept);
            Sleep(step);
            slept += step;
        }
    }
}

void SerialModule::HandleSuccessfulPoll(const ParsedToolFrame& parsedFrame, const std::vector<uint8_t>& rawPayload) {
    consecutivePollFailures = 0;
    packetsReceived++;
    lastFrameNumber = GetCanonicalFrameNumber(parsedFrame);
    if (!hasSessionFirstFrameNumber) {
        sessionFirstFrameNumber = lastFrameNumber;
        hasSessionFirstFrameNumber = true;
    }
    DebugLog4Tx("HandleSuccessfulPoll: frame=%u, received=%u, errors=%u, recording=%d, hasMetro=%d",
                 lastFrameNumber, packetsReceived, packetsErrors,
                 (int)isRecordingEnabled, (int)hasMetrology);

    if (isRecordingEnabled && !hasMetrology && metrologyAutoRetryCount < 4) {
        DebugLog4Tx("HandleSuccessfulPoll: auto-retry metrology #%u", metrologyAutoRetryCount);
        METROLOGY_DATA tmp = {};
        (void)RequestMetrology(tmp);
        metrologyAutoRetryCount++;
    }

    if (isRecordingEnabled) {
        std::wstring settingsError;
        if (!ValidateRecordingSettings(settingsError)) {
            lastError = settingsError;
        } else {
            // Записываем canonical GP_TOOL_DATA (240 байт, 2D-массивы),
            // а не raw LEGACY_GP_TOOL_DATA (flat-массивы), чтобы файлы
            // были совместимы с загрузкой через ParseRawFrameDetailed.
            WriteDataToFile(&parsedFrame.canonicalData, sizeof(GP_TOOL_DATA));
        }
    }

    DispatchDataCallbacks(parsedFrame, onParsedDataReceived, onDataReceived);
}

void SerialModule::HandleFailedPoll(SerialResult result) {
    packetsErrors++;
    const uint32_t failures = ++consecutivePollFailures;
    if (ShouldTriggerAutoReconnect(result) && failures >= kAutoReconnectFailureThreshold) {
        InvalidateConnectionForRecovery();
    }
}

bool SerialModule::ShouldTriggerAutoReconnect(SerialResult result) const {
    return result == SerialResult::TIMEOUT ||
           result == SerialResult::CONNECTION_FAILED ||
           result == SerialResult::NOT_CONNECTED;
}

void SerialModule::InvalidateConnectionForRecovery() {
    if (hSerial != INVALID_HANDLE_VALUE) {
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
    }

    hasMetrology = false;
    metrologyAutoRetryCount = 0;
    protocolMode = DeviceProtocolMode::UNKNOWN;
    nextReconnectAttemptTick = GetTickCount() + kAutoReconnectAttemptIntervalMs;
    SetState(ConnectionState::DISCONNECTED);
}

bool SerialModule::TryReconnectCurrentPort() {
    if (stopLegacyPolling || portName.empty()) {
        return false;
    }

    const DWORD now = GetTickCount();
    const DWORD nextAttempt = nextReconnectAttemptTick.load();
    if (nextAttempt != 0 && static_cast<DWORD>(now - nextAttempt) < 0x80000000UL && now < nextAttempt) {
        return false;
    }

    nextReconnectAttemptTick = now + kAutoReconnectAttemptIntervalMs;

    std::wstring fullPortName = L"\\\\.\\" + portName;
    HANDLE reopened = CreateFileW(
        fullPortName.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );

    if (reopened == INVALID_HANDLE_VALUE) {
        return false;
    }

    std::wstring setupError;
    if (!ConfigureOpenedSerialHandle(reopened, baudRate, readTimeout, writeTimeout, setupError)) {
        CloseHandle(reopened);
        return false;
    }

    hSerial = reopened;
    PurgeBuffers();
    protocolMode = DeviceProtocolMode::UNKNOWN;
    hasMetrology = false;
    metrologyAutoRetryCount = 0;
    consecutivePollFailures = 0;
    nextReconnectAttemptTick = 0;
    ++connectionSessionId;
    SetState(ConnectionState::CONNECTED);
    return true;
}

// === Команды прибора ===

SerialResult SerialModule::RequestData(ParsedToolFrame& outParsedFrame) {
    return RequestData(outParsedFrame, nullptr);
}

SerialResult SerialModule::RequestData(ParsedToolFrame& outParsedFrame, std::vector<uint8_t>* outRawPayload) {
    outParsedFrame = {};
    if (!IsConnected()) {
        return SerialResult::NOT_CONNECTED;
    }

    std::lock_guard<std::mutex> lock(ioMutex);

    auto tryStoreParsedFrame = [this, outRawPayload, &outParsedFrame](const uint8_t* rawFrame, size_t rawFrameSize) -> bool {
        if (!rawFrame || rawFrameSize != sizeof(LEGACY_GP_TOOL_DATA)) {
            return false;
        }

        ParsedToolFrame parsedFrame = {};
        bool parsed = ToolDataAdapterRegistry::Get().ParseRawFrame(rawFrame, rawFrameSize, parsedFrame);
        if (!parsed) {
            parsed = TryParseLegacyLwd4TxFrame(rawFrame, rawFrameSize, parsedFrame);
        }
        if (!parsed) {
            return false;
        }

        outParsedFrame = parsedFrame;
        lastRawDataFrame.assign(rawFrame, rawFrame + rawFrameSize);
        if (outRawPayload) {
            *outRawPayload = lastRawDataFrame;
        }
        return true;
    };

    auto requestModernData = [this, outRawPayload, &outParsedFrame]() -> SerialResult {
        PurgeBuffers();

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

        if (!ToolDataAdapterRegistry::Get().ParseRawFrame(payload.data(), payload.size(), outParsedFrame)) {
            return SerialResult::INVALID_SIGNATURE;
        }

        lastRawDataFrame = payload;
        if (outRawPayload) {
            *outRawPayload = payload;
        }

        return SerialResult::SUCCESS;
    };

    auto requestLegacyData = [this, &tryStoreParsedFrame]() -> SerialResult {
        PurgeBuffers();

        // Формат запроса данных: [CMD=0x67][0x00][0x04][0xF0][CRC16:2] = 6 байт
        // Идентичен старому коду: send_to_tool_(COM, 0x67, 0x00, 0x04, 0xF0)
        const uint8_t payload[] = { 0x00, 0x04, 0xF0 };
        std::vector<uint8_t> request(1 + sizeof(payload) + sizeof(uint16_t));
        request[0] = kLegacyLwdDataCommand;
        memcpy(request.data() + 1, payload, sizeof(payload));
        const uint16_t crc = CalculateCRC16(request.data(), 1 + sizeof(payload));
        request[1 + sizeof(payload)] = static_cast<uint8_t>(crc & 0xFF);
        request[2 + sizeof(payload)] = static_cast<uint8_t>(crc >> 8);

        DebugLog4Tx("requestLegacyData: sending %zu bytes: %s",
                     request.size(), HexDump(request.data(), request.size()).c_str());

        DWORD bytesWritten = 0;
        if (!WriteFile(hSerial, request.data(), static_cast<DWORD>(request.size()), &bytesWritten, nullptr) ||
            bytesWritten != request.size()) {
            DebugLog4Tx("requestLegacyData: WriteFile FAILED, written=%lu", bytesWritten);
            return SerialResult::CONNECTION_FAILED;
        }

        std::vector<uint8_t> legacyResponse;
        if (!ReceiveLegacyFixedPacket(hSerial, kLegacyLwdDataCommand, kLegacyLwdDataPacketSize, 300, legacyResponse)) {
            DebugLog4Tx("requestLegacyData: ReceiveLegacyFixedPacket TIMEOUT (expected %zu bytes)",
                         kLegacyLwdDataPacketSize);
            return SerialResult::TIMEOUT;
        }

        DebugLog4Tx("requestLegacyData: received %zu bytes, header: %s",
                     legacyResponse.size(),
                     HexDump(legacyResponse.data(), legacyResponse.size(), 16).c_str());

        if (!ValidateCRC(legacyResponse.data(), legacyResponse.size())) {
            DebugLog4Tx("requestLegacyData: CRC_ERROR");
            return SerialResult::CRC_ERROR;
        }

        const size_t candidateOffsets[] = { kLegacyLwdDataPayloadOffset, 12u };
        for (size_t offset : candidateOffsets) {
            if (legacyResponse.size() >= offset + sizeof(LEGACY_GP_TOOL_DATA) &&
                tryStoreParsedFrame(legacyResponse.data() + offset, sizeof(LEGACY_GP_TOOL_DATA))) {
                uint32_t sig = 0;
                memcpy(&sig, legacyResponse.data() + offset, sizeof(sig));
                DebugLog4Tx("requestLegacyData: SUCCESS at offset=%zu, signature=0x%08X",
                             offset, sig);
                return SerialResult::SUCCESS;
            }
        }

        DebugLog4Tx("requestLegacyData: INVALID_SIGNATURE (no adapter matched)");
        return SerialResult::INVALID_SIGNATURE;
    };

    if (protocolMode == DeviceProtocolMode::MODERN_5TX) {
        return requestModernData();
    }

    if (protocolMode == DeviceProtocolMode::LEGACY_4TX) {
        return requestLegacyData();
    }

    DebugLog4Tx("RequestData: protocolMode=UNKNOWN, trying modern first");
    const SerialResult modernResult = requestModernData();
    if (modernResult == SerialResult::SUCCESS) {
        protocolMode = DeviceProtocolMode::MODERN_5TX;
        return modernResult;
    }

    // Modern не удался — сбрасываем буфер устройства
    DebugLog4Tx("RequestData: modern failed, flushing device buffer");
    FlushDeviceBufferAfterModernAttempt();

    const SerialResult legacyResult = requestLegacyData();
    DebugLog4Tx("RequestData: legacy result=%d", static_cast<int>(legacyResult));
    if (legacyResult == SerialResult::SUCCESS) {
        protocolMode = DeviceProtocolMode::LEGACY_4TX;
        return legacyResult;
    }

    return modernResult != SerialResult::SUCCESS ? modernResult : legacyResult;
}

SerialResult SerialModule::RequestData(GP_TOOL_DATA& outData) {
    return RequestData(outData, nullptr);
}

SerialResult SerialModule::RequestData(GP_TOOL_DATA& outData, std::vector<uint8_t>* outRawPayload) {
    ParsedToolFrame parsedFrame = {};
    const SerialResult result = RequestData(parsedFrame, outRawPayload);
    if (result != SerialResult::SUCCESS) {
        outData = {};
        return result;
    }

    (void)TryExtractCanonicalData(parsedFrame, outData);
    return result;
}

SerialResult SerialModule::RequestMetrology(METROLOGY_DATA& outData) {
    if (!IsConnected()) {
        return SerialResult::NOT_CONNECTED;
    }

    // Приостанавливаем polling на время чтения метрологии, чтобы избежать
    // конфликтов на COM-порте (аналог metro_check в старом приложении).
    const bool wasPaused = pausePolling.exchange(true);

    SerialResult finalResult = SerialResult::TIMEOUT;
    {
        std::lock_guard<std::mutex> lock(ioMutex);

    auto applyMetrology = [this, &outData](const METROLOGY_DATA& data) {
        outData = data;
        memcpy(&metrology, &outData, sizeof(METROLOGY_DATA));
        hasMetrology = true;
        HandleMetrologyUpdated();

        if (onMetrologyReceived) {
            onMetrologyReceived(outData);
        }
    };

    auto requestModernMetrology = [this, &applyMetrology]() -> SerialResult {
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

        METROLOGY_DATA directData = {};
        memcpy(&directData, payload.data(), sizeof(METROLOGY_DATA));
        applyMetrology(directData);
        return SerialResult::SUCCESS;
    };

    auto requestLegacyMetrology = [this, &applyMetrology]() -> SerialResult {
        PurgeBuffers();

        const uint8_t legacyPayload[] = { 0x00, 0x04, 240 };
        std::vector<uint8_t> legacyRequest(1 + sizeof(legacyPayload) + sizeof(uint16_t));
        legacyRequest[0] = 0x65;
        memcpy(legacyRequest.data() + 1, legacyPayload, sizeof(legacyPayload));
        const uint16_t legacyRequestCrc = CalculateCRC16(legacyRequest.data(), 1 + sizeof(legacyPayload));
        legacyRequest[1 + sizeof(legacyPayload)] = static_cast<uint8_t>(legacyRequestCrc & 0xFF);
        legacyRequest[2 + sizeof(legacyPayload)] = static_cast<uint8_t>(legacyRequestCrc >> 8);

        DebugLog4Tx("requestLegacyMetrology: sending %zu bytes: %s",
                     legacyRequest.size(), HexDump(legacyRequest.data(), legacyRequest.size()).c_str());

        DWORD bytesWritten = 0;
        if (!WriteFile(hSerial, legacyRequest.data(), static_cast<DWORD>(legacyRequest.size()), &bytesWritten, nullptr) ||
            bytesWritten != legacyRequest.size()) {
            DebugLog4Tx("requestLegacyMetrology: WriteFile FAILED");
            return SerialResult::CONNECTION_FAILED;
        }

        std::vector<uint8_t> legacyResponse;
        Sleep(150);
        if (!ReadExactAvailableBytes(hSerial, kLegacyLwdMetrologyPacketSize, 1500, legacyResponse)) {
            DebugLog4Tx("requestLegacyMetrology: TIMEOUT (got %zu of %zu bytes)",
                         legacyResponse.size(), kLegacyLwdMetrologyPacketSize);
            return SerialResult::TIMEOUT;
        }

        DebugLog4Tx("requestLegacyMetrology: received %zu bytes, header: %s",
                     legacyResponse.size(),
                     HexDump(legacyResponse.data(), legacyResponse.size(), 16).c_str());

        Sleep(20);

        if (legacyResponse.empty() || legacyResponse[0] != kLegacyLwdMetrologyCommand) {
            DebugLog4Tx("requestLegacyMetrology: INVALID_RESPONSE (first byte=0x%02X, expected 0x%02X)",
                         legacyResponse.empty() ? 0 : legacyResponse[0], kLegacyLwdMetrologyCommand);
            return SerialResult::INVALID_RESPONSE;
        }

        if (!ValidateCRC(legacyResponse.data(), legacyResponse.size())) {
            DebugLog4Tx("requestLegacyMetrology: CRC_ERROR");
            return SerialResult::CRC_ERROR;
        }

        const size_t candidateOffsets[] = { kLegacyLwdMetrologyPayloadOffset, 0u };
        for (size_t offset : candidateOffsets) {
            if (legacyResponse.size() < offset + sizeof(LEGACY_LWD_METROLOGY_DATA)) {
                continue;
            }

            LEGACY_LWD_METROLOGY_DATA legacyData = {};
            memcpy(&legacyData, legacyResponse.data() + offset, sizeof(legacyData));
            if (!IsLegacyLwd4TxCompatibilitySignature(legacyData.signature)) {
                continue;
            }

            METROLOGY_DATA converted = {};
            ConvertLegacyLwd4TxMetrology(legacyData, converted);
            applyMetrology(converted);
            return SerialResult::SUCCESS;
        }

        return SerialResult::INVALID_RESPONSE;
    };

    if (protocolMode == DeviceProtocolMode::MODERN_5TX) {
        finalResult = requestModernMetrology();
    } else if (protocolMode == DeviceProtocolMode::LEGACY_4TX) {
        finalResult = requestLegacyMetrology();
    } else {
        DebugLog4Tx("RequestMetrology: protocolMode=UNKNOWN, trying modern first");
        const SerialResult modernResult = requestModernMetrology();
        if (modernResult == SerialResult::SUCCESS) {
            protocolMode = DeviceProtocolMode::MODERN_5TX;
            finalResult = modernResult;
        } else {
            // Modern не удался — сбрасываем буфер устройства (SendPacket отправил 5 байт,
            // Arduino прочитал 4, 1 остался → отправляем 3 padding → Arduino читает 4 и отбрасывает)
            DebugLog4Tx("RequestMetrology: modern failed, flushing device buffer");
            FlushDeviceBufferAfterModernAttempt();

            const SerialResult legacyResult = requestLegacyMetrology();
            DebugLog4Tx("RequestMetrology: legacy result=%d", static_cast<int>(legacyResult));
            if (legacyResult == SerialResult::SUCCESS) {
                protocolMode = DeviceProtocolMode::LEGACY_4TX;
                finalResult = legacyResult;
            } else {
                finalResult = modernResult != SerialResult::SUCCESS ? modernResult : legacyResult;
            }
        }
    }
    } // lock scope

    if (!wasPaused) {
        pausePolling = false;
    }
    return finalResult;
}

SerialResult SerialModule::WriteMetrology(const METROLOGY_DATA& inData) {
    if (!IsConnected()) {
        return SerialResult::NOT_CONNECTED;
    }

    // Приостанавливаем polling на время записи метрологии (аналог metro_check
    // + ожидания busy_read_from_sonde + Sleep(2000) в старом стабильном приложении).
    pausePolling = true;

    SerialResult result;
    {
        std::lock_guard<std::mutex> lock(ioMutex);

        // === Legacy 4Tx: использовать legacy протокол для записи ===
        // Формат: [CMD=0x66][METROLOGY_DATA:240][CRC:2] = 243 байта + 1 padding = 244 (кратно 4)
        // Arduino читает по 4 байта → 244/4 = 61 чтений, 0 остаток = чистый буфер
        if (protocolMode == DeviceProtocolMode::LEGACY_4TX) {
            DebugLog4Tx("WriteMetrology: LEGACY_4TX, using legacy CMD=0x66");
            PurgeBuffers();

            const size_t payloadSize = sizeof(METROLOGY_DATA);
            // 1(CMD) + 240(METROLOGY) + 2(CRC) + 1(padding) = 244 байт = 4*61
            std::vector<uint8_t> packet(1 + payloadSize + 2 + 1, 0x00);
            packet[0] = 0x66; // CMD_WRITE_EE
            memcpy(packet.data() + 1, &inData, payloadSize);
            const uint16_t crc = CalculateCRC16(packet.data(), 1 + payloadSize);
            packet[1 + payloadSize] = static_cast<uint8_t>(crc & 0xFF);
            packet[2 + payloadSize] = static_cast<uint8_t>(crc >> 8);
            // packet[3 + payloadSize] = 0x00; // padding (уже 0)

            DebugLog4Tx("WriteMetrology: sending %zu bytes (legacy), header: %s",
                         packet.size(), HexDump(packet.data(), packet.size(), 8).c_str());

            DWORD bytesWritten = 0;
            if (!WriteFile(hSerial, packet.data(), static_cast<DWORD>(packet.size()), &bytesWritten, nullptr) ||
                bytesWritten != packet.size()) {
                DebugLog4Tx("WriteMetrology: WriteFile FAILED");
                pausePolling = false;
                return SerialResult::CONNECTION_FAILED;
            }

            // Ждём ответ (прибор может не поддерживать запись — эмулятор не поддерживает)
            Sleep(500);

            // Вычитываем и отбрасываем всё что Arduino мог отправить
            // (при совпадении 0x67/0x65 в payload Arduino отправляет ответы)
            PurgeBuffers();
            DebugLog4Tx("WriteMetrology: legacy write done, buffers purged");

            // Применяем метрологию локально (даже если прибор не подтвердил)
            memcpy(&metrology, &inData, sizeof(METROLOGY_DATA));
            hasMetrology = true;
            HandleMetrologyUpdated();

            if (onMetrologyReceived) {
                onMetrologyReceived(metrology);
            }
            pausePolling = false;
            return SerialResult::SUCCESS;
        }

        // === Modern 5Tx: стандартный modern протокол ===
        PurgeBuffers();
        if (!SendPacket(SerialCommands::CMD_SET_METROLOGY,
                        reinterpret_cast<const uint8_t*>(&inData),
                        static_cast<uint16_t>(sizeof(METROLOGY_DATA)))) {
            pausePolling = false;
            return SerialResult::CONNECTION_FAILED;
        }

        // Даём прибору время обработать команду (старое приложение: Sleep(500))
        Sleep(500);

        uint8_t cmd = 0;
        std::vector<uint8_t> payload;
        if (!ReceivePacket(cmd, payload)) {
            // Команда была отправлена; отсутствие ответа допустимо — применяем
            // метрологию локально (аналог старого приложения, которое показывало
            // успех/неудачу, но метрология применялась).
            DebugLog4Tx("WriteMetrology: modern TIMEOUT, applying locally");

            memcpy(&metrology, &inData, sizeof(METROLOGY_DATA));
            hasMetrology = true;
            HandleMetrologyUpdated();
            if (onMetrologyReceived) {
                onMetrologyReceived(metrology);
            }

            result = SerialResult::SUCCESS;
        } else if (cmd != SerialCommands::CMD_SET_METROLOGY) {
            result = SerialResult::INVALID_RESPONSE;
        } else {
            METROLOGY_DATA applied = inData;
            if (!payload.empty()) {
                if (payload.size() == sizeof(METROLOGY_DATA)) {
                    memcpy(&applied, payload.data(), sizeof(METROLOGY_DATA));
                } else if (payload.size() != 1) {
                    pausePolling = false;
                    return SerialResult::INVALID_RESPONSE;
                }
            }

            memcpy(&metrology, &applied, sizeof(METROLOGY_DATA));
            hasMetrology = true;
            HandleMetrologyUpdated();

            if (onMetrologyReceived) {
                onMetrologyReceived(metrology);
            }
            result = SerialResult::SUCCESS;
        }
    }

    pausePolling = false;
    return result;
}

SerialResult SerialModule::SetSelectedKey(uint32_t key) {
    if (!IsConnected()) {
        return SerialResult::NOT_CONNECTED;
    }

    // Legacy 4Tx устройства не поддерживают выбор ключа — пропускаем
    if (protocolMode == DeviceProtocolMode::LEGACY_4TX) {
        DebugLog4Tx("SetSelectedKey: LEGACY_4TX, skipping (not supported)");
        return SerialResult::SUCCESS;
    }

    // Приостанавливаем polling на время отправки команды (аналог metro_check
    // + ожидания busy_read_from_sonde в старом стабильном приложении).
    pausePolling = true;

    SerialResult result;
    {
        std::lock_guard<std::mutex> lock(ioMutex);
        PurgeBuffers();

        if (!SendPacket(SerialCommands::CMD_SET_SELECTED_KEY,
                        reinterpret_cast<const uint8_t*>(&key),
                        static_cast<uint16_t>(sizeof(uint32_t)))) {
            pausePolling = false;
            return SerialResult::CONNECTION_FAILED;
        }

        // Даём прибору время обработать команду (старое приложение: Sleep(150))
        Sleep(150);

        uint8_t cmd = 0;
        std::vector<uint8_t> payload;
        if (!ReceivePacket(cmd, payload)) {
            // Команда была отправлена; отсутствие ответа не считается ошибкой
            // для CMD 0x17 (квитанция — прибор может не отвечать).
            DebugLog4Tx("SetSelectedKey: no response (timeout), treating as success");
            result = SerialResult::SUCCESS;
        } else if (cmd != SerialCommands::CMD_SET_SELECTED_KEY) {
            result = SerialResult::INVALID_RESPONSE;
        } else {
            result = SerialResult::SUCCESS;
        }
    }

    pausePolling = false;
    return result;
}

SerialResult SerialModule::RequestSignature(uint32_t& outSignature) {
    if (!IsConnected()) {
        return SerialResult::NOT_CONNECTED;
    }

    std::lock_guard<std::mutex> lock(ioMutex);

    auto requestModernSignature = [this, &outSignature]() -> SerialResult {
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
    };

    auto requestLegacySignature = [this, &outSignature]() -> SerialResult {
        PurgeBuffers();

        const uint8_t legacyPayload[] = { 0x00, 0x04, 240 };
        std::vector<uint8_t> legacyRequest(1 + sizeof(legacyPayload) + sizeof(uint16_t));
        legacyRequest[0] = 0x65;
        memcpy(legacyRequest.data() + 1, legacyPayload, sizeof(legacyPayload));
        const uint16_t legacyRequestCrc = CalculateCRC16(legacyRequest.data(), 1 + sizeof(legacyPayload));
        legacyRequest[1 + sizeof(legacyPayload)] = static_cast<uint8_t>(legacyRequestCrc & 0xFF);
        legacyRequest[2 + sizeof(legacyPayload)] = static_cast<uint8_t>(legacyRequestCrc >> 8);

        DWORD bytesWritten = 0;
        if (!WriteFile(hSerial, legacyRequest.data(), static_cast<DWORD>(legacyRequest.size()), &bytesWritten, nullptr) ||
            bytesWritten != legacyRequest.size()) {
            return SerialResult::CONNECTION_FAILED;
        }

        std::vector<uint8_t> legacyResponse;
        Sleep(150);
        if (!ReadExactAvailableBytes(hSerial, kLegacyLwdMetrologyPacketSize, 1500, legacyResponse)) {
            return SerialResult::TIMEOUT;
        }

        Sleep(20);

        if (legacyResponse.empty() || legacyResponse[0] != kLegacyLwdMetrologyCommand) {
            return SerialResult::INVALID_RESPONSE;
        }

        if (!ValidateCRC(legacyResponse.data(), legacyResponse.size())) {
            return SerialResult::CRC_ERROR;
        }

        const size_t candidateOffsets[] = { kLegacyLwdMetrologyPayloadOffset, 0u };
        for (size_t offset : candidateOffsets) {
            if (legacyResponse.size() < offset + sizeof(LEGACY_LWD_METROLOGY_DATA)) {
                continue;
            }

            LEGACY_LWD_METROLOGY_DATA legacyData = {};
            memcpy(&legacyData, legacyResponse.data() + offset, sizeof(legacyData));
            if (!IsLegacyLwd4TxCompatibilitySignature(legacyData.signature)) {
                continue;
            }

            outSignature = legacyData.signature;
            return SerialResult::SUCCESS;
        }

        return SerialResult::INVALID_RESPONSE;
    };

    if (protocolMode == DeviceProtocolMode::MODERN_5TX) {
        return requestModernSignature();
    }

    if (protocolMode == DeviceProtocolMode::LEGACY_4TX) {
        return requestLegacySignature();
    }

    DebugLog4Tx("RequestSignature: protocolMode=UNKNOWN, trying modern first");
    const SerialResult modernResult = requestModernSignature();
    if (modernResult == SerialResult::SUCCESS) {
        protocolMode = DeviceProtocolMode::MODERN_5TX;
        return modernResult;
    }

    DebugLog4Tx("RequestSignature: modern failed, flushing device buffer");
    FlushDeviceBufferAfterModernAttempt();

    const SerialResult legacyResult = requestLegacySignature();
    if (legacyResult == SerialResult::SUCCESS) {
        protocolMode = DeviceProtocolMode::LEGACY_4TX;
        return legacyResult;
    }

    return modernResult != SerialResult::SUCCESS ? modernResult : legacyResult;
}

void SerialModule::HandleMetrologyUpdated() {
    std::wstring newDeviceId = GetDeviceId();
    if (!currentDeviceId.empty() && currentDeviceId != newDeviceId) {
        CloseRecordFile();
    }
    currentDeviceId = newDeviceId;

    if (appendToExistingFile) {
        return;
    }

    if (recordFilePath.empty()) return;

    const std::wstring unknownPrefix = L"AMK_Unknown_";
    size_t fileNamePos = recordFilePath.find_last_of(L"\\/");
    std::wstring fileName = (fileNamePos != std::wstring::npos) ? recordFilePath.substr(fileNamePos + 1) : recordFilePath;

    if (fileName.rfind(unknownPrefix, 0) != 0) return;

    const bool wasOpen = recordFile.is_open();
    if (wasOpen) {
        CloseRecordFile();
    }

    std::wstring tail = fileName.substr(unknownPrefix.size());
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
        (void)OpenAppendBinaryStream(recordFile, recordFilePath);
    }
}

// === Callbacks ===

void SerialModule::SetParsedDataCallback(std::function<void(const ParsedToolFrame&)> callback) {
    onParsedDataReceived = callback;
}

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
    sessionFirstFrameNumber = 0;
    hasSessionFirstFrameNumber = false;
    metrologyAutoRetryCount = 0;
    consecutivePollFailures = 0;
    nextReconnectAttemptTick = 0;
}

uint32_t SerialModule::GetDisplayedFrameNumber() const {
    if (!hasSessionFirstFrameNumber || lastFrameNumber < sessionFirstFrameNumber) {
        return lastFrameNumber;
    }

    return (lastFrameNumber - sessionFirstFrameNumber) + 1;
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
    if (remaining > 0 && !WaitForBytesAvailable(hSerial, remaining, 350)) {
        Sleep(50);
    }

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

void SerialModule::FlushDeviceBufferAfterModernAttempt() {
    // Modern SendPacket отправляет 5 байт ([CMD][LEN:2][CRC:2]).
    // Arduino читает ровно 4 байта за раз → 1 байт остаётся в буфере Arduino.
    // Отправляем 3 padding-байта (0x00), чтобы Arduino получил 1+3=4 байта,
    // прочитал их, не нашёл валидной команды и отбросил.
    if (hSerial == INVALID_HANDLE_VALUE) return;

    DebugLog4Tx("FlushDeviceBufferAfterModernAttempt: sending 3 padding bytes");
    const uint8_t padding[3] = { 0x00, 0x00, 0x00 };
    DWORD written = 0;
    WriteFile(hSerial, padding, 3, &written, nullptr);

    // Даём Arduino время обработать (его loop() работает быстро, ~100мкс)
    Sleep(50);

    // Очищаем app RX буфер (Arduino мог отправить что-то по ошибке)
    PurgeBuffers();
    DebugLog4Tx("FlushDeviceBufferAfterModernAttempt: done, purged app RX");
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

void SerialModule::SetAppendToExistingEnabled(bool enabled) {
    if (appendToExistingFile == enabled) {
        return;
    }

    appendToExistingFile = enabled;

    if (recordFile.is_open()) {
        CloseRecordFile();
    }

    if (!appendToExistingFile) {
        existingRecordFilePath.clear();
    }
}

bool SerialModule::SetExistingRecordFilePath(const std::wstring& filePath, std::wstring& outError) {
    outError.clear();

    if (!ValidateExistingRecordFile(filePath, outError)) {
        return false;
    }

    if (recordFile.is_open()) {
        CloseRecordFile();
    }

    existingRecordFilePath = filePath;
    return true;
}

bool SerialModule::ValidateRecordingSettings(std::wstring& outError) const {
    outError.clear();

    if (!isRecordingEnabled) {
        return true;
    }

    if (appendToExistingFile) {
        return ValidateExistingRecordFile(existingRecordFilePath, outError);
    }

    if (recordFolderPath.empty()) {
        outError = L"Не указана папка для записи";
        return false;
    }

    return true;
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
    
    if (appendToExistingFile) {
        std::wstring validateError;
        if (!ValidateExistingRecordFile(existingRecordFilePath, validateError)) {
            lastError = validateError;
            return false;
        }

        recordFilePath = existingRecordFilePath;
    } else {
        // Убеждаемся, что папка существует
        EnsureFolderExists(recordFolderPath);

        // Генерируем имя файла
        recordFilePath = GenerateRecordFileName();
    }
    
    // Открываем файл для бинарной записи
    OpenAppendBinaryStream(recordFile, recordFilePath);
    
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

void SerialModule::FlushRecordFile() {
    if (recordFile.is_open()) {
        recordFile.flush();
    }
}

void SerialModule::WriteDataToFile(const void* rawData, size_t rawSize) {
    // Открываем файл если ещё не открыт
    if (!recordFile.is_open()) {
        if (!OpenRecordFile()) {
            return;  // Не удалось открыть файл
        }
    }

    if (!rawData || rawSize != sizeof(GP_TOOL_DATA)) {
        return;
    }
    
    // Записываем структуру данных (240 байт)
    recordFile.write(reinterpret_cast<const char*>(rawData), static_cast<std::streamsize>(rawSize));
    
    recordedFrames++;
    if ((recordedFrames % 16u) == 0u) {
        recordFile.flush();
    }
}

std::wstring SerialModule::BrowseForFolder(HWND hWnd) {
    wchar_t selectedPath[MAX_PATH] = L"folder.select";

    OPENFILENAMEW ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = selectedPath;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = L"Выберите папку для записи";
    ofn.lpstrFilter = L"Все файлы\0*.*\0";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;

    if (GetSaveFileNameW(&ofn)) {
        std::wstring fullPath = selectedPath;
        size_t lastSlash = fullPath.find_last_of(L"\\/");
        if (lastSlash != std::wstring::npos) {
            return fullPath.substr(0, lastSlash);
        }
    }

    return L"";
}

std::wstring SerialModule::BrowseForExistingRecordFile(HWND hWnd) {
    wchar_t filePath[MAX_PATH] = { 0 };

    OPENFILENAMEW ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = filePath;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = L"Выберите существующий файл для дозаписи";
    ofn.lpstrFilter = L"Данные AMK (*.bin;*.biv)\0*.bin;*.biv\0BIN (*.bin)\0*.bin\0BIV (*.biv)\0*.biv\0Все файлы\0*.*\0";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameW(&ofn)) {
        return filePath;
    }

    return L"";
}

void SerialModule::OpenRecordFolder() const {
    std::wstring folderToOpen = recordFolderPath;

    if (appendToExistingFile && !existingRecordFilePath.empty()) {
        size_t lastSlash = existingRecordFilePath.find_last_of(L"\\/");
        if (lastSlash != std::wstring::npos) {
            folderToOpen = existingRecordFilePath.substr(0, lastSlash);
        }
    }

    if (!folderToOpen.empty()) {
        // ShellExecute доступен через windows.h
        ShellExecuteW(nullptr, L"explore", folderToOpen.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
}
