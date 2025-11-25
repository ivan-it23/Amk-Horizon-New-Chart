#include "framework.h"
#include "DataModule.h"
#include "SchematicModule.h"
#include <fstream>
#include <algorithm>
#include <cmath>

DataModule::DataModule() {
}

DataModule::~DataModule() {
}

bool DataModule::LoadDataFromFile(const std::wstring& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
        return false;
        
    data.clear();
    
    GP_TOOL_DATA record;
    while (file.read(reinterpret_cast<char*>(&record), sizeof(GP_TOOL_DATA))) {
        data.push_back(record);
    }
    
    file.close();
    
    // Сохраняем имя файла без пути и расширения
    if (!data.empty()) {
        // Извлекаем имя файла из полного пути
        size_t lastSlash = filename.find_last_of(L"\\/");
        std::wstring fileNameOnly = (lastSlash != std::wstring::npos) 
            ? filename.substr(lastSlash + 1) 
            : filename;
        
        // Удаляем расширение .bin
        size_t lastDot = fileNameOnly.find_last_of(L'.');
        loadedFileName = (lastDot != std::wstring::npos) 
            ? fileNameOnly.substr(0, lastDot) 
            : fileNameOnly;
    }
    
    return !data.empty();
}

void DataModule::ClearData() {
    data.clear();
    loadedFileName.clear();
}

std::wstring DataModule::GetLoadedFileName() const {
    return loadedFileName;
}

bool DataModule::HasData() const {
    return !data.empty();
}

size_t DataModule::GetDataSize() const {
    return data.size();
}

const std::vector<GP_TOOL_DATA>& DataModule::GetData() const {
    return data;
}

bool DataModule::SaveDataRange(const std::wstring& filename, size_t startIndex, size_t endIndex) const {
    // Проверка корректности индексов
    if (data.empty()) {
        return false;
    }
    
    if (startIndex >= data.size() || endIndex >= data.size() || startIndex > endIndex) {
        return false;
    }
    
    // Открываем файл для записи
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    // Записываем данные из диапазона [startIndex, endIndex]
    for (size_t i = startIndex; i <= endIndex; ++i) {
        file.write(reinterpret_cast<const char*>(&data[i]), sizeof(GP_TOOL_DATA));
    }
    
    file.close();
    
    return true;
}

void DataModule::CalculateChartData(DataType type, FrequencyBand freq, std::vector<std::vector<float>>& outData) const {
    // Вызываем перегруженную версию без ограничений диапазона
    std::vector<bool> allTransmittersEnabled = {true, true, true, true}; // По умолчанию все передатчики включены
    CalculateChartData(type, freq, outData, allTransmittersEnabled, 0.0, static_cast<double>(data.size() - 1), -1e6, 1e6);
}

void DataModule::CalculateChartData(DataType type, FrequencyBand freq, std::vector<std::vector<float>>& outData,
                                   double minX, double maxX, double minY, double maxY) const {
    std::vector<bool> allTransmittersEnabled = {true, true, true, true}; // По умолчанию все передатчики включены
    CalculateChartData(type, freq, outData, allTransmittersEnabled, minX, maxX, minY, maxY);
}

