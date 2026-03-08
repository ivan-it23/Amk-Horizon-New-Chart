#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <fstream>
#include "ToolDataAccess.h"

// Forward declarations
struct CurveDefinition;
struct ChartConfiguration;

// Типы данных для отображения
enum class DataType {
    PHASE_DIFFERENCE,        // Разность фаз (источник: DELTA_PH) (5 каналов T1-T5)
    AMPLITUDE_RATIO,         // Отношение амплитуд RX2/RX1 (5 каналов T1-T5)
    AMPLITUDE_RX1,           // Амплитуды RX1 активных передатчиков (5 каналов)
    AMPLITUDE_RX2,           // Амплитуды RX2 активных передатчиков (5 каналов)
    PHASE_SMT,               // Симметризованные фазы (5 каналов)
    RHO_SMT,                 // Симметризованные УЭС (5 каналов)
    SILENT_AMPLITUDE_RX1,    // Амплитуды молчащих передатчиков RX1 (1 канал)
    SILENT_AMPLITUDE_RX2,    // Амплитуды молчащих передатчиков RX2 (1 канал)
    SILENT_PHASE_DIFFERENCE, // Разность фаз молчащих передатчиков R2−R1 (источник: ZERO_dPH) (1 канал)
    TEMPERATURE             // Температура прибора (1 значение на запись)
};

// Типы передатчиков
enum class Transmitter {
    T1 = 0,
    T2 = 1,
    T3 = 2,
    T4 = 3,
    T5 = 4
};

// Частотные диапазоны
enum class FrequencyBand {
    FREQ_400_KHZ,    // 400 кГц
    FREQ_2000_KHZ    // 2000 кГц
};

// Класс для управления данными
class DataModule {
private:
    std::vector<GP_TOOL_DATA> data;
    std::vector<ToolRecordMetadata> recordMetadata;
    std::wstring loadedFileName;  // Имя загруженного файла (без пути и расширения)

    static bool ReadFileSignature(std::ifstream& file, FileFormat format, uint32_t& outSignature);
    static uint32_t DecodeToolCode(uint32_t signature);
    static bool IsDirectlySupportedToolCode(uint32_t toolCode);
    static ToolRecordMetadata BuildRecordMetadata(const GP_TOOL_DATA& record);

public:
    DataModule();
    ~DataModule();

    // Управление данными
    bool LoadDataFromFile(const std::wstring& filename);
    bool LoadDataFromFile(const std::wstring& filename, std::wstring& outError);
    void ClearData();
    
    // v2.15.1: Определение формата файла по расширению
    static FileFormat DetectFileFormat(const std::wstring& filename);
    static bool ValidateDataFile(const std::wstring& filename, std::wstring& outError);
    static uint32_t DecodeToolCodeFromSignature(uint32_t signature);
    static ToolDataKind DetermineToolDataKindFromSignature(uint32_t signature);
    static bool ParseRawFrameDetailed(const void* rawFrame, size_t rawFrameSize, ParsedToolFrame& outFrame);
    static bool ParseRawFrame(const void* rawFrame, size_t rawFrameSize, GP_TOOL_DATA& outData);
    bool HasData() const;
    size_t GetDataSize() const;
    const std::vector<GP_TOOL_DATA>& GetData() const;
    bool TryGetRecordMetadata(size_t index, ToolRecordMetadata& outMetadata) const;
    MeasurementView GetMeasurementView(size_t index) const;
    ToolDataAccessView GetDataView(size_t index) const;
    
    // v2.13.0: Добавление данных в реальном времени
    void AddData(const GP_TOOL_DATA& newData);
    void AddDataBatch(const std::vector<GP_TOOL_DATA>& newDataBatch);
    
    // Получение имени загруженного файла
    std::wstring GetLoadedFileName() const;
    
    // Сохранение диапазона данных
    bool SaveDataRange(const std::wstring& filename, size_t startIndex, size_t endIndex) const;

