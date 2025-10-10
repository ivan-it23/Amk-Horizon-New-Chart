#pragma once

#include <vector>
#include <string>
#include <cstdint>

// Forward declarations
struct CurveDefinition;
struct ChartConfiguration;

// Структура данных прибора
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

// Типы данных для отображения
enum class DataType {
    PHASE_DIFFERENCE,        // Разность фаз RX2-RX1 (4 канала T1-T4)
    AMPLITUDE_RATIO,         // Отношение амплитуд RX2/RX1 (4 канала T1-T4)
    AMPLITUDE_RX1,           // Амплитуды RX1 активных передатчиков (4 канала)
    AMPLITUDE_RX2,           // Амплитуды RX2 активных передатчиков (4 канала)
    PHASE_RX1,              // Фазы RX1 активных передатчиков (4 канала)
    PHASE_RX2,              // Фазы RX2 активных передатчиков (4 канала)
    SILENT_AMPLITUDE_RX1,    // Амплитуды молчащих передатчиков RX1 (1 канал)
    SILENT_AMPLITUDE_RX2,    // Амплитуды молчащих передатчиков RX2 (1 канал)
    TEMPERATURE             // Температура прибора (1 значение на запись)
};

// Типы передатчиков
enum class Transmitter {
    T1 = 0,
    T2 = 1,
    T3 = 2,
    T4 = 3
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

public:
    DataModule();
    ~DataModule();

    // Управление данными
    bool LoadDataFromFile(const std::wstring& filename);
    void ClearData();
    bool HasData() const;
    size_t GetDataSize() const;
    const std::vector<GP_TOOL_DATA>& GetData() const;

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
    
private:
    // Внутренние методы для вычисления различных типов данных
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
    
    // Вспомогательные методы для работы с индексами
    int GetDataArrayIndex(int transmitterIndex, int receiverIndex, 
                         FrequencyBand frequency) const;
    
    bool IsValidDataRange(double minX, double maxX) const;
    std::pair<size_t, size_t> GetDataIndexRange(double minX, double maxX) const;
};