void DataModule::CalculateChartData(DataType type, FrequencyBand freq, std::vector<std::vector<float>>& outData,
                                   const std::vector<bool>& enabledTransmitters,
                                   double minX, double maxX, double minY, double maxY) const {
    outData.clear();
    
    if (data.empty())
        return;
    
    // Используем оптимизированный метод получения диапазона индексов
    auto indexRange = GetDataIndexRange(minX, maxX);
    size_t startIndex = indexRange.first;
    size_t endIndex = indexRange.second;
    
    if (startIndex >= endIndex) 
        return;
    
    switch (type) {
    case DataType::PHASE_DIFFERENCE:
    case DataType::AMPLITUDE_RATIO:
    case DataType::AMPLITUDE_RX1:
    case DataType::AMPLITUDE_RX2:
    case DataType::PHASE_RX1:
    case DataType::PHASE_RX2:
        {
            // 4 канала для активных передатчиков
            outData.resize(4); // Четыре канала T1-T4
            
            // Предварительно вычисляем индексы для оптимизации
            int rxIndices[4];
            bool validTransmitters[4];
            for (int t = 0; t < 4; ++t) {
                validTransmitters[t] = enabledTransmitters[t];
                rxIndices[t] = validTransmitters[t] ? 
                    GetActiveTransmitterIndex(static_cast<Transmitter>(t), freq) : -1;
                if (rxIndices[t] == -1) {
                    validTransmitters[t] = false;
                }
            }
            
            // Резервируем память для оптимизации
            size_t expectedSize = endIndex - startIndex;
            for (int t = 0; t < 4; ++t) {
                if (validTransmitters[t]) {
                    outData[t].reserve(expectedSize);
                }
            }
            
            // Основной цикл вычислений
            for (size_t i = startIndex; i < endIndex; ++i) {
                const GP_TOOL_DATA& record = data[i];
                
                for (int t = 0; t < 4; ++t) {
                    if (!validTransmitters[t]) continue;
                    
                    int rxIdx = rxIndices[t];
                    float value = 0.0f;
                    
                    switch (type) {
                        case DataType::PHASE_DIFFERENCE:
                            {
                                float phaseDiff = record.PH_RX_2[rxIdx] - record.PH_RX_1[rxIdx];
                                phaseDiff = NormalizePhase(phaseDiff); // Нормализация в диапазон (-π, π]
                                value = phaseDiff * 57295.7795f; // Преобразование рад -> мдег
                            }
                            break;
                        case DataType::AMPLITUDE_RATIO:
                            value = (record.AM_RX_1[rxIdx] != 0.0f) ? 
                                   (record.AM_RX_2[rxIdx] / record.AM_RX_1[rxIdx]) : 0.0f;
                            break;
                        case DataType::AMPLITUDE_RX1:
                            value = record.AM_RX_1[rxIdx] * 4000.0f / pow(2.0f, 24.0f); // Преобразование В -> мВ
                            break;
                        case DataType::AMPLITUDE_RX2:
                            value = record.AM_RX_2[rxIdx] * 4000.0f / pow(2.0f, 24.0f); // Преобразование В -> мВ
                            break;
                        case DataType::PHASE_RX1:
                            {
                                float phase = record.PH_RX_1[rxIdx];
                                phase = NormalizePhase(phase); // Нормализация в диапазон (-π, π]
                                value = phase * 57295.7795f; // Преобразование рад -> мдег
                            }
                            break;
                        case DataType::PHASE_RX2:
                            {
                                float phase = record.PH_RX_2[rxIdx];
                                phase = NormalizePhase(phase); // Нормализация в диапазон (-π, π]
                                value = phase * 57295.7795f; // Преобразование рад -> мдег
                            }
                            break;
                        default:
                            continue;
                    }
                    
                    // Проверяем диапазон значений для фильтрации
                    if (value >= minY && value <= maxY) {
                        outData[t].push_back(value);
                    }
                }
            }
        }
        break;
    
    case DataType::SILENT_AMPLITUDE_RX1:
    case DataType::SILENT_AMPLITUDE_RX2:
        {
            // 1 канал для молчащих передатчиков
            outData.resize(1);
            int rxIdx = GetSilentTransmitterIndex(freq);
            if (rxIdx == -1) break;
            
            // Резервируем память для оптимизации
            outData[0].reserve(endIndex - startIndex);
            
            for (size_t i = startIndex; i < endIndex; ++i) {
                const GP_TOOL_DATA& record = data[i];
                float value = (type == DataType::SILENT_AMPLITUDE_RX1) ? 
                             record.AM_RX_1[rxIdx] * 4000.0f / pow(2.0f, 24.0f) : 
                             record.AM_RX_2[rxIdx] * 4000.0f / pow(2.0f, 24.0f); // Преобразование В -> мВ
                
                // Проверяем диапазон значений для фильтрации
                if (value >= minY && value <= maxY) {
                    outData[0].push_back(value);
                }
            }
        }
        break;

    case DataType::TEMPERATURE:
        {
            // 1 канал для температуры
            outData.resize(1);
            outData[0].reserve(endIndex - startIndex);
            
            for (size_t i = startIndex; i < endIndex; ++i) {
                const GP_TOOL_DATA& record = data[i];
                float value = record.temperature;
                
                // Проверяем диапазон значений для фильтрации
                if (value >= minY && value <= maxY) {
                    outData[0].push_back(value);
                }
            }
        }
        break;
    }
}

