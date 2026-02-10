#include "framework.h"
#include "DataModule.h"
#include "SchematicModule.h"
#include <fstream>
#include <algorithm>
#include <cmath>
#include <cwctype>  // v2.15.1: для towlower

DataModule::DataModule() {
}

DataModule::~DataModule() {
}

// v2.15.1: Определение формата файла по расширению
FileFormat DataModule::DetectFileFormat(const std::wstring& filename) {
    // Находим расширение файла
    size_t lastDot = filename.find_last_of(L'.');
    if (lastDot == std::wstring::npos) {
        return FileFormat::FORMAT_UNKNOWN;
    }
    
    std::wstring extension = filename.substr(lastDot);
    
    // Приводим к нижнему регистру для сравнения
    for (auto& c : extension) {
        c = towlower(c);
    }
    
    if (extension == L".bin") {
        return FileFormat::FORMAT_BIN;
    } else if (extension == L".biv") {
        return FileFormat::FORMAT_BIV;
    } else if (extension == L".dev") {
        return FileFormat::FORMAT_DEV;
    }
    
    return FileFormat::FORMAT_UNKNOWN;
}

bool DataModule::LoadDataFromFile(const std::wstring& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
        return false;
        
    data.clear();
    
    // v2.15.1: Определяем формат файла
    FileFormat format = DetectFileFormat(filename);
    
    // Определяем размер файла
    file.seekg(0, std::ios::end);
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    if (fileSize <= 0) {
        file.close();
        return false;
    }
    
    // Константы размеров
    const size_t GP_TOOL_DATA_SIZE = sizeof(GP_TOOL_DATA);  // 240 байт
    const size_t DEV_HEADER_SIZE = 11;                       // 11-байтный заголовок для .DEV
    const size_t DEV_BLOCK_SIZE = GP_TOOL_DATA_SIZE + DEV_HEADER_SIZE;  // 251 байт
    
    size_t frameCount = 0;
    
    switch (format) {
        case FileFormat::FORMAT_BIN:
        case FileFormat::FORMAT_BIV:
            // .bin и .biv: последовательность GP_TOOL_DATA без заголовков
            // Формула: количество_кадров = размер_файла / 240
            frameCount = static_cast<size_t>(fileSize) / GP_TOOL_DATA_SIZE;
            
            if (frameCount == 0) {
                file.close();
                return false;
            }
            
            data.reserve(frameCount);
            
            for (size_t n = 0; n < frameCount; ++n) {
                GP_TOOL_DATA record;
                // Смещение: n * 240
                file.seekg(n * GP_TOOL_DATA_SIZE);
                if (file.read(reinterpret_cast<char*>(&record), GP_TOOL_DATA_SIZE)) {
                    data.push_back(record);
                }
            }
            break;
            
        case FileFormat::FORMAT_DEV:
            // .DEV: 11-байтный заголовок + GP_TOOL_DATA для каждого кадра
            // Формула: количество_кадров = размер_файла / 251
            frameCount = static_cast<size_t>(fileSize) / DEV_BLOCK_SIZE;
            
            if (frameCount == 0) {
                file.close();
                return false;
            }
            
            data.reserve(frameCount);
            
            for (size_t n = 0; n < frameCount; ++n) {
                // Смещение блока: n * 251
                file.seekg(n * DEV_BLOCK_SIZE);
                
                // Пропускаем 11-байтный заголовок
                char headerBuffer[DEV_HEADER_SIZE];
                file.read(headerBuffer, DEV_HEADER_SIZE);
                
                // Читаем данные GP_TOOL_DATA
                GP_TOOL_DATA record;
                if (file.read(reinterpret_cast<char*>(&record), GP_TOOL_DATA_SIZE)) {
                    data.push_back(record);
                }
            }
            break;
            
        case FileFormat::FORMAT_UNKNOWN:
        default:
            // Неизвестный формат - пробуем читать как .bin
            frameCount = static_cast<size_t>(fileSize) / GP_TOOL_DATA_SIZE;
            
            if (frameCount == 0) {
                file.close();
                return false;
            }
            
            data.reserve(frameCount);
            
            GP_TOOL_DATA record;
            while (file.read(reinterpret_cast<char*>(&record), GP_TOOL_DATA_SIZE)) {
                data.push_back(record);
            }
            break;
    }
    
    file.close();
    
    // Сохраняем имя файла без пути и расширения
    if (!data.empty()) {
        // Извлекаем имя файла из полного пути
        size_t lastSlash = filename.find_last_of(L"\\/");
        std::wstring fileNameOnly = (lastSlash != std::wstring::npos) 
            ? filename.substr(lastSlash + 1) 
            : filename;
        
        // Удаляем расширение
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

// v2.13.0: Добавление данных в реальном времени
void DataModule::AddData(const GP_TOOL_DATA& newData) {
    data.push_back(newData);
}

void DataModule::AddDataBatch(const std::vector<GP_TOOL_DATA>& newDataBatch) {
    data.insert(data.end(), newDataBatch.begin(), newDataBatch.end());
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
    std::vector<bool> allTransmittersEnabled = {true, true, true, true, true}; // По умолчанию все передатчики включены
    CalculateChartData(type, freq, outData, allTransmittersEnabled, 0.0, static_cast<double>(data.size() - 1), -1e6, 1e6);
}

void DataModule::CalculateChartData(DataType type, FrequencyBand freq, std::vector<std::vector<float>>& outData,
                                   double minX, double maxX, double minY, double maxY) const {
    std::vector<bool> allTransmittersEnabled = {true, true, true, true, true}; // По умолчанию все передатчики включены
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
    
    const int freqIdx = (freq == FrequencyBand::FREQ_400_KHZ) ? 0 : 1;

    switch (type) {
    case DataType::PHASE_DIFFERENCE:
    case DataType::AMPLITUDE_RATIO:
    case DataType::AMPLITUDE_RX1:
    case DataType::AMPLITUDE_RX2:
    case DataType::PHASE_SMT:
    case DataType::RHO_SMT:
        {
            // 5 каналов для активных передатчиков
            outData.resize(5); // Каналы T1-T5

            bool validTransmitters[5];
            for (int t = 0; t < 5; ++t) {
                validTransmitters[t] = (t < (int)enabledTransmitters.size()) ? enabledTransmitters[t] : true;
            }

            // Резервируем память для оптимизации
            size_t expectedSize = endIndex - startIndex;
            for (int t = 0; t < 5; ++t) {
                if (validTransmitters[t]) {
                    outData[t].reserve(expectedSize);
                }
            }

            // Основной цикл вычислений
            for (size_t i = startIndex; i < endIndex; ++i) {
                const GP_TOOL_DATA& record = data[i];

                for (int t = 0; t < 5; ++t) {
                    if (!validTransmitters[t]) continue;

                    float value = 0.0f;

                    switch (type) {
                        case DataType::PHASE_DIFFERENCE:
                            {
                                float phaseDiff = record.DELTA_PH[freqIdx][t];
                                phaseDiff = NormalizePhase(phaseDiff);
                                value = phaseDiff * 57295.7795f; // рад -> мдег
                            }
                            break;
                        case DataType::AMPLITUDE_RATIO:
                            value = (record.AM_RX_1[freqIdx][t] != 0.0f) ?
                                   (record.AM_RX_2[freqIdx][t] / record.AM_RX_1[freqIdx][t]) : 0.0f;
                            break;
                        case DataType::AMPLITUDE_RX1:
                            value = record.AM_RX_1[freqIdx][t] * 4000.0f / pow(2.0f, 24.0f);
                            break;
                        case DataType::AMPLITUDE_RX2:
                            value = record.AM_RX_2[freqIdx][t] * 4000.0f / pow(2.0f, 24.0f);
                            break;
                        case DataType::PHASE_SMT:
                            {
                                float phase = record.phase_smt[freqIdx][t];
                                phase = NormalizePhase(phase);
                                value = phase * 57295.7795f; // рад -> мдег
                            }
                            break;
                        case DataType::RHO_SMT:
                            value = record.rho_smt[freqIdx][t];
                            break;
                        default:
                            continue;
                    }

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
            
            
            // Резервируем память для оптимизации
            outData[0].reserve(endIndex - startIndex);
            
            for (size_t i = startIndex; i < endIndex; ++i) {
                const GP_TOOL_DATA& record = data[i];
                float value = (type == DataType::SILENT_AMPLITUDE_RX1) ?
                             (record.ZERO_AM_RX_1[freqIdx] * 4000.0f / pow(2.0f, 24.0f)) :
                             (record.ZERO_AM_RX_2[freqIdx] * 4000.0f / pow(2.0f, 24.0f));
                
                // Проверяем диапазон значений для фильтрации
                if (value >= minY && value <= maxY) {
                    outData[0].push_back(value);
                }
            }
        }
        break;

    case DataType::SILENT_PHASE_DIFFERENCE:
        {
            // 1 канал: разность фаз молчащих передатчиков (R2−R1) по ZERO_dPH[freqIdx]
            outData.resize(1);
            outData[0].reserve(endIndex - startIndex);

            for (size_t i = startIndex; i < endIndex; ++i) {
                const GP_TOOL_DATA& record = data[i];
                float diff = record.ZERO_dPH[freqIdx];
                diff = NormalizePhase(diff);
                float value = diff * 57295.7795f; // рад -> мдег

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
    case DataType::PHASE_DIFFERENCE: return L"Разность фаз";
    case DataType::AMPLITUDE_RATIO: return L"Отношение амплитуд";
    case DataType::AMPLITUDE_RX1: return L"Амплитуда RX1";
    case DataType::AMPLITUDE_RX2: return L"Амплитуда RX2";
    case DataType::PHASE_SMT: return L"Фаза";
    case DataType::RHO_SMT: return L"УЭС";
    case DataType::SILENT_AMPLITUDE_RX1: return L"Молчащий Амплитуда RX1";
    case DataType::SILENT_AMPLITUDE_RX2: return L"Молчащий Амплитуда RX2";
    case DataType::SILENT_PHASE_DIFFERENCE: return L"Молчащий Разность фаз";
    case DataType::TEMPERATURE: return L"Temperature";
    default: return L"Unknown Type";
    }
}

// Вспомогательные методы
int DataModule::GetActiveTransmitterIndex(Transmitter transmitter, FrequencyBand freq) const {
    (void)freq;
    const int idx = static_cast<int>(transmitter);
    return (idx >= 0 && idx < 5) ? idx : -1;
}

int DataModule::GetSilentTransmitterIndex(FrequencyBand freq) const {
    // Для нового формата возвращаем индекс частоты (0=400, 1=2000)
    return (freq == FrequencyBand::FREQ_400_KHZ) ? 0 : 1;
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
            
        case DataType::PHASE_SMT:
            CalculatePhaseData(curve.elementIndex, 0, curve.frequency, outData, minX, maxX);
            break;

        case DataType::RHO_SMT:
            {
                auto indexRange = GetDataIndexRange(minX, maxX);
                size_t startIndex = indexRange.first;
                size_t endIndex = indexRange.second;

                const int freqIdx = (curve.frequency == FrequencyBand::FREQ_400_KHZ) ? 0 : 1;
                const int txIdx = GetActiveTransmitterIndex(static_cast<Transmitter>(curve.elementIndex), curve.frequency);
                if (txIdx == -1) break;

                outData.reserve(endIndex - startIndex);
                for (size_t i = startIndex; i < endIndex; ++i) {
                    float value = data[i].rho_smt[freqIdx][txIdx];
                    if (value >= minY && value <= maxY) {
                        outData.push_back(value);
                    }
                }
            }
            break;
            
        case DataType::SILENT_AMPLITUDE_RX1:
            CalculateSilentAmplitudeData(0, curve.frequency, outData, minX, maxX);
            break;
            
        case DataType::SILENT_AMPLITUDE_RX2:
            CalculateSilentAmplitudeData(1, curve.frequency, outData, minX, maxX);
            break;

        case DataType::SILENT_PHASE_DIFFERENCE:
            CalculateSilentPhaseDifferenceData(curve.frequency, outData, minX, maxX);
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

    const int freqIdx = (frequency == FrequencyBand::FREQ_400_KHZ) ? 0 : 1;
    const int txIdx = GetActiveTransmitterIndex(static_cast<Transmitter>(transmitterIndex), frequency);
    if (txIdx == -1) return;
    
    // Резервируем память для оптимизации
    outData.reserve(endIndex - startIndex);
    
    // Константа для проверки деления на ноль
    const float EPSILON = 1e-10f;
    
    for (size_t i = startIndex; i < endIndex; ++i) {
        const GP_TOOL_DATA& record = data[i];
        float denominator = record.AM_RX_1[freqIdx][txIdx];
        
        if (std::abs(denominator) > EPSILON) { // Избегаем деления на ноль
            float ratio = record.AM_RX_2[freqIdx][txIdx] / denominator;
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

    const int freqIdx = (frequency == FrequencyBand::FREQ_400_KHZ) ? 0 : 1;
    const int txIdx = GetActiveTransmitterIndex(static_cast<Transmitter>(transmitterIndex), frequency);
    if (txIdx == -1) return;
    
    // Резервируем память для оптимизации
    outData.reserve(endIndex - startIndex);
    
    for (size_t i = startIndex; i < endIndex; ++i) {
        const GP_TOOL_DATA& record = data[i];
        float difference = record.DELTA_PH[freqIdx][txIdx];
        difference = NormalizePhase(difference);
        outData.push_back(difference * 57295.7795f);
    }
}

void DataModule::CalculateAmplitudeData(int transmitterIndex, int receiverIndex, 
                                       FrequencyBand frequency, std::vector<float>& outData,
                                       double minX, double maxX) const {
    auto indexRange = GetDataIndexRange(minX, maxX);
    size_t startIndex = indexRange.first;
    size_t endIndex = indexRange.second;

    const int freqIdx = (frequency == FrequencyBand::FREQ_400_KHZ) ? 0 : 1;
    const int txIdx = GetActiveTransmitterIndex(static_cast<Transmitter>(transmitterIndex), frequency);
    if (txIdx == -1) return;
    
    // Резервируем память для оптимизации
    outData.reserve(endIndex - startIndex);
    
    for (size_t i = startIndex; i < endIndex; ++i) {
        const GP_TOOL_DATA& record = data[i];
        float amplitude = (receiverIndex == 0) ? record.AM_RX_1[freqIdx][txIdx] : record.AM_RX_2[freqIdx][txIdx];
        outData.push_back(amplitude * 4000.0f / pow(2.0f, 24.0f)); // Преобразование В -> мВ
    }
}

void DataModule::CalculatePhaseData(int transmitterIndex, int receiverIndex, 
                                   FrequencyBand frequency, std::vector<float>& outData,
                                   double minX, double maxX) const {
    (void)receiverIndex;
    auto indexRange = GetDataIndexRange(minX, maxX);
    size_t startIndex = indexRange.first;
    size_t endIndex = indexRange.second;

    const int freqIdx = (frequency == FrequencyBand::FREQ_400_KHZ) ? 0 : 1;
    const int txIdx = GetActiveTransmitterIndex(static_cast<Transmitter>(transmitterIndex), frequency);
    if (txIdx == -1) return;
    
    // Резервируем память для оптимизации
    outData.reserve(endIndex - startIndex);
    
    for (size_t i = startIndex; i < endIndex; ++i) {
        const GP_TOOL_DATA& record = data[i];
        float phase = record.phase_smt[freqIdx][txIdx];
        phase = NormalizePhase(phase);
        outData.push_back(phase * 57295.7795f);
    }
}

void DataModule::CalculateSilentAmplitudeData(int receiverIndex, FrequencyBand frequency,
                                             std::vector<float>& outData,
                                             double minX, double maxX) const {
    auto indexRange = GetDataIndexRange(minX, maxX);
    size_t startIndex = indexRange.first;
    size_t endIndex = indexRange.second;

    const int freqIdx = GetSilentTransmitterIndex(frequency);
    if (freqIdx == -1) return;
    
    // Резервируем память для оптимизации
    outData.reserve(endIndex - startIndex);
    
    for (size_t i = startIndex; i < endIndex; ++i) {
        const GP_TOOL_DATA& record = data[i];
        float amplitude = (receiverIndex == 0) ? record.ZERO_AM_RX_1[freqIdx] : record.ZERO_AM_RX_2[freqIdx];
        outData.push_back(amplitude * 4000.0f / pow(2.0f, 24.0f)); // Преобразование В -> мВ
    }
}

void DataModule::CalculateSilentPhaseDifferenceData(FrequencyBand frequency,
                                                   std::vector<float>& outData,
                                                   double minX, double maxX) const {
    auto indexRange = GetDataIndexRange(minX, maxX);
    size_t startIndex = indexRange.first;
    size_t endIndex = indexRange.second;

    const int freqIdx = GetSilentTransmitterIndex(frequency);
    if (freqIdx == -1) return;

    outData.reserve(endIndex - startIndex);

    for (size_t i = startIndex; i < endIndex; ++i) {
        const GP_TOOL_DATA& record = data[i];
        float diff = record.ZERO_dPH[freqIdx];
        diff = NormalizePhase(diff);
        outData.push_back(diff * 57295.7795f); // рад -> мдег
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
