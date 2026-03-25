#pragma once

#include "framework.h"
#include "ToolDataAdapters.h"
#include "ToolDataTypes.h"
#include <string>
#include <functional>
#include <vector>
#include <cstdint>
#include <fstream>
#include <mutex>
#include <atomic>
#include <deque>
#include <thread>

// Команды протокола обмена с прибором
namespace SerialCommands {
    constexpr uint8_t CMD_DATA = 0x11;          // Запрос данных измерений
    constexpr uint8_t CMD_SET_METROLOGY = 0x12; // Запись метрологии (квитанция)
    constexpr uint8_t CMD_GET_METROLOGY = 0x13; // Запрос метрологии
    constexpr uint8_t CMD_GET_SIGNATURE = 0x16; // Запрос сигнатуры
    constexpr uint8_t CMD_SET_SELECTED_KEY = 0x17; // Запись selected_key (квитанция)
}

// Структура метрологических данных прибора
#pragma pack(push, 1)
struct METROLOGY_DATA {
    uint32_t signature;
    uint32_t serial;
    uint16_t L1[5];
    uint16_t L2[5];
    uint16_t F[2];
    int16_t Air_zz[10];
    int16_t Air_zz_amt[10];
    uint32_t D_sonde_mm;
    uint8_t service[164];
};
#pragma pack(pop)

static_assert(sizeof(METROLOGY_DATA) == 240, "METROLOGY_DATA must be exactly 240 bytes");

// Состояние подключения
enum class ConnectionState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    CONNECTION_ERROR  // Переименовано из ERROR для избежания конфликта с макросом Windows
};

// Результат операции
enum class SerialResult {
    SUCCESS,
    PORT_NOT_FOUND,
    PORT_BUSY,
    CONNECTION_FAILED,
    TIMEOUT,
    CRC_ERROR,
    INVALID_SIGNATURE,
    INVALID_RESPONSE,
    NOT_CONNECTED
};

enum class DeviceProtocolMode {
    UNKNOWN,
    MODERN_5TX,
    LEGACY_4TX
};

// Класс для работы с COM-портом и протоколом обмена с прибором
class SerialModule {
private:
    // Handle COM-порта (Windows API)
    HANDLE hSerial;
    
    // Параметры подключения
    std::wstring portName;
    DWORD baudRate;
    DWORD readTimeout;
    DWORD writeTimeout;
    
    // Состояние
    ConnectionState state;
    std::wstring lastError;
    DeviceProtocolMode protocolMode;
    
    // Таймер для периодического опроса
    UINT_PTR timerId;
    DWORD pollInterval;
    HWND callbackWindow;
    
    // Метрологические данные (кэшируются после первого запроса)
    METROLOGY_DATA metrology;
    bool hasMetrology;
    uint32_t metrologyAutoRetryCount;   // v2.15.3: авто-попытки получить метрологию при опросе
    
    // Счётчики для статистики
    uint32_t packetsReceived;
    uint32_t packetsErrors;
    uint32_t lastFrameNumber;
    uint32_t sessionFirstFrameNumber;
    bool hasSessionFirstFrameNumber;

    struct PendingPollEvent {
        SerialResult result = SerialResult::TIMEOUT;
        ParsedToolFrame parsedFrame = {};
        std::vector<uint8_t> rawPayload;
        uint32_t connectionSessionId = 0;
    };

    DWORD legacyPollInterval;
    std::thread legacyPollingThread;
    std::atomic<bool> stopLegacyPolling;
    std::atomic<bool> pausePolling;
    std::mutex pendingPollEventsMutex;
    std::deque<PendingPollEvent> pendingPollEvents;
    std::atomic<uint32_t> consecutivePollFailures;
    std::atomic<DWORD> nextReconnectAttemptTick;
    std::atomic<uint32_t> connectionSessionId;
    
    // Callbacks
    std::function<void(const ParsedToolFrame&)> onParsedDataReceived;
    std::function<void(const GP_TOOL_DATA&)> onDataReceived;
    std::function<void(const METROLOGY_DATA&)> onMetrologyReceived;
    std::function<void(ConnectionState, const std::wstring&)> onStateChanged;
    
    // CRC16 таблица (Modbus)
    static const uint16_t CRC16_TABLE[256];
    