std::wstring DataModule::GetDataTypeName(DataType type) const {
    switch (type) {
    case DataType::PHASE_DIFFERENCE: return L"Phase Difference";
    case DataType::AMPLITUDE_RATIO: return L"Amplitude Ratio";
    case DataType::AMPLITUDE_RX1: return L"Amplitude RX1";
    case DataType::AMPLITUDE_RX2: return L"Amplitude RX2";
    case DataType::PHASE_RX1: return L"Phase RX1";
    case DataType::PHASE_RX2: return L"Phase RX2";
    case DataType::SILENT_AMPLITUDE_RX1: return L"Silent Amplitude RX1";
    case DataType::SILENT_AMPLITUDE_RX2: return L"Silent Amplitude RX2";
    case DataType::TEMPERATURE: return L"Temperature";
    default: return L"Unknown Type";
    }
}

// Вспомогательные методы
int DataModule::GetActiveTransmitterIndex(Transmitter transmitter, FrequencyBand freq) const {
    // Индексы для активных передатчиков T1-T4
    if (freq == FrequencyBand::FREQ_400_KHZ) {
        switch (transmitter) {
            case Transmitter::T1: return 2;
            case Transmitter::T2: return 3;
            case Transmitter::T3: return 4;
            case Transmitter::T4: return 5;
            default: return -1; // Should not happen
        }
    } else { // FREQ_2000_KHZ
        switch (transmitter) {
            case Transmitter::T1: return 6;
            case Transmitter::T2: return 7;
            case Transmitter::T3: return 8;
            case Transmitter::T4: return 9;
            default: return -1; // Should not happen
        }
    }
}

int DataModule::GetSilentTransmitterIndex(FrequencyBand freq) const {
    // Индексы для молчащих передатчиков
    if (freq == FrequencyBand::FREQ_400_KHZ)
        return 0; // индекс 0
    else
        return 1; // индекс 1
}

// Новые методы для работы с произвольными кривыми
void DataModule::CalculateCustomCurveData(const std::vector<CurveDefinition>& curves,
                                         std::vector<std::vector<float>>& leftAxisData,
                                         std::vector<std::vector<float>>& rightAxisData) const {
    // Вызываем перегруженную версию без ограничений диапазона
    CalculateCustomCurveData(curves, leftAxisData, rightAxisData, 
                            0.0, static_cast<double>(data.size() - 1), -1e6, 1e6);
}

void DataModule::CalculateCustomCurveData(const std::vector<CurveDefinition>& curves,
                                         std::vector<std::vector<float>>& leftAxisData,
                                         std::vector<std::vector<float>>& rightAxisData,
                                         double minX, double maxX, double minY, double maxY) const {
    leftAxisData.clear();
    rightAxisData.clear();
    
    if (data.empty() || curves.empty()) {
        return;
    }
    
    // Группируем кривые по осям
    std::vector<CurveDefinition> leftAxisCurves, rightAxisCurves;
    GroupCurvesByAxis(curves, leftAxisCurves, rightAxisCurves);
    
    // Вычисляем данные для левой оси
    leftAxisData.resize(leftAxisCurves.size());
    for (size_t i = 0; i < leftAxisCurves.size(); ++i) {
        CalculateSingleCurveData(leftAxisCurves[i], leftAxisData[i], minX, maxX, minY, maxY);
    }
    
    // Вычисляем данные для правой оси
    rightAxisData.resize(rightAxisCurves.size());
    for (size_t i = 0; i < rightAxisCurves.size(); ++i) {
        CalculateSingleCurveData(rightAxisCurves[i], rightAxisData[i], minX, maxX, minY, maxY);
    }
}