    // Вычисления данных для графиков (существующие методы для обратной совместимости)
    void CalculateChartData(DataType type, FrequencyBand freq, std::vector<std::vector<float>>& outData) const;
    void CalculateChartData(DataType type, FrequencyBand freq, std::vector<std::vector<float>>& outData, 
                           double minX, double maxX, double minY, double maxY) const;
    void CalculateChartData(DataType type, FrequencyBand freq, std::vector<std::vector<float>>& outData,
                            const std::vector<bool>& enabledTransmitters, double minX, double maxX, double minY, double maxY) const;
    
    // Новые оптимизированные методы для работы с конфигурациями кривых
    void CalculateChartDataFromConfiguration(const ChartConfiguration& config,
                                           std::vector<std::vector<float>>& leftAxisData,
                                           std::vector<std::vector<float>>& rightAxisData) const;
    
    void CalculateChartDataFromConfiguration(const ChartConfiguration& config,
                                           std::vector<std::vector<float>>& leftAxisData,
                                           std::vector<std::vector<float>>& rightAxisData,
                                           double minX, double maxX, double minY, double maxY) const;
    
    // Новые методы для работы с произвольными кривыми
    void CalculateCustomCurveData(const std::vector<CurveDefinition>& curves,
                                 std::vector<std::vector<float>>& leftAxisData,
                                 std::vector<std::vector<float>>& rightAxisData) const;
    
    void CalculateCustomCurveData(const std::vector<CurveDefinition>& curves,
                                 std::vector<std::vector<float>>& leftAxisData,
                                 std::vector<std::vector<float>>& rightAxisData,
                                 double minX, double maxX, double minY, double maxY) const;
    
    // Вычисление данных для отдельной кривой
    void CalculateSingleCurveData(const CurveDefinition& curve, 
                                 std::vector<float>& outData) const;
    
    void CalculateSingleCurveData(const CurveDefinition& curve, 
                                 std::vector<float>& outData,
                                 double minX, double maxX, double minY, double maxY) const;
    
    // Группировка кривых по осям
    void GroupCurvesByAxis(const std::vector<CurveDefinition>& curves,
                          std::vector<CurveDefinition>& leftAxisCurves,
                          std::vector<CurveDefinition>& rightAxisCurves) const;
    
    // Вспомогательные методы
    std::wstring GetDataTypeName(DataType type) const;
    int GetActiveTransmitterIndex(Transmitter transmitter, FrequencyBand freq) const;
    int GetSilentTransmitterIndex(FrequencyBand freq) const;
    
    // v2.10.0: Получение температурных данных для оси X
    void GetTemperatureData(std::vector<float>& outData) const;
    void GetTemperatureData(std::vector<float>& outData, double minX, double maxX) const;
    
    // Методы для вычисления различных типов данных (публичные для использования в CurveSelectionModule)
    void CalculateRatioData(int transmitterIndex, FrequencyBand frequency,
                           std::vector<float>& outData, 
                           double minX, double maxX) const;
    
    void CalculateDifferenceData(int transmitterIndex, FrequencyBand frequency,
                                std::vector<float>& outData,
                                double minX, double maxX) const;
    
    void CalculateAmplitudeData(int transmitterIndex, int receiverIndex, 
                               FrequencyBand frequency, std::vector<float>& outData,
                               double minX, double maxX) const;
    
    void CalculatePhaseData(int transmitterIndex, int receiverIndex, 
                           FrequencyBand frequency, std::vector<float>& outData,
                           double minX, double maxX) const;
    
    void CalculateSilentAmplitudeData(int receiverIndex, FrequencyBand frequency,
                                     std::vector<float>& outData,
                                     double minX, double maxX) const;

    // Разность фаз молчащих передатчиков (R2−R1) по полю ZERO_dPH[freqIdx]
    void CalculateSilentPhaseDifferenceData(FrequencyBand frequency,
                                           std::vector<float>& outData,
                                           double minX, double maxX) const;
    
private:
    
    // Вспомогательные методы для работы с индексами
    int GetDataArrayIndex(int transmitterIndex, int receiverIndex, 
                         FrequencyBand frequency) const;
    
    int GetAvailableTransmitterCount() const;

    bool IsValidDataRange(double minX, double maxX) const;
    std::pair<size_t, size_t> GetDataIndexRange(double minX, double maxX) const;
    
    // Метод нормализации фазы в диапазон (-π, π]
    float NormalizePhase(float phaseRadians) const;
};