    // === Запись данных в файл (v2.15.0) ===
    std::ofstream recordFile;           // Файл для записи данных
    std::wstring recordFilePath;        // Полный путь к файлу записи
    std::wstring recordFolderPath;      // Папка для сохранения файлов
    std::wstring existingRecordFilePath; // Полный путь к существующему файлу (режим дозаписи)
    bool isRecordingEnabled;            // Включена ли запись
    bool appendToExistingFile;          // Режим дозаписи в существующий файл
    uint32_t recordedFrames;            // Количество записанных кадров
    std::wstring currentDeviceId;       // ID текущего прибора (SN_Diameter)
    std::vector<uint8_t> lastRawDataFrame;

    // Защита от конфликтов: polling (таймер) vs метрология/команды
    std::mutex ioMutex;
    
public:
    SerialModule();
    ~SerialModule();
    
    // Запрет копирования
    SerialModule(const SerialModule&) = delete;
    SerialModule& operator=(const SerialModule&) = delete;
    
    // === Управление подключением ===
    
    // Подключение к COM-порту
    SerialResult Connect(const std::wstring& port, DWORD baud = 125000);
    
    // Отключение от COM-порта
    void Disconnect();
    
    // Проверка состояния подключения
    bool IsConnected() const;
    ConnectionState GetState() const;
    std::wstring GetConnectedPort() const { return portName; }
    std::wstring GetLastErrorMessage() const;  // Переименовано из GetLastError для избежания конфликта с Windows API
    
    // === Управление опросом ===
    
    // Запуск периодического опроса прибора
    void StartPolling(HWND hWnd, DWORD intervalMs = 2000);
    
    // Остановка опроса
    void StopPolling();
    
    // Проверка, запущен ли опрос
    bool IsPolling() const;
    
    // Обработчик таймера (вызывается из WndProc)
    void OnTimer();
    void FlushPendingPollResults();
    
    // === Команды прибора ===
    
    // Запрос данных измерений (CMD_DATA = 0x11)
    SerialResult RequestData(ParsedToolFrame& outParsedFrame);
    SerialResult RequestData(ParsedToolFrame& outParsedFrame, std::vector<uint8_t>* outRawPayload);
    SerialResult RequestData(GP_TOOL_DATA& outData);
    SerialResult RequestData(GP_TOOL_DATA& outData, std::vector<uint8_t>* outRawPayload);
    
    // Запрос метрологии (CMD_GET_METROLOGY = 0x13)
    SerialResult RequestMetrology(METROLOGY_DATA& outData);
    
    // Запись метрологии (CMD_SET_METROLOGY = 0x12)
    SerialResult WriteMetrology(const METROLOGY_DATA& inData);

    // Запись selected_key (CMD_SET_SELECTED_KEY = 0x17)
    SerialResult SetSelectedKey(uint32_t key);

    // Запрос сигнатуры (CMD_GET_SIGNATURE = 0x16)
    SerialResult RequestSignature(uint32_t& outSignature);
    
    // === Callbacks ===
    
    // Установка callback для получения данных
    void SetParsedDataCallback(std::function<void(const ParsedToolFrame&)> callback);
    void SetDataCallback(std::function<void(const GP_TOOL_DATA&)> callback);
    
    // Установка callback для получения метрологии
    void SetMetrologyCallback(std::function<void(const METROLOGY_DATA&)> callback);
    
    // Установка callback для изменения состояния
    void SetStateCallback(std::function<void(ConnectionState, const std::wstring&)> callback);
    
    // === Статистика ===
    
    uint32_t GetPacketsReceived() const { return packetsReceived; }
    uint32_t GetPacketsErrors() const { return packetsErrors; }
    uint32_t GetLastFrameNumber() const { return lastFrameNumber; }
    uint32_t GetDisplayedFrameNumber() const;
    void ResetStatistics();
    
    // === Вспомогательные методы ===
    
    // Получение списка доступных COM-портов
    static std::vector<std::wstring> GetAvailablePorts();
    
    // Получение кэшированной метрологии
    bool HasMetrology() const { return hasMetrology; }
    const METROLOGY_DATA& GetCachedMetrology() const { return metrology; }
    
    // Преобразование результата в строку
    static std::wstring ResultToString(SerialResult result);
    
    // === Запись данных в файл (v2.15.0) ===
    