void DataModule::CalculateSingleCurveData(const CurveDefinition& curve, 
                                         std::vector<float>& outData) const {
    // Вызываем перегруженную версию без ограничений диапазона
    CalculateSingleCurveData(curve, outData, 0.0, static_cast<double>(data.size() - 1), -1e6, 1e6);
}

void DataModule::CalculateSingleCurveData(const CurveDefinition& curve, 
                                         std::vector<float>& outData,
                                         double minX, double maxX, double minY, double maxY) const {
    outData.clear();
    
    if (data.empty()) {
        return;
    }
    
    // Определяем диапазон данных
    auto indexRange = GetDataIndexRange(minX, maxX);
    size_t startIndex = indexRange.first;
    size_t endIndex = indexRange.second;
    
    if (startIndex >= endIndex) {
        return;
    }
    
    // Вычисляем данные в зависимости от типа кривой
    switch (curve.dataType) {
        case DataType::AMPLITUDE_RATIO:
            CalculateRatioData(curve.elementIndex, curve.frequency, outData, minX, maxX);
            break;
            
        case DataType::PHASE_DIFFERENCE:
            CalculateDifferenceData(curve.elementIndex, curve.frequency, outData, minX, maxX);
            break;
            
        case DataType::AMPLITUDE_RX1:
            CalculateAmplitudeData(curve.elementIndex, 0, curve.frequency, outData, minX, maxX);
            break;
            
        case DataType::AMPLITUDE_RX2:
            CalculateAmplitudeData(curve.elementIndex, 1, curve.frequency, outData, minX, maxX);
            break;
            
        case DataType::PHASE_RX1:
            CalculatePhaseData(curve.elementIndex, 0, curve.frequency, outData, minX, maxX);
            break;
            
        case DataType::PHASE_RX2:
            CalculatePhaseData(curve.elementIndex, 1, curve.frequency, outData, minX, maxX);
            break;
            
        case DataType::SILENT_AMPLITUDE_RX1:
            CalculateSilentAmplitudeData(0, curve.frequency, outData, minX, maxX);
            break;
            
        case DataType::SILENT_AMPLITUDE_RX2:
            CalculateSilentAmplitudeData(1, curve.frequency, outData, minX, maxX);
            break;
            
        case DataType::TEMPERATURE:
            // Температура - особый случай
            for (size_t i = startIndex; i < endIndex; ++i) {
                float value = data[i].temperature;
                if (value >= minY && value <= maxY) {
                    outData.push_back(value);
                }
            }
            break;
            
        default:
            // Неизвестный тип данных
            break;
    }
}

void DataModule::GroupCurvesByAxis(const std::vector<CurveDefinition>& curves,
                                  std::vector<CurveDefinition>& leftAxisCurves,
                                  std::vector<CurveDefinition>& rightAxisCurves) const {
    leftAxisCurves.clear();
    rightAxisCurves.clear();
    
    for (size_t i = 0; i < curves.size(); i++) {
        const auto& curve = curves[i];
        
        if (!curve.enabled) {
            continue; // Пропускаем отключенные кривые
        }
        
        if (curve.yAxisGroup == 0) {
            leftAxisCurves.push_back(curve);
        } else if (curve.yAxisGroup == 1) {
            rightAxisCurves.push_back(curve);
        }
    }
}

// Внутренние методы для вычисления различных типов данных
void DataModule::CalculateRatioData(int transmitterIndex, FrequencyBand frequency,
                                   std::vector<float>& outData, 
                                   double minX, double maxX) const {
    auto indexRange = GetDataIndexRange(minX, maxX);
    size_t startIndex = indexRange.first;
    size_t endIndex = indexRange.second;
    
    int rxIdx = GetActiveTransmitterIndex(static_cast<Transmitter>(transmitterIndex), frequency);
    if (rxIdx == -1) return;
    
    // Резервируем память для оптимизации
    outData.reserve(endIndex - startIndex);
    
    // Константа для проверки деления на ноль
    const float EPSILON = 1e-10f;
    
    for (size_t i = startIndex; i < endIndex; ++i) {
        const GP_TOOL_DATA& record = data[i];
        float denominator = record.AM_RX_1[rxIdx];
        
        if (std::abs(denominator) > EPSILON) { // Избегаем деления на ноль
            float ratio = record.AM_RX_2[rxIdx] / denominator;
            outData.push_back(ratio);
        } else {
            outData.push_back(0.0f); // Или можно использовать std::numeric_limits<float>::quiet_NaN()
        }
    }
}

