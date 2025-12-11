#pragma once

#include "framework.h"
#include "DataModule.h"
#include <string>
#include <functional>
#include <vector>
#include <cstdint>

// Команды протокола обмена с прибором
namespace SerialCommands {
    constexpr uint8_t CMD_WORK = 0x67;      // Запрос данных измерений
    constexpr uint8_t CMD_READ_EE = 0x65;   // Чтение EEPROM (метрология)
    constexpr uint8_t CMD_WRITE_EE = 0x66;  // Запись EEPROM
    constexpr uint8_t CMD_ERAM = 0x61;      // Чтение ERAM
    constexpr uint8_t CMD_TURBO = 0xFD;     // Турбо режим
}

// Сигнатуры данных прибора
namespace DataSignatures {
    constexpr uint32_t SIGNATURE_1D4S = 0x31443453;  // "1D4S" - стандартный формат
    constexpr uint32_t SIGNATURE_1D4T = 0x31443454;  // "1D4T" - альтернативный формат
    constexpr uint32_t SIGNATURE_METRO = 0x31443453; // Сигнатура метрологии
}

// Структура метрологических данных прибора
#pragma pack(push, 1)
struct METROLOGY_DATA {
    uint32_t signature;         // Сигнатура для идентификации
    uint32_t serial;            // Серийный номер прибора
    uint16_t L1[8];             // Расстояния L1
    uint16_t L2[8];             // Расстояния L2
    uint16_t F[8];              // Частоты
    int16_t Air_zz[8];          // Воздушные поправки
    uint16_t PhaseOffsets[4];   // Сдвиг между квадратурами
    uint32_t D_sonde_mm;        // Диаметр зонда в мм
    int16_t Air_zz_amt[8];      // Воздушные поправки AMT
    uint8_t service[140];       // Резерв (до 256 байт)
};
#pragma pack(pop)

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
    
    // Таймер для периодического опроса
    UINT_PTR timerId;
    DWORD pollInterval;
    HWND callbackWindow;
    
    // Метрологические данные (кэшируются после первого запроса)
    METROLOGY_DATA metrology;
    bool hasMetrology;
    
    // Счётчики для статистики
    uint32_t packetsReceived;
    uint32_t packetsErrors;
    uint32_t lastFrameNumber;
    
    // Callbacks
    std::function<void(const GP_TOOL_DATA&)> onDataReceived;
    std::function<void(const METROLOGY_DATA&)> onMetrologyReceived;
    std::function<void(ConnectionState, const std::wstring&)> onStateChanged;
    
    // CRC16 таблица (Modbus)
    static const uint16_t CRC16_TABLE[256];
    
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
    
    // === Команды прибора ===
    
    // Запрос данных измерений (CMD_WORK = 0x67)
    SerialResult RequestData(GP_TOOL_DATA& outData);
    
    // Запрос метрологии (CMD_READ_EE = 0x65)
    SerialResult RequestMetrology(METROLOGY_DATA& outData);
    
    // === Callbacks ===
    
    // Установка callback для получения данных
    void SetDataCallback(std::function<void(const GP_TOOL_DATA&)> callback);
    
    // Установка callback для получения метрологии
    void SetMetrologyCallback(std::function<void(const METROLOGY_DATA&)> callback);
    
    // Установка callback для изменения состояния
    void SetStateCallback(std::function<void(ConnectionState, const std::wstring&)> callback);
    
    // === Статистика ===
    
    uint32_t GetPacketsReceived() const { return packetsReceived; }
    uint32_t GetPacketsErrors() const { return packetsErrors; }
    uint32_t GetLastFrameNumber() const { return lastFrameNumber; }
    void ResetStatistics();
    
    // === Вспомогательные методы ===
    
    // Получение списка доступных COM-портов
    static std::vector<std::wstring> GetAvailablePorts();
    
    // Получение кэшированной метрологии
    bool HasMetrology() const { return hasMetrology; }
    const METROLOGY_DATA& GetCachedMetrology() const { return metrology; }
    
    // Преобразование результата в строку
    static std::wstring ResultToString(SerialResult result);
    
private:
    // === Низкоуровневые методы ===
    
    // Отправка команды прибору
    bool SendCommand(uint8_t cmd, const uint8_t* data, size_t dataLength);
    
    // Получение пакета от прибора
    bool ReceivePacket(uint8_t* buffer, size_t bufferSize, size_t& bytesRead);
    
    // Вычисление CRC16 (Modbus)
    uint16_t CalculateCRC16(const uint8_t* data, size_t length) const;
    
    // Проверка CRC пакета
    bool ValidateCRC(const uint8_t* packet, size_t length) const;
    
    // Проверка сигнатуры данных
    bool ValidateSignature(uint32_t signature) const;
    
    // Установка состояния с уведомлением
    void SetState(ConnectionState newState, const std::wstring& error = L"");
    
    // Очистка буферов COM-порта
    void PurgeBuffers();
};

// ID таймера для опроса (используется в WndProc)
#define IDT_SERIAL_POLL 9001