    // Включение/выключение записи
    void SetRecordingEnabled(bool enabled);
    bool IsRecordingEnabled() const { return isRecordingEnabled; }
    
    // Установка папки для сохранения (по умолчанию - рабочий стол)
    void SetRecordFolder(const std::wstring& folderPath);
    std::wstring GetRecordFolder() const { return recordFolderPath; }

    // Режим дозаписи в существующий файл
    void SetAppendToExistingEnabled(bool enabled);
    bool IsAppendToExistingEnabled() const { return appendToExistingFile; }
    bool SetExistingRecordFilePath(const std::wstring& filePath, std::wstring& outError);
    std::wstring GetExistingRecordFilePath() const { return existingRecordFilePath; }
    bool ValidateRecordingSettings(std::wstring& outError) const;
    
    // Получение информации о записи
    std::wstring GetRecordFilePath() const { return recordFilePath; }
    uint32_t GetRecordedFrames() const { return recordedFrames; }
    bool IsRecordingActive() const { return recordFile.is_open(); }
    
    // v2.19.0: Принудительный flush записи (для безопасного чтения файла извне)
    void FlushRecordFile();
    
    // Выбор папки через диалог
    static std::wstring BrowseForFolder(HWND hWnd);

    // Выбор существующего файла для дозаписи
    static std::wstring BrowseForExistingRecordFile(HWND hWnd);
    
    // Открытие папки в Explorer
    void OpenRecordFolder() const;
    
private:
    // === Низкоуровневые методы ===
    
    // Отправка пакета прибору (формат: CMD + uint16 length + payload + CRC16)
    bool SendPacket(uint8_t cmd, const uint8_t* data, uint16_t dataLength);
    
    // Получение пакета от прибора (формат: CMD + uint16 length + payload + CRC16)
    bool ReceivePacket(uint8_t& outCmd, std::vector<uint8_t>& outPayload);
    
    // Вычисление CRC16 (Modbus)
    uint16_t CalculateCRC16(const uint8_t* data, size_t length) const;
    
    // Проверка CRC пакета (CRC считается по первым (packetSize-2) байтам)
    bool ValidateCRC(const uint8_t* packet, size_t packetSize) const;
    
    // Установка состояния с уведомлением
    void SetState(ConnectionState newState, const std::wstring& error = L"");

    void LegacyPollingLoop();
    void HandleSuccessfulPoll(const ParsedToolFrame& parsedFrame, const std::vector<uint8_t>& rawPayload);
    void HandleFailedPoll(SerialResult result);
    bool ShouldTriggerAutoReconnect(SerialResult result) const;
    void InvalidateConnectionForRecovery();
    bool TryReconnectCurrentPort();
    
    // Очистка буферов COM-порта
    void PurgeBuffers();

    // Сброс буфера устройства после неудачной попытки modern-протокола.
    // Modern SendPacket отправляет 5 байт, Arduino читает 4 → 1 лишний байт.
    // Эта функция отправляет 3 padding-байта (1+3=4), чтобы Arduino
    // прочитал и отбросил пакет, очищая буфер.
    void FlushDeviceBufferAfterModernAttempt();

    // Пост-обработка после обновления metrology (deviceId, rename Unknown record file, callbacks)
    void HandleMetrologyUpdated();
    
    // === Методы записи в файл (v2.15.0) ===
    
    // Создание/открытие файла записи для текущего прибора
    bool OpenRecordFile();
    
    // Закрытие файла записи
    void CloseRecordFile();
    
    // Запись одного кадра данных
    void WriteDataToFile(const void* rawData, size_t rawSize);
    
    // Генерация уникального имени файла
    std::wstring GenerateRecordFileName() const;
    
    // Получение ID прибора из метрологии
    std::wstring GetDeviceId() const;
    
    // Инициализация папки по умолчанию (рабочий стол)
    void InitDefaultRecordFolder();

    // Проверка, что файл подходит для дозаписи кадрами GP_TOOL_DATA
    bool ValidateExistingRecordFile(const std::wstring& filePath, std::wstring& outError) const;
    
    // Создание папки если не существует
    static bool EnsureFolderExists(const std::wstring& folderPath);
};

// ID таймера для опроса (используется в WndProc)
#define IDT_SERIAL_POLL 9001