void DataModule::CalculateDifferenceData(int transmitterIndex, FrequencyBand frequency,
                                        std::vector<float>& outData,
                                        double minX, double maxX) const {
    auto indexRange = GetDataIndexRange(minX, maxX);
    size_t startIndex = indexRange.first;
    size_t endIndex = indexRange.second;
    
    int rxIdx = GetActiveTransmitterIndex(static_cast<Transmitter>(transmitterIndex), frequency);
    if (rxIdx == -1) return;
    
    // Резервируем память для оптимизации
    outData.reserve(endIndex - startIndex);
    
    for (size_t i = startIndex; i < endIndex; ++i) {
        const GP_TOOL_DATA& record = data[i];
        float difference = record.PH_RX_2[rxIdx] - record.PH_RX_1[rxIdx];
        difference = NormalizePhase(difference); // Нормализация в диапазон (-π, π]
        difference *= 57295.7795f; // Преобразование рад -> мдег
        outData.push_back(difference);
    }
}

void DataModule::CalculateAmplitudeData(int transmitterIndex, int receiverIndex, 
                                       FrequencyBand frequency, std::vector<float>& outData,
                                       double minX, double maxX) const {
    auto indexRange = GetDataIndexRange(minX, maxX);
    size_t startIndex = indexRange.first;
    size_t endIndex = indexRange.second;
    
    int rxIdx = GetActiveTransmitterIndex(static_cast<Transmitter>(transmitterIndex), frequency);
    if (rxIdx == -1) return;
    
    // Резервируем память для оптимизации
    outData.reserve(endIndex - startIndex);
    
    // Выбираем массив данных один раз для оптимизации
    const float* dataArray = (receiverIndex == 0) ? nullptr : nullptr; // Будет установлено в цикле
    
    for (size_t i = startIndex; i < endIndex; ++i) {
        const GP_TOOL_DATA& record = data[i];
        float amplitude = (receiverIndex == 0) ? record.AM_RX_1[rxIdx] : record.AM_RX_2[rxIdx];
        outData.push_back(amplitude * 4000.0f / pow(2.0f, 24.0f)); // Преобразование В -> мВ
    }
}

void DataModule::CalculatePhaseData(int transmitterIndex, int receiverIndex, 
                                   FrequencyBand frequency, std::vector<float>& outData,
                                   double minX, double maxX) const {
    auto indexRange = GetDataIndexRange(minX, maxX);
    size_t startIndex = indexRange.first;
    size_t endIndex = indexRange.second;
    
    int rxIdx = GetActiveTransmitterIndex(static_cast<Transmitter>(transmitterIndex), frequency);
    if (rxIdx == -1) return;
    
    // Резервируем память для оптимизации
    outData.reserve(endIndex - startIndex);
    
    for (size_t i = startIndex; i < endIndex; ++i) {
        const GP_TOOL_DATA& record = data[i];
        float phase = (receiverIndex == 0) ? record.PH_RX_1[rxIdx] : record.PH_RX_2[rxIdx];
        phase = NormalizePhase(phase); // Нормализация в диапазон (-π, π]
        outData.push_back(phase * 57295.7795f); // Преобразование рад -> мдег
    }
}

void DataModule::CalculateSilentAmplitudeData(int receiverIndex, FrequencyBand frequency,
                                             std::vector<float>& outData,
                                             double minX, double maxX) const {
    auto indexRange = GetDataIndexRange(minX, maxX);
    size_t startIndex = indexRange.first;
    size_t endIndex = indexRange.second;
    
    int rxIdx = GetSilentTransmitterIndex(frequency);
    if (rxIdx == -1) return;
    
    // Резервируем память для оптимизации
    outData.reserve(endIndex - startIndex);
    
    for (size_t i = startIndex; i < endIndex; ++i) {
        const GP_TOOL_DATA& record = data[i];
        float amplitude = (receiverIndex == 0) ? record.AM_RX_1[rxIdx] : record.AM_RX_2[rxIdx];
        outData.push_back(amplitude * 4000.0f / pow(2.0f, 24.0f)); // Преобразование В -> мВ
    }
}

// Метод нормализации фазы в диапазон (-π, π]
float DataModule::NormalizePhase(float phaseRadians) const {
    const float PI = 3.14159265358979323846f;
    
    // Приводим фазу к диапазону (-π, π]
    while (phaseRadians > PI) {
        phaseRadians -= 2.0f * PI;
    }
    while (phaseRadians <= -PI) {
        phaseRadians += 2.0f * PI;
    }
    
    return phaseRadians;
}

// Вспомогательные методы
int DataModule::GetDataArrayIndex(int transmitterIndex, int receiverIndex, 
                                 FrequencyBand frequency) const {
    // Этот метод может быть использован для более сложной логики индексации
    if (receiverIndex == 0 || receiverIndex == 1) {
        return GetActiveTransmitterIndex(static_cast<Transmitter>(transmitterIndex), frequency);
    }
    return -1;
}

bool DataModule::IsValidDataRange(double minX, double maxX) const {
    return minX >= 0 && maxX < static_cast<double>(data.size()) && minX <= maxX;
}

std::pair<size_t, size_t> DataModule::GetDataIndexRange(double minX, double maxX) const {
    if (data.empty()) {
        return {0, 0};
    }
    
    size_t startIndex = static_cast<size_t>(std::max(0.0, minX));
    size_t endIndex = static_cast<size_t>(std::min(static_cast<double>(data.size()), maxX + 1));
    
    // Убеждаемся, что индексы корректны
    startIndex = std::min(startIndex, data.size());
    endIndex = std::min(endIndex, data.size());
    
    if (startIndex >= endIndex) {
        return {0, 0};
    }
    
    return {startIndex, endIndex};
}

// Новые оптимизированные методы для работы с конфигурациями кривых
void DataModule::CalculateChartDataFromConfiguration(const ChartConfiguration& config,
                                                   std::vector<std::vector<float>>& leftAxisData,
                                                   std::vector<std::vector<float>>& rightAxisData) const {
    // Вызываем перегруженную версию без ограничений диапазона
    CalculateChartDataFromConfiguration(config, leftAxisData, rightAxisData,
                                      0.0, static_cast<double>(data.size() - 1), -1e6, 1e6);
}

void DataModule::CalculateChartDataFromConfiguration(const ChartConfiguration& config,
                                                   std::vector<std::vector<float>>& leftAxisData,
                                                   std::vector<std::vector<float>>& rightAxisData,
                                                   double minX, double maxX, double minY, double maxY) const {
    leftAxisData.clear();
    rightAxisData.clear();
    
    if (data.empty() || config.curves.empty()) {
        return;
    }
    
    // Используем новый метод для вычисления данных кривых
    CalculateCustomCurveData(config.curves, leftAxisData, rightAxisData, minX, maxX, minY, maxY);
}


// v2.10.0: Получение температурных данных для оси X
void DataModule::GetTemperatureData(std::vector<float>& outData) const {
    outData.clear();
    if (data.empty()) return;
    
    outData.reserve(data.size());
    for (const auto& record : data) {
        outData.push_back(record.temperature);
    }
}

void DataModule::GetTemperatureData(std::vector<float>& outData, double minX, double maxX) const {
    outData.clear();
    if (data.empty()) return;
    
    // Получаем диапазон индексов
    auto indexRange = GetDataIndexRange(minX, maxX);
    size_t startIndex = indexRange.first;
    size_t endIndex = indexRange.second;
    
    outData.reserve(endIndex - startIndex);
    for (size_t i = startIndex; i < endIndex; ++i) {
        outData.push_back(data[i].temperature);
    }
